#include "core/game_engine.h"

#include "core/city_content.h"

#include <QDateTime>
#include <algorithm>

namespace neon {

GameEngine::GameEngine() = default;

bool GameEngine::createGame(const QStringList &playerNames, int aiPlayers, int maxRounds,
                            quint64 seed, QString *error)
{
    if (playerNames.size() < 2 || playerNames.size() > 6 || aiPlayers < 0 || aiPlayers > playerNames.size()) {
        if (error) *error = QStringLiteral("A game requires 2 to 6 players");
        return false;
    }
    if (maxRounds != 16 && maxRounds != 24 && maxRounds != 32) {
        if (error) *error = QStringLiteral("Round count must be 16, 24, or 32");
        return false;
    }

    m_state = {};
    m_processedCommands.clear();
    m_state.matchId = QUuid::createUuid();
    m_state.randomSeed = seed ? seed : quint64(QDateTime::currentMSecsSinceEpoch());
    m_random.seed(m_state.randomSeed);
    m_state.maxRounds = maxRounds;
    m_state.phase = GamePhase::AwaitingRoll;
    m_state.tiles = CityContent::createNeonCityMap();

    const QList<QColor> colors = {QColor("#52F7FF"), QColor("#FF5CA8"), QColor("#FFD166"),
        QColor("#8AFF80"), QColor("#AA8BFF"), QColor("#FF875E")};
    const int firstAi = playerNames.size() - aiPlayers;
    for (int i = 0; i < playerNames.size(); ++i) {
        PlayerState player;
        player.id = QUuid::createUuid();
        player.name = playerNames.at(i).trimmed().isEmpty() ? QStringLiteral("玩家%1").arg(i + 1) : playerNames.at(i);
        player.color = colors.at(i);
        player.characterIndex = i;
        player.aiControlled = i >= firstAi;
        m_state.players.append(player);
    }
    for (const auto &tile : m_state.tiles) {
        if (tile.type == TileType::Property) m_state.properties.append({tile.index, {}, 0, false});
    }
    if (error) error->clear();
    return true;
}

bool GameEngine::validateCommon(const GameCommand &command, QString *error) const
{
    if (m_state.phase == GamePhase::Finished) {
        *error = QStringLiteral("The game is already finished");
        return false;
    }
    if (command.matchId != m_state.matchId || !m_state.activePlayer() || command.playerId != m_state.activePlayer()->id) {
        *error = QStringLiteral("Command is not from the active player");
        return false;
    }
    if (command.commandId == 0 || m_processedCommands.contains(command.commandId)) {
        *error = QStringLiteral("Duplicate or invalid command id");
        return false;
    }
    return true;
}

CommandResult GameEngine::apply(const GameCommand &command)
{
    QString error;
    if (!validateCommon(command, &error)) return {false, error, {}};

    CommandResult result;
    switch (command.type) {
    case CommandType::Roll: result = roll(command); break;
    case CommandType::BuyProperty: result = buy(command); break;
    case CommandType::UpgradeProperty: result = upgrade(command); break;
    case CommandType::MortgageProperty: result = mortgage(command); break;
    case CommandType::UseCard: result = useCard(command); break;
    case CommandType::UseSkill: result = useSkill(command); break;
    case CommandType::EndTurn: result = endTurn(command); break;
    default: result = {false, QStringLiteral("Command is not implemented for this phase"), {}}; break;
    }
    if (result.accepted) m_processedCommands.insert(command.commandId);
    return result;
}

CommandResult GameEngine::roll(const GameCommand &command)
{
    if (m_state.phase != GamePhase::AwaitingRoll) return {false, QStringLiteral("It is not time to roll"), {}};
    QList<GameEvent> events;
    auto *player = m_state.activePlayer();
    const int rawDice = int(m_random.bounded(1, 7));
    const int dice = rawDice + player->movementBonus;
    player->movementBonus = 0;
    const int oldPosition = player->position;
    player->position = (player->position + dice) % m_state.tiles.size();
    if (player->position < oldPosition) {
        const int bonus = 1000 + (player->characterIndex == 0 ? 100 : 0);
        player->cash += bonus;
        emitEvent(events, QStringLiteral("startBonus"), player->id, {{QStringLiteral("amount"), bonus}});
    }
    m_state.lastDice = rawDice;
    m_state.phase = GamePhase::AwaitingDecision;
    emitEvent(events, QStringLiteral("rolled"), player->id,
              {{QStringLiteral("dice"), dice}, {QStringLiteral("position"), player->position}});
    resolveLanding(*player, events);
    return {true, {}, events};
}

void GameEngine::resolveLanding(PlayerState &player, QList<GameEvent> &events)
{
    const auto *tile = m_state.tileAt(player.position);
    if (!tile) return;
    emitEvent(events, QStringLiteral("landed"), player.id,
              {{QStringLiteral("tile"), tile->index}, {QStringLiteral("name"), tile->name}});

    if (tile->type == TileType::Property) {
        auto *property = m_state.propertyAt(tile->index);
        if (property && !property->ownerId.isNull() && property->ownerId != player.id && !property->mortgaged) {
            auto ownerIt = std::find_if(m_state.players.begin(), m_state.players.end(),
                [&](const PlayerState &candidate) { return candidate.id == property->ownerId; });
            int rent = tile->baseRent * (property->level + 1) * (property->level + 1)
                * m_state.rentModifierPercent / 100;
            if (player.shieldCharges > 0) {
                --player.shieldCharges;
                rent = 0;
                emitEvent(events, QStringLiteral("shieldBlockedRent"), player.id);
            }
            const int payment = qMin(player.cash, rent);
            player.cash -= payment;
            if (ownerIt != m_state.players.end()) ownerIt->cash += payment;
            emitEvent(events, QStringLiteral("rentPaid"), player.id,
                {{QStringLiteral("amount"), payment}, {QStringLiteral("owner"), property->ownerId.toString()}});
            if (player.cash <= 0) {
                player.bankrupt = true;
                emitEvent(events, QStringLiteral("bankrupt"), player.id);
            }
        }
    } else if (tile->type == TileType::Tax) {
        const int tax = qMin(player.cash, player.characterIndex == 2 ? 350 : 500);
        player.cash -= tax;
        emitEvent(events, QStringLiteral("taxPaid"), player.id, {{QStringLiteral("amount"), tax}});
    } else if (tile->type == TileType::Event) {
        int delta = int(m_random.bounded(200, 1001)) * (m_random.bounded(2) ? 1 : -1);
        if (delta < 0 && player.characterIndex == 3) delta /= 2;
        player.cash = qMax(0, player.cash + delta);
        emitEvent(events, QStringLiteral("cityEvent"), player.id, {{QStringLiteral("cashDelta"), delta}});
    } else if (tile->type == TileType::Mission) {
        const int score = player.characterIndex == 4 ? 350 : 250;
        player.missionScore += score;
        emitEvent(events, QStringLiteral("missionProgress"), player.id, {{QStringLiteral("score"), score}});
    } else if (tile->type == TileType::Service) {
        player.energy = qMin(5, player.energy + (player.characterIndex == 5 ? 2 : 1));
        emitEvent(events, QStringLiteral("energyRestored"), player.id, {{QStringLiteral("energy"), player.energy}});
    } else if (tile->type == TileType::Shop && player.cash >= 400 && player.strategyCards.size() < 5) {
        player.cash -= 400;
        const int card = int(m_random.bounded(36));
        player.strategyCards.append(card);
        emitEvent(events, QStringLiteral("cardPurchased"), player.id,
                  {{QStringLiteral("card"), card}, {QStringLiteral("cost"), 400}});
    }
}

CommandResult GameEngine::buy(const GameCommand &command)
{
    if (m_state.phase != GamePhase::AwaitingDecision) return {false, QStringLiteral("No property decision is active"), {}};
    auto *player = m_state.activePlayer();
    const auto *tile = m_state.tileAt(player->position);
    auto *property = m_state.propertyAt(player->position);
    if (!tile || !property || tile->type != TileType::Property || !property->ownerId.isNull())
        return {false, QStringLiteral("This property cannot be purchased"), {}};
    int price = tile->price;
    if (player->characterIndex == 1) price = price * 95 / 100;
    if (player->cash < price) return {false, QStringLiteral("Not enough cash"), {}};
    player->cash -= price;
    const int rebate = price * m_state.purchaseRebatePercent / 100;
    player->cash += rebate;
    property->ownerId = player->id;
    QList<GameEvent> events;
    emitEvent(events, QStringLiteral("propertyBought"), player->id,
              {{QStringLiteral("tile"), tile->index}, {QStringLiteral("price"), price},
               {QStringLiteral("rebate"), rebate}});
    return {true, {}, events};
}

CommandResult GameEngine::upgrade(const GameCommand &command)
{
    if (m_state.phase != GamePhase::AwaitingDecision) return {false, QStringLiteral("No upgrade decision is active"), {}};
    auto *player = m_state.activePlayer();
    const int tileIndex = command.arguments.value(QStringLiteral("tile"), player->position).toInt();
    auto *property = m_state.propertyAt(tileIndex);
    const auto *tile = m_state.tileAt(tileIndex);
    if (!property || !tile || property->ownerId != player->id || property->mortgaged || property->level >= 3)
        return {false, QStringLiteral("This property cannot be upgraded"), {}};
    const int cost = tile->price / 2 * (property->level + 1) * m_state.buildingCostPercent / 100;
    if (player->cash < cost) return {false, QStringLiteral("Not enough cash"), {}};
    player->cash -= cost;
    ++property->level;
    QList<GameEvent> events;
    emitEvent(events, QStringLiteral("propertyUpgraded"), player->id,
              {{QStringLiteral("tile"), tileIndex}, {QStringLiteral("level"), property->level},
               {QStringLiteral("cost"), cost}});
    return {true, {}, events};
}

CommandResult GameEngine::endTurn(const GameCommand &)
{
    if (m_state.phase != GamePhase::AwaitingDecision) return {false, QStringLiteral("The turn cannot end yet"), {}};
    QList<GameEvent> events;
    advancePlayer(events);
    return {true, {}, events};
}

CommandResult GameEngine::mortgage(const GameCommand &command)
{
    if (m_state.phase != GamePhase::AwaitingDecision) return {false, QStringLiteral("No property decision is active"), {}};
    auto *player = m_state.activePlayer();
    const int tileIndex = command.arguments.value(QStringLiteral("tile"), player->position).toInt();
    auto *property = m_state.propertyAt(tileIndex);
    const auto *tile = m_state.tileAt(tileIndex);
    if (!property || !tile || property->ownerId != player->id)
        return {false, QStringLiteral("You do not own this property"), {}};
    QList<GameEvent> events;
    if (!property->mortgaged) {
        const int value = tile->price / 2 + property->level * tile->price / 4;
        player->cash += value;
        property->mortgaged = true;
        emitEvent(events, QStringLiteral("propertyMortgaged"), player->id,
                  {{QStringLiteral("tile"), tileIndex}, {QStringLiteral("amount"), value}});
    } else {
        const int cost = tile->price * 60 / 100 + property->level * tile->price / 4;
        if (player->cash < cost) return {false, QStringLiteral("Not enough cash to redeem"), {}};
        player->cash -= cost;
        property->mortgaged = false;
        emitEvent(events, QStringLiteral("propertyRedeemed"), player->id,
                  {{QStringLiteral("tile"), tileIndex}, {QStringLiteral("cost"), cost}});
    }
    return {true, {}, events};
}

CommandResult GameEngine::useCard(const GameCommand &command)
{
    auto *player = m_state.activePlayer();
    const int slot = command.arguments.value(QStringLiteral("slot"), 0).toInt();
    if (!player || slot < 0 || slot >= player->strategyCards.size())
        return {false, QStringLiteral("Card is not available"), {}};
    const int card = player->strategyCards.takeAt(slot);
    switch (card % 6) {
    case 0: player->movementBonus += 2; break;
    case 1: ++player->shieldCharges; break;
    case 2: player->cash += 700; break;
    case 3: player->energy = qMin(5, player->energy + 2); break;
    case 4: player->missionScore += 300; break;
    case 5: player->movementBonus += 1; player->cash += 300; break;
    }
    QList<GameEvent> events;
    emitEvent(events, QStringLiteral("cardUsed"), player->id, {{QStringLiteral("card"), card}});
    return {true, {}, events};
}

CommandResult GameEngine::useSkill(const GameCommand &)
{
    auto *player = m_state.activePlayer();
    if (!player || m_state.phase != GamePhase::AwaitingRoll || player->energy < 2 || player->skillCooldown > 0)
        return {false, QStringLiteral("Character skill is not ready"), {}};
    player->energy -= 2;
    player->skillCooldown = 3;
    switch (player->characterIndex) {
    case 0: player->movementBonus += 3; break;
    case 1: player->cash += 900; break;
    case 2: player->shieldCharges += 2; break;
    case 3: player->strategyCards.append(int(m_random.bounded(36))); break;
    case 4: player->missionScore += 500; break;
    case 5: player->energy = qMin(5, player->energy + 1); player->movementBonus += 1; break;
    }
    QList<GameEvent> events;
    emitEvent(events, QStringLiteral("characterSkillUsed"), player->id,
              {{QStringLiteral("character"), player->characterIndex}});
    return {true, {}, events};
}

void GameEngine::advancePlayer(QList<GameEvent> &events)
{
    const int alive = std::count_if(m_state.players.cbegin(), m_state.players.cend(),
                                    [](const PlayerState &p) { return !p.bankrupt; });
    if (alive <= 1 || m_state.round > m_state.maxRounds) {
        m_state.phase = GamePhase::Finished;
        emitEvent(events, QStringLiteral("gameFinished"), {});
        return;
    }
    do {
        m_state.currentPlayer = (m_state.currentPlayer + 1) % m_state.players.size();
        if (m_state.currentPlayer == 0) {
            ++m_state.round;
            if (m_state.round > m_state.maxRounds) {
                m_state.phase = GamePhase::Finished;
                emitEvent(events, QStringLiteral("gameFinished"), {});
                return;
            }
            if ((m_state.round - 1) % 4 == 0) {
                activateCityPulse(events);
            }
        }
    } while (m_state.players.at(m_state.currentPlayer).bankrupt);
    m_state.phase = GamePhase::AwaitingRoll;
    if (m_state.activePlayer()->skillCooldown > 0) --m_state.activePlayer()->skillCooldown;
    emitEvent(events, QStringLiteral("turnStarted"), m_state.activePlayer()->id,
              {{QStringLiteral("round"), m_state.round}});
}

void GameEngine::activateCityPulse(QList<GameEvent> &events)
{
    const auto pulses = CityContent::cityPulseEvents();
    m_state.activePulse = ((m_state.round - 1) / 4 - 1) % pulses.size();
    m_state.rentModifierPercent = 100;
    m_state.buildingCostPercent = 100;
    m_state.purchaseRebatePercent = 0;
    switch (m_state.activePulse) {
    case 0: m_state.rentModifierPercent = 125; break;
    case 1: for (auto &player : m_state.players) player.movementBonus += 1; break;
    case 2: m_state.buildingCostPercent = 125; break;
    case 3: m_state.purchaseRebatePercent = 15; break;
    case 4: for (auto &player : m_state.players) player.energy = qMin(5, player.energy + 1); break;
    case 5: for (auto &player : m_state.players) player.shieldCharges += 1; break;
    case 6: for (auto &player : m_state.players) if (player.cash >= 150) player.cash -= 150; break;
    case 7: m_state.buildingCostPercent = 75; break;
    }
    emitEvent(events, QStringLiteral("cityPulse"), {},
              {{QStringLiteral("index"), m_state.activePulse},
               {QStringLiteral("name"), pulses.at(m_state.activePulse)}});
}

void GameEngine::emitEvent(QList<GameEvent> &events, const QString &type, const QUuid &playerId,
                           const QVariantMap &data)
{
    GameEvent event{++m_state.sequence, type, playerId, data};
    events.append(event);
    m_state.eventLog.append(event);
}

int GameEngine::finalScore(const QUuid &playerId) const
{
    const auto playerIt = std::find_if(m_state.players.cbegin(), m_state.players.cend(),
        [&](const PlayerState &player) { return player.id == playerId; });
    if (playerIt == m_state.players.cend()) return 0;
    int score = playerIt->cash + playerIt->missionScore;
    for (const auto &property : m_state.properties) {
        if (property.ownerId != playerId) continue;
        const auto *tile = m_state.tileAt(property.tileIndex);
        if (tile) score += tile->price * (property.mortgaged ? 40 : 70) / 100
            + property.level * tile->price / 3;
    }
    for (int district = 0; district < 8; ++district) {
        const int owned = std::count_if(m_state.properties.cbegin(), m_state.properties.cend(),
            [&](const PropertyState &property) {
                const auto *tile = m_state.tileAt(property.tileIndex);
                return property.ownerId == playerId && tile && tile->district == district;
            });
        if (owned == 3) score += 1200;
    }
    return score;
}

void GameEngine::restore(GameState state)
{
    m_state = std::move(state);
    m_random.seed(m_state.randomSeed ^ m_state.sequence);
    m_processedCommands.clear();
}

bool GameEngine::setAiControlled(const QUuid &playerId, bool controlled)
{
    auto it = std::find_if(m_state.players.begin(), m_state.players.end(),
                           [&](const PlayerState &player) { return player.id == playerId; });
    if (it == m_state.players.end() || it->aiControlled == controlled) return false;
    it->aiControlled = controlled;
    QList<GameEvent> ignored;
    emitEvent(ignored, controlled ? QStringLiteral("aiTakeover") : QStringLiteral("playerReconnected"), playerId);
    return true;
}

} // namespace neon

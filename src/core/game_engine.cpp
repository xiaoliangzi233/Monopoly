#include "core/game_engine.h"

#include "core/city_content.h"

#include <QDateTime>
#include <algorithm>
#include <functional>

namespace neon {

GameEngine::GameEngine() = default;

bool GameEngine::createGame(const QStringList &playerNames, int aiPlayers, int maxRounds,
                            quint64 seed, QString *error)
{
    if (playerNames.size() < 2 || playerNames.size() > 6 || aiPlayers < 0 || aiPlayers > playerNames.size()) {
        if (error) *error = QStringLiteral("对局需要2至6名玩家");
        return false;
    }
    if (maxRounds != 90 && maxRounds != 120 && maxRounds != 150) {
        if (error) *error = QStringLiteral("回合数必须为90、120或150");
        return false;
    }
    m_state = {};
    m_processedCommands.clear();
    m_state.matchId = QUuid::createUuid();
    m_state.rulesVersion = 2;
    m_state.contentVersion = CityContent::ContentVersion;
    m_state.contentHash = CityContent::contentHash();
    m_state.randomSeed = seed ? seed : quint64(QDateTime::currentMSecsSinceEpoch());
    m_random.seed(m_state.randomSeed);
    m_state.maxRounds = maxRounds;
    m_state.phase = GamePhase::AwaitingRoll;
    m_state.tiles = CityContent::createProsperousCityMap();

    const QList<QColor> colors = {QColor("#B83A2D"), QColor("#236B63"), QColor("#D39B34"),
        QColor("#6A4C93"), QColor("#2E5E9E"), QColor("#8A5A32")};
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
    for (const auto &tile : m_state.tiles)
        if (tile.type == TileType::Property) m_state.properties.append({tile.index, {}, 0, false});
    if (error) error->clear();
    return true;
}

PlayerState *GameEngine::playerById(const QUuid &id)
{
    auto it = std::find_if(m_state.players.begin(), m_state.players.end(), [&](const auto &p) { return p.id == id; });
    return it == m_state.players.end() ? nullptr : &*it;
}

const PlayerState *GameEngine::playerById(const QUuid &id) const
{
    auto it = std::find_if(m_state.players.cbegin(), m_state.players.cend(), [&](const auto &p) { return p.id == id; });
    return it == m_state.players.cend() ? nullptr : &*it;
}

bool GameEngine::validateCommon(const GameCommand &command, QString *error) const
{
    if (m_state.phase == GamePhase::Finished) { *error = QStringLiteral("对局已经结束"); return false; }
    if (m_state.phase == GamePhase::Moving) { *error = QStringLiteral("角色正在移动，请稍候"); return false; }
    const auto *sender = playerById(command.playerId);
    if (command.matchId != m_state.matchId || !sender || sender->bankrupt) {
        *error = QStringLiteral("命令不属于当前对局或玩家无效"); return false;
    }
    if (command.commandId == 0 || m_processedCommands.contains(command.commandId)) {
        *error = QStringLiteral("命令编号重复或无效"); return false;
    }
    const bool auctionCommand = m_state.phase == GamePhase::Auction
        && (command.type == CommandType::PlaceBid || command.type == CommandType::PassAuction);
    const bool tradeResponse = m_state.phase == GamePhase::Trade && command.type == CommandType::RespondTrade
        && command.playerId == m_state.tradeOffer.recipientId;
    if (!auctionCommand && !tradeResponse
        && (!m_state.activePlayer() || command.playerId != m_state.activePlayer()->id)) {
        *error = QStringLiteral("当前不是该玩家的操作阶段"); return false;
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
    case CommandType::UseReroll: result = reroll(command); break;
    case CommandType::ChooseRoute: result = chooseRoute(command); break;
    case CommandType::BuyProperty: result = buy(command); break;
    case CommandType::UpgradeProperty: result = upgrade(command); break;
    case CommandType::MortgageProperty: result = mortgage(command); break;
    case CommandType::UseCard: result = useCard(command); break;
    case CommandType::UseSkill: result = useSkill(command); break;
    case CommandType::EndTurn: result = endTurn(command); break;
    case CommandType::PlaceBid: result = placeBid(command); break;
    case CommandType::PassAuction: result = passAuction(command); break;
    case CommandType::ProposeTrade: result = proposeTrade(command); break;
    case CommandType::RespondTrade: result = respondTrade(command); break;
    case CommandType::SellProperty: result = sellProperty(command); break;
    case CommandType::ContributeCivic: result = contributeCivic(command); break;
    default: result = {false, QStringLiteral("当前阶段不支持该命令"), {}}; break;
    }
    if (result.accepted) m_processedCommands.insert(command.commandId);
    return result;
}

QList<QList<int>> GameEngine::enumerateRoutes(const PlayerState &player, int steps) const
{
    QList<QList<int>> routes;
    QList<int> path{player.position};
    std::function<void(int)> visit = [&](int remaining) {
        if (routes.size() >= 8) return;
        if (remaining == 0) { routes.append(path.mid(1)); return; }
        const auto *tile = m_state.tileAt(path.last());
        if (!tile) return;
        QList<int> choices;
        for (int neighbor : tile->neighbors) if (!path.contains(neighbor)) choices.append(neighbor);
        if (path.size() == 1 && choices.size() > 1 && choices.contains(player.previousPosition))
            choices.removeAll(player.previousPosition);
        if (choices.isEmpty() && player.previousPosition >= 0 && path.size() == 1) choices.append(player.previousPosition);
        for (int neighbor : choices) {
            path.append(neighbor); visit(remaining - 1); path.removeLast();
            if (routes.size() >= 8) break;
        }
    };
    visit(steps);
    QList<QList<int>> unique;
    QSet<int> endpoints;
    for (const auto &route : routes) {
        if (!route.isEmpty() && !endpoints.contains(route.last())) {
            endpoints.insert(route.last()); unique.append(route);
        }
    }
    return unique;
}

CommandResult GameEngine::roll(const GameCommand &)
{
    if (m_state.phase != GamePhase::AwaitingRoll) return {false, QStringLiteral("当前不能投掷骰子"), {}};
    QList<GameEvent> events;
    auto *player = m_state.activePlayer();
    const int raw = int(m_random.bounded(1, 7));
    const int steps = qBound(1, raw + player->movementBonus, 9);
    player->movementBonus = 0;
    m_state.lastDice = raw;
    m_state.pendingDice = steps;
    m_state.routeOptions = enumerateRoutes(*player, steps);
    emitEvent(events, QStringLiteral("rolled"), player->id, {{QStringLiteral("dice"), raw}, {QStringLiteral("steps"), steps}});
    if (m_state.routeOptions.isEmpty()) return {false, QStringLiteral("地图路线异常，无法生成移动路径"), {}};
    if (m_state.routeOptions.size() == 1) beginMovement(*player, m_state.routeOptions.first(), events);
    else {
        m_state.phase = GamePhase::AwaitingRoute;
        emitEvent(events, QStringLiteral("routeChoiceRequested"), player->id,
                  {{QStringLiteral("count"), m_state.routeOptions.size()}});
    }
    return {true, {}, events};
}

CommandResult GameEngine::reroll(const GameCommand &command)
{
    auto *player = m_state.activePlayer();
    if (!player || m_state.phase != GamePhase::AwaitingRoute || player->rerolls <= 0)
        return {false, QStringLiteral("当前没有可用的重掷机会"), {}};
    --player->rerolls;
    m_state.phase = GamePhase::AwaitingRoll;
    m_state.routeOptions.clear();
    auto result = roll(command);
    if (result.accepted) emitEvent(result.events, QStringLiteral("rerolled"), player->id);
    return result;
}

CommandResult GameEngine::chooseRoute(const GameCommand &command)
{
    if (m_state.phase != GamePhase::AwaitingRoute) return {false, QStringLiteral("当前没有待选路线"), {}};
    const int option = command.arguments.value(QStringLiteral("option"), -1).toInt();
    if (option < 0 || option >= m_state.routeOptions.size()) return {false, QStringLiteral("路线选项无效"), {}};
    QList<GameEvent> events;
    beginMovement(*m_state.activePlayer(), m_state.routeOptions.at(option), events);
    return {true, {}, events};
}

void GameEngine::beginMovement(PlayerState &player, const QList<int> &route, QList<GameEvent> &events)
{
    m_state.phase = GamePhase::Moving;
    QVariantList path;
    for (int node : route) path.append(node);
    m_state.movingPlayerId = player.id;
    m_state.pendingMovePath = route;
    m_state.pendingMoveIndex = 0;
    ++m_state.movementSerial;
    emitEvent(events, QStringLiteral("movementStarted"), player.id,
              {{QStringLiteral("path"), path}, {QStringLiteral("serial"), QVariant::fromValue(m_state.movementSerial)}});
    m_state.routeOptions.clear();
    m_state.pendingDice = 0;
}

CommandResult GameEngine::advanceMovementStep()
{
    if (m_state.phase != GamePhase::Moving)
        return {false, QStringLiteral("当前没有正在进行的移动"), {}};
    auto *player = playerById(m_state.movingPlayerId);
    if (!player || m_state.pendingMoveIndex < 0
        || m_state.pendingMoveIndex >= m_state.pendingMovePath.size()) {
        return {false, QStringLiteral("移动状态损坏，无法继续"), {}};
    }

    QList<GameEvent> events;
    player->previousPosition = player->position;
    player->position = m_state.pendingMovePath.at(m_state.pendingMoveIndex++);
    emitEvent(events, QStringLiteral("movementStep"), player->id,
              {{QStringLiteral("position"), player->position},
               {QStringLiteral("index"), m_state.pendingMoveIndex},
               {QStringLiteral("total"), m_state.pendingMovePath.size()},
               {QStringLiteral("serial"), QVariant::fromValue(m_state.movementSerial)}});

    if (m_state.pendingMoveIndex >= m_state.pendingMovePath.size()) {
        const quint64 serial = m_state.movementSerial;
        m_state.movingPlayerId = QUuid{};
        m_state.pendingMovePath.clear();
        m_state.pendingMoveIndex = 0;
        m_state.phase = GamePhase::AwaitingDecision;
        emitEvent(events, QStringLiteral("movementCompleted"), player->id,
                  {{QStringLiteral("position"), player->position},
                   {QStringLiteral("serial"), QVariant::fromValue(serial)}});
        resolveLanding(*player, events);
    }
    return {true, {}, events};
}

bool GameEngine::ensureFunds(PlayerState &player, int amount, QList<GameEvent> &events)
{
    while (player.cash < amount) {
        auto it = std::find_if(m_state.properties.begin(), m_state.properties.end(), [&](const auto &property) {
            return property.ownerId == player.id && !property.mortgaged;
        });
        if (it == m_state.properties.end()) break;
        const auto *tile = m_state.tileAt(it->tileIndex);
        const int value = tile ? tile->price / 2 + it->level * tile->price / 4 : 0;
        it->mortgaged = true;
        player.cash += value;
        emitEvent(events, QStringLiteral("forcedPawn"), player.id,
                  {{QStringLiteral("tile"), it->tileIndex}, {QStringLiteral("amount"), value}});
    }
    if (player.cash >= amount) return true;
    player.bankrupt = true;
    for (auto &property : m_state.properties) {
        if (property.ownerId == player.id) { property.ownerId = QUuid{}; property.level = 0; property.mortgaged = false; }
    }
    emitEvent(events, QStringLiteral("bankrupt"), player.id);
    return false;
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
            auto *owner = playerById(property->ownerId);
            int revenue = tile->baseRent * (property->level + 1) * (property->level + 1)
                * m_state.rentModifierPercent / 100;
            if (owner && owner->characterIndex == 1) {
                const int ownedInDistrict = std::count_if(m_state.properties.cbegin(), m_state.properties.cend(),
                    [&](const auto &p) { const auto *t = m_state.tileAt(p.tileIndex);
                        return p.ownerId == owner->id && t && t->district == tile->district; });
                if (ownedInDistrict == 4) revenue = revenue * 110 / 100;
            }
            if (owner && owner->boostedIndustryTile == tile->index && owner->boostedCollections > 0) {
                revenue = revenue * 150 / 100;
                if (--owner->boostedCollections == 0) owner->boostedIndustryTile = -1;
            }
            if (player.shieldCharges > 0) { --player.shieldCharges; revenue = 0; }
            if (ensureFunds(player, revenue, events)) {
                player.cash -= revenue;
                if (owner) { owner->cash += revenue; owner->reputation = qMin(100, owner->reputation + 1); }
            }
            emitEvent(events, QStringLiteral("industryRevenue"), player.id,
                      {{QStringLiteral("amount"), revenue}, {QStringLiteral("owner"), property->ownerId.toString()}});
        }
    } else if (tile->type == TileType::Tax) {
        const int tax = 650;
        if (ensureFunds(player, tax, events)) player.cash -= tax;
        emitEvent(events, QStringLiteral("taxPaid"), player.id, {{QStringLiteral("amount"), tax}});
    } else if (tile->type == TileType::Event) {
        int delta = int(m_random.bounded(300, 1201)) * (m_random.bounded(2) ? 1 : -1);
        if (delta < 0 && player.characterIndex == 2) delta = delta * 75 / 100;
        if (delta < 0) ensureFunds(player, -delta, events);
        player.cash = qMax(0, player.cash + delta);
        player.reputation = qBound(0, player.reputation + (delta > 0 ? 2 : -1), 100);
        emitEvent(events, QStringLiteral("marketEncounter"), player.id, {{QStringLiteral("cashDelta"), delta}});
    } else if (tile->type == TileType::Commission) {
        const int mission = int(m_random.bounded(24));
        if (!player.commissions.contains(mission)) player.commissions.append(mission);
        player.culture = qMin(100, player.culture + (player.characterIndex == 4 ? 6 : 5));
        emitEvent(events, QStringLiteral("commissionAccepted"), player.id, {{QStringLiteral("mission"), mission}});
    } else if (tile->type == TileType::Civic) {
        player.energy = qMin(6, player.energy + 1);
        player.livelihood = qMin(100, player.livelihood + 3);
        emitEvent(events, QStringLiteral("civicVisited"), player.id);
    } else if (tile->type == TileType::Guild && player.cash >= 500 && player.strategyCards.size() < 6) {
        player.cash -= 500;
        const int card = int(m_random.bounded(48));
        player.strategyCards.append(card);
        emitEvent(events, QStringLiteral("strategyAcquired"), player.id, {{QStringLiteral("card"), card}});
    } else if (tile->type == TileType::Transit) {
        if (player.characterIndex == 5) player.reputation = qMin(100, player.reputation + 3);
        player.movementBonus += 1;
        emitEvent(events, QStringLiteral("transitVisited"), player.id);
    } else if (tile->type == TileType::Festival) {
        player.culture = qMin(100, player.culture + 7);
        player.reputation = qMin(100, player.reputation + 4);
        emitEvent(events, QStringLiteral("festivalVisited"), player.id);
    }
    // Forced pawning is completed synchronously. If it still cannot cover the
    // landing charge, do not leave a bankrupt player as the active actor.
    if (player.bankrupt) advancePlayer(events);
}

CommandResult GameEngine::buy(const GameCommand &)
{
    if (m_state.phase != GamePhase::AwaitingDecision) return {false, QStringLiteral("当前不能购买产业"), {}};
    auto *player = m_state.activePlayer();
    const auto *tile = m_state.tileAt(player->position);
    auto *property = m_state.propertyAt(player->position);
    if (!tile || !property || tile->type != TileType::Property || !property->ownerId.isNull())
        return {false, QStringLiteral("该产业不能购买"), {}};
    int price = tile->price;
    if (player->cash < price) return {false, QStringLiteral("资金不足"), {}};
    player->cash -= price;
    player->cash += price * m_state.purchaseRebatePercent / 100;
    player->reputation = qMin(100, player->reputation + 3);
    property->ownerId = player->id;
    QList<GameEvent> events;
    emitEvent(events, QStringLiteral("industryBought"), player->id,
              {{QStringLiteral("tile"), tile->index}, {QStringLiteral("price"), price}});
    return {true, {}, events};
}

CommandResult GameEngine::upgrade(const GameCommand &command)
{
    if (m_state.phase != GamePhase::AwaitingDecision) return {false, QStringLiteral("当前不能扩建产业"), {}};
    auto *player = m_state.activePlayer();
    const int tileIndex = command.arguments.value(QStringLiteral("tile"), player->position).toInt();
    auto *property = m_state.propertyAt(tileIndex);
    const auto *tile = m_state.tileAt(tileIndex);
    if (!property || !tile || property->ownerId != player->id || property->mortgaged || property->level >= 3)
        return {false, QStringLiteral("该产业不能扩建"), {}};
    int cost = tile->price / 2 * (property->level + 1) * m_state.buildingCostPercent / 100;
    if (player->characterIndex == 0) cost = cost * 88 / 100;
    if (player->upgradeDiscountPercent > 0) {
        cost = cost * (100 - player->upgradeDiscountPercent) / 100;
        player->upgradeDiscountPercent = 0;
    }
    if (player->cash < cost) return {false, QStringLiteral("资金不足"), {}};
    player->cash -= cost;
    ++property->level;
    player->reputation = qMin(100, player->reputation + 2 + property->level);
    QList<GameEvent> events;
    emitEvent(events, QStringLiteral("industryUpgraded"), player->id,
              {{QStringLiteral("tile"), tileIndex}, {QStringLiteral("level"), property->level},
               {QStringLiteral("cost"), cost}});
    return {true, {}, events};
}

void GameEngine::startAuction(int tileIndex, QList<GameEvent> &events)
{
    const auto *tile = m_state.tileAt(tileIndex);
    m_state.phase = GamePhase::Auction;
    m_state.auctionTile = tileIndex;
    m_state.auctionHighBid = tile ? tile->price / 2 : 0;
    m_state.auctionHighBidder = QUuid{};
    m_state.auctionPassedPlayers.clear();
    m_state.auctionDeadlineMs = QDateTime::currentMSecsSinceEpoch() + 30000;
    emitEvent(events, QStringLiteral("auctionStarted"), {},
              {{QStringLiteral("tile"), tileIndex}, {QStringLiteral("openingBid"), m_state.auctionHighBid},
               {QStringLiteral("deadline"), m_state.auctionDeadlineMs}});
}

void GameEngine::settleAuction(QList<GameEvent> &events)
{
    if (!m_state.auctionHighBidder.isNull()) {
        auto *winner = playerById(m_state.auctionHighBidder);
        auto *property = m_state.propertyAt(m_state.auctionTile);
        if (winner && property && winner->cash >= m_state.auctionHighBid) {
            winner->cash -= m_state.auctionHighBid;
            winner->reputation = qMin(100, winner->reputation + 4);
            property->ownerId = winner->id;
            emitEvent(events, QStringLiteral("auctionWon"), winner->id,
                      {{QStringLiteral("tile"), m_state.auctionTile}, {QStringLiteral("bid"), m_state.auctionHighBid}});
        }
    } else emitEvent(events, QStringLiteral("auctionUnsold"), {}, {{QStringLiteral("tile"), m_state.auctionTile}});
    m_state.auctionTile = -1; m_state.auctionHighBid = 0; m_state.auctionHighBidder = QUuid{};
    m_state.auctionPassedPlayers.clear(); m_state.auctionDeadlineMs = 0;
    advancePlayer(events);
}

CommandResult GameEngine::placeBid(const GameCommand &command)
{
    if (m_state.phase != GamePhase::Auction) return {false, QStringLiteral("当前没有竞价"), {}};
    auto *bidder = playerById(command.playerId);
    const int amount = command.arguments.value(QStringLiteral("amount"), m_state.auctionHighBid + 100).toInt();
    if (!bidder || amount < m_state.auctionHighBid + 100 || amount > bidder->cash)
        return {false, QStringLiteral("出价无效或资金不足"), {}};
    m_state.auctionHighBid = amount;
    m_state.auctionHighBidder = bidder->id;
    m_state.auctionPassedPlayers.removeAll(bidder->id);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_state.auctionDeadlineMs - now <= 3000) m_state.auctionDeadlineMs = now + 5000;
    QList<GameEvent> events;
    emitEvent(events, QStringLiteral("auctionBid"), bidder->id,
              {{QStringLiteral("amount"), amount}, {QStringLiteral("deadline"), m_state.auctionDeadlineMs}});
    return {true, {}, events};
}

CommandResult GameEngine::passAuction(const GameCommand &command)
{
    if (m_state.phase != GamePhase::Auction) return {false, QStringLiteral("当前没有竞价"), {}};
    if (!m_state.auctionPassedPlayers.contains(command.playerId)) m_state.auctionPassedPlayers.append(command.playerId);
    QList<GameEvent> events;
    emitEvent(events, QStringLiteral("auctionPassed"), command.playerId);
    int required = std::count_if(m_state.players.cbegin(), m_state.players.cend(), [](const auto &p) { return !p.bankrupt; });
    if (!m_state.auctionHighBidder.isNull()) --required;
    if (m_state.auctionPassedPlayers.size() >= required) settleAuction(events);
    return {true, {}, events};
}

CommandResult GameEngine::endTurn(const GameCommand &)
{
    if (m_state.phase != GamePhase::AwaitingDecision) return {false, QStringLiteral("当前不能结束回合"), {}};
    QList<GameEvent> events;
    const auto *player = m_state.activePlayer();
    const auto *tile = player ? m_state.tileAt(player->position) : nullptr;
    const auto *property = player ? m_state.propertyAt(player->position) : nullptr;
    if (tile && property && property->ownerId.isNull()) startAuction(tile->index, events);
    else advancePlayer(events);
    return {true, {}, events};
}

CommandResult GameEngine::mortgage(const GameCommand &command)
{
    if (m_state.phase != GamePhase::AwaitingDecision && m_state.phase != GamePhase::ForcedSettlement)
        return {false, QStringLiteral("当前不能典当产业"), {}};
    auto *player = m_state.activePlayer();
    const int tileIndex = command.arguments.value(QStringLiteral("tile"), player->position).toInt();
    auto *property = m_state.propertyAt(tileIndex);
    const auto *tile = m_state.tileAt(tileIndex);
    if (!property || !tile || property->ownerId != player->id) return {false, QStringLiteral("你并不拥有该产业"), {}};
    QList<GameEvent> events;
    if (!property->mortgaged) {
        const int value = tile->price / 2 + property->level * tile->price / 4;
        player->cash += value; property->mortgaged = true;
        emitEvent(events, QStringLiteral("industryPawned"), player->id,
                  {{QStringLiteral("tile"), tileIndex}, {QStringLiteral("amount"), value}});
    } else {
        const int cost = tile->price * 60 / 100 + property->level * tile->price / 4;
        if (player->cash < cost) return {false, QStringLiteral("赎回所需资金不足"), {}};
        player->cash -= cost; property->mortgaged = false;
        emitEvent(events, QStringLiteral("industryRedeemed"), player->id,
                  {{QStringLiteral("tile"), tileIndex}, {QStringLiteral("cost"), cost}});
    }
    return {true, {}, events};
}

CommandResult GameEngine::sellProperty(const GameCommand &command)
{
    if (m_state.phase != GamePhase::AwaitingDecision && m_state.phase != GamePhase::ForcedSettlement)
        return {false, QStringLiteral("当前不能出售产业"), {}};
    auto *player = m_state.activePlayer();
    const int tileIndex = command.arguments.value(QStringLiteral("tile"), -1).toInt();
    auto *property = m_state.propertyAt(tileIndex);
    const auto *tile = m_state.tileAt(tileIndex);
    if (!player || !property || !tile || property->ownerId != player->id)
        return {false, QStringLiteral("产业归属无效"), {}};
    const int value = tile->price / 2 + property->level * tile->price / 3;
    player->cash += value;
    property->ownerId = QUuid{}; property->level = 0; property->mortgaged = false;
    QList<GameEvent> events;
    emitEvent(events, QStringLiteral("industrySold"), player->id,
              {{QStringLiteral("tile"), tileIndex}, {QStringLiteral("amount"), value}});
    return {true, {}, events};
}

CommandResult GameEngine::useCard(const GameCommand &command)
{
    auto *player = m_state.activePlayer();
    const int slot = command.arguments.value(QStringLiteral("slot"), 0).toInt();
    if (!player || slot < 0 || slot >= player->strategyCards.size())
        return {false, QStringLiteral("筹策卡不可用"), {}};
    const int card = player->strategyCards.takeAt(slot);
    switch (card % 8) {
    case 0: player->movementBonus += 2; break;
    case 1: ++player->shieldCharges; break;
    case 2: player->cash += 800; break;
    case 3: player->energy = qMin(6, player->energy + 2); break;
    case 4: player->reputation = qMin(100, player->reputation + 7); break;
    case 5: player->culture = qMin(100, player->culture + 7); break;
    case 6: player->livelihood = qMin(100, player->livelihood + 7); break;
    case 7: player->rerolls = qMin(3, player->rerolls + 1); break;
    }
    QList<GameEvent> events;
    emitEvent(events, QStringLiteral("strategyUsed"), player->id, {{QStringLiteral("card"), card}});
    return {true, {}, events};
}

CommandResult GameEngine::useSkill(const GameCommand &)
{
    auto *player = m_state.activePlayer();
    if (!player || m_state.phase != GamePhase::AwaitingRoll || player->energy < 3 || player->skillCooldown > 0)
        return {false, QStringLiteral("人物才略尚未就绪"), {}};
    player->energy -= 3;
    player->skillCooldown = 4;
    switch (player->characterIndex) {
    case 0: player->upgradeDiscountPercent = 50; break;
    case 1: {
        auto it = std::find_if(m_state.properties.cbegin(), m_state.properties.cend(),
                               [&](const auto &p) { return p.ownerId == player->id; });
        if (it != m_state.properties.cend()) { player->boostedIndustryTile = it->tileIndex; player->boostedCollections = 2; }
        else player->cash += 600;
        break;
    }
    case 2: player->statusEffects.clear(); player->livelihood = qMin(100, player->livelihood + 12); break;
    case 3: player->rerolls = qMin(3, player->rerolls + 1); break;
    case 4:
        for (int i = 0; i < 3; ++i) player->strategyCards.append(int(m_random.bounded(48)));
        while (player->strategyCards.size() > 6) player->strategyCards.removeLast();
        player->culture = qMin(100, player->culture + 8);
        break;
    case 5: {
        int target = player->position;
        for (const auto &tile : m_state.tiles) if (tile.type == TileType::Transit && tile.index != player->position) { target = tile.index; break; }
        player->previousPosition = player->position; player->position = target; player->reputation = qMin(100, player->reputation + 5);
        break;
    }
    }
    QList<GameEvent> events;
    emitEvent(events, QStringLiteral("characterSkillUsed"), player->id,
              {{QStringLiteral("character"), player->characterIndex}});
    return {true, {}, events};
}

bool GameEngine::validateTradeAssets(const TradeOfferState &trade, QString *error) const
{
    const auto *proposer = playerById(trade.proposerId);
    const auto *recipient = playerById(trade.recipientId);
    if (!proposer || !recipient || proposer->bankrupt || recipient->bankrupt || proposer->cash < trade.offeredCash
        || recipient->cash < trade.requestedCash) {
        if (error) *error = QStringLiteral("交易双方或资金状态无效"); return false;
    }
    const auto owns = [&](const QUuid &owner, int tile) {
        if (tile < 0) return true;
        const auto *property = m_state.propertyAt(tile);
        return property && property->ownerId == owner;
    };
    if (!owns(proposer->id, trade.offeredTile) || !owns(recipient->id, trade.requestedTile)
        || (trade.offeredCardSlot >= 0 && trade.offeredCardSlot >= proposer->strategyCards.size())
        || (trade.requestedCardSlot >= 0 && trade.requestedCardSlot >= recipient->strategyCards.size())) {
        if (error) *error = QStringLiteral("交易资产已发生变化"); return false;
    }
    return true;
}

CommandResult GameEngine::proposeTrade(const GameCommand &command)
{
    auto *player = m_state.activePlayer();
    if (!player || m_state.phase != GamePhase::AwaitingDecision || player->tradedThisTurn)
        return {false, QStringLiteral("本回合不能再次发起交易"), {}};
    TradeOfferState trade;
    trade.active = true; trade.proposerId = player->id;
    trade.recipientId = QUuid(command.arguments.value(QStringLiteral("recipient")).toString());
    trade.offeredCash = qMax(0, command.arguments.value(QStringLiteral("offeredCash")).toInt());
    trade.requestedCash = qMax(0, command.arguments.value(QStringLiteral("requestedCash")).toInt());
    trade.offeredTile = command.arguments.value(QStringLiteral("offeredTile"), -1).toInt();
    trade.requestedTile = command.arguments.value(QStringLiteral("requestedTile"), -1).toInt();
    trade.offeredCardSlot = command.arguments.value(QStringLiteral("offeredCard"), -1).toInt();
    trade.requestedCardSlot = command.arguments.value(QStringLiteral("requestedCard"), -1).toInt();
    trade.deadlineMs = QDateTime::currentMSecsSinceEpoch() + 60000;
    QString error;
    if (trade.recipientId == player->id || !validateTradeAssets(trade, &error)) return {false, error, {}};
    m_state.tradeOffer = trade;
    m_state.phase = GamePhase::Trade;
    QList<GameEvent> events;
    emitEvent(events, QStringLiteral("tradeProposed"), player->id,
              {{QStringLiteral("recipient"), trade.recipientId.toString()}, {QStringLiteral("deadline"), trade.deadlineMs}});
    return {true, {}, events};
}

CommandResult GameEngine::respondTrade(const GameCommand &command)
{
    if (m_state.phase != GamePhase::Trade || !m_state.tradeOffer.active)
        return {false, QStringLiteral("当前没有待回应交易"), {}};
    const bool accept = command.arguments.value(QStringLiteral("accept")).toBool();
    QList<GameEvent> events;
    QString error;
    if (accept && validateTradeAssets(m_state.tradeOffer, &error)) {
        auto *a = playerById(m_state.tradeOffer.proposerId);
        auto *b = playerById(m_state.tradeOffer.recipientId);
        a->cash += m_state.tradeOffer.requestedCash - m_state.tradeOffer.offeredCash;
        b->cash += m_state.tradeOffer.offeredCash - m_state.tradeOffer.requestedCash;
        if (m_state.tradeOffer.offeredTile >= 0) m_state.propertyAt(m_state.tradeOffer.offeredTile)->ownerId = b->id;
        if (m_state.tradeOffer.requestedTile >= 0) m_state.propertyAt(m_state.tradeOffer.requestedTile)->ownerId = a->id;
        if (m_state.tradeOffer.offeredCardSlot >= 0) {
            const int card = a->strategyCards.takeAt(m_state.tradeOffer.offeredCardSlot); b->strategyCards.append(card);
        }
        if (m_state.tradeOffer.requestedCardSlot >= 0) {
            const int card = b->strategyCards.takeAt(m_state.tradeOffer.requestedCardSlot); a->strategyCards.append(card);
        }
        a->tradedThisTurn = true;
        a->reputation = qMin(100, a->reputation + 2);
        b->reputation = qMin(100, b->reputation + 2);
        emitEvent(events, QStringLiteral("tradeAccepted"), command.playerId);
    } else emitEvent(events, QStringLiteral("tradeDeclined"), command.playerId, {{QStringLiteral("reason"), error}});
    m_state.tradeOffer = {};
    m_state.phase = GamePhase::AwaitingDecision;
    return {true, {}, events};
}

CommandResult GameEngine::contributeCivic(const GameCommand &)
{
    auto *player = m_state.activePlayer();
    const auto *tile = player ? m_state.tileAt(player->position) : nullptr;
    if (!player || !tile || tile->type != TileType::Civic || m_state.phase != GamePhase::AwaitingDecision || player->cash < 800)
        return {false, QStringLiteral("当前不能投资公共项目"), {}};
    player->cash -= 800; player->livelihood = qMin(100, player->livelihood + 10);
    player->reputation = qMin(100, player->reputation + 3);
    QList<GameEvent> events;
    emitEvent(events, QStringLiteral("civicContributed"), player->id, {{QStringLiteral("amount"), 800}});
    return {true, {}, events};
}

QList<GameEvent> GameEngine::expireTimedPhase(qint64 nowMs)
{
    QList<GameEvent> events;
    if (m_state.phase == GamePhase::Auction && m_state.auctionDeadlineMs > 0 && nowMs >= m_state.auctionDeadlineMs)
        settleAuction(events);
    else if (m_state.phase == GamePhase::Trade && m_state.tradeOffer.deadlineMs > 0
             && nowMs >= m_state.tradeOffer.deadlineMs) {
        emitEvent(events, QStringLiteral("tradeExpired"), m_state.tradeOffer.recipientId);
        m_state.tradeOffer = {};
        m_state.phase = GamePhase::AwaitingDecision;
    }
    return events;
}

void GameEngine::advancePlayer(QList<GameEvent> &events)
{
    m_state.movingPlayerId = QUuid{};
    m_state.pendingMovePath.clear();
    m_state.pendingMoveIndex = 0;
    const int alive = std::count_if(m_state.players.cbegin(), m_state.players.cend(), [](const auto &p) { return !p.bankrupt; });
    if (alive <= 1 || m_state.round > m_state.maxRounds) {
        m_state.phase = GamePhase::Finished; emitEvent(events, QStringLiteral("gameFinished"), {}); return;
    }
    do {
        m_state.currentPlayer = (m_state.currentPlayer + 1) % m_state.players.size();
        if (m_state.currentPlayer == 0) {
            ++m_state.round;
            if (m_state.round > m_state.maxRounds) {
                m_state.phase = GamePhase::Finished; emitEvent(events, QStringLiteral("gameFinished"), {}); return;
            }
            for (auto &player : m_state.players) if (!player.bankrupt) player.cash += 260;
            if ((m_state.round - 1) % 10 == 0) {
                activateCityPulse(events);
            }
            for (auto &player : m_state.players) {
                if (player.bankrupt) continue;
                const int cadence = player.characterIndex == 3 ? 4 : 8;
                const int cap = player.characterIndex == 3 ? 3 : 2;
                if ((m_state.round - 1) % cadence == 0) player.rerolls = qMin(cap, player.rerolls + 1);
            }
        }
    } while (m_state.players.at(m_state.currentPlayer).bankrupt);
    auto *active = m_state.activePlayer();
    active->tradedThisTurn = false;
    active->energy = qMin(6, active->energy + 1);
    if (active->skillCooldown > 0) --active->skillCooldown;
    m_state.phase = GamePhase::AwaitingRoll;
    emitEvent(events, QStringLiteral("turnStarted"), active->id, {{QStringLiteral("round"), m_state.round}});
}

void GameEngine::activateCityPulse(QList<GameEvent> &events)
{
    const auto pulses = CityContent::cityPulseEvents();
    m_state.activePulse = ((m_state.round - 1) / 10 - 1) % pulses.size();
    m_state.rentModifierPercent = 100; m_state.buildingCostPercent = 100; m_state.purchaseRebatePercent = 0;
    switch (m_state.activePulse) {
    case 0: for (auto &p : m_state.players) p.culture = qMin(100, p.culture + 5); break;
    case 1: for (auto &p : m_state.players) p.livelihood = qMin(100, p.livelihood + 5); break;
    case 2: for (auto &p : m_state.players) p.movementBonus += 1; break;
    case 3: m_state.buildingCostPercent = 80; break;
    case 4: for (auto &p : m_state.players) p.reputation = qMin(100, p.reputation + 4); break;
    case 5: m_state.rentModifierPercent = 120; break;
    case 6: m_state.purchaseRebatePercent = 12; break;
    case 7: for (auto &p : m_state.players) p.cash += 500; break;
    case 8: m_state.rentModifierPercent = 110; m_state.purchaseRebatePercent = 8; break;
    case 9: for (auto &p : m_state.players) p.culture = qMin(100, p.culture + 7); break;
    case 10: for (auto &p : m_state.players) p.livelihood = qMin(100, p.livelihood + 7); break;
    case 11: m_state.buildingCostPercent = 75; break;
    }
    emitEvent(events, QStringLiteral("seasonalEvent"), {},
              {{QStringLiteral("index"), m_state.activePulse}, {QStringLiteral("name"), pulses.at(m_state.activePulse)}});
}

void GameEngine::emitEvent(QList<GameEvent> &events, const QString &type, const QUuid &playerId,
                           const QVariantMap &data)
{
    GameEvent event{++m_state.sequence, type, playerId, data};
    events.append(event); m_state.eventLog.append(event);
}

int GameEngine::finalScore(const QUuid &playerId) const
{
    const auto netWorth = [&](const PlayerState &player) {
        int value = player.cash;
        for (const auto &property : m_state.properties) if (property.ownerId == player.id) {
            const auto *tile = m_state.tileAt(property.tileIndex);
            if (tile) value += tile->price * (property.mortgaged ? 40 : 70) / 100 + property.level * tile->price / 3;
        }
        return qMax(0, value);
    };
    const auto *player = playerById(playerId);
    if (!player || player->bankrupt) return 0;
    int highest = 1;
    for (const auto &candidate : m_state.players) if (!candidate.bankrupt) highest = qMax(highest, netWorth(candidate));
    const int wealth = qBound(0, qRound(100.0 * netWorth(*player) / highest), 100);
    return wealth + player->reputation + player->culture + player->livelihood;
}

void GameEngine::restore(GameState state)
{
    m_state = std::move(state);
    m_random.seed(m_state.randomSeed ^ m_state.sequence);
    m_processedCommands.clear();
}

bool GameEngine::setAiControlled(const QUuid &playerId, bool controlled)
{
    auto *player = playerById(playerId);
    if (!player || player->aiControlled == controlled) return false;
    player->aiControlled = controlled;
    QList<GameEvent> ignored;
    emitEvent(ignored, controlled ? QStringLiteral("aiTakeover") : QStringLiteral("playerReconnected"), playerId);
    return true;
}

} // namespace neon

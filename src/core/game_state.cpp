#include "core/game_state.h"

#include "core/city_content.h"

#include <QCborArray>
#include <QCryptographicHash>

namespace neon {
namespace {

QCborMap playerToCbor(const PlayerState &player)
{
    QCborArray cards;
    for (int card : player.strategyCards) cards.append(card);
    return {{QStringLiteral("id"), player.id.toString(QUuid::WithoutBraces)},
            {QStringLiteral("name"), player.name},
            {QStringLiteral("color"), player.color.name(QColor::HexArgb)},
            {QStringLiteral("character"), player.characterIndex},
            {QStringLiteral("position"), player.position},
            {QStringLiteral("cash"), player.cash},
            {QStringLiteral("energy"), player.energy},
            {QStringLiteral("movementBonus"), player.movementBonus},
            {QStringLiteral("shieldCharges"), player.shieldCharges},
            {QStringLiteral("skillCooldown"), player.skillCooldown},
            {QStringLiteral("cards"), cards},
            {QStringLiteral("missionScore"), player.missionScore},
            {QStringLiteral("bankrupt"), player.bankrupt},
            {QStringLiteral("ai"), player.aiControlled}};
}

PlayerState playerFromCbor(const QCborMap &map)
{
    PlayerState player;
    player.id = QUuid(map.value(QStringLiteral("id")).toString());
    player.name = map.value(QStringLiteral("name")).toString();
    player.color = QColor(map.value(QStringLiteral("color")).toString());
    player.characterIndex = int(map.value(QStringLiteral("character")).toInteger());
    player.position = int(map.value(QStringLiteral("position")).toInteger());
    player.cash = int(map.value(QStringLiteral("cash")).toInteger());
    player.energy = int(map.value(QStringLiteral("energy")).toInteger());
    player.movementBonus = int(map.value(QStringLiteral("movementBonus")).toInteger());
    player.shieldCharges = int(map.value(QStringLiteral("shieldCharges")).toInteger());
    player.skillCooldown = int(map.value(QStringLiteral("skillCooldown")).toInteger());
    for (const auto &card : map.value(QStringLiteral("cards")).toArray()) player.strategyCards.append(int(card.toInteger()));
    player.missionScore = int(map.value(QStringLiteral("missionScore")).toInteger());
    player.bankrupt = map.value(QStringLiteral("bankrupt")).toBool();
    player.aiControlled = map.value(QStringLiteral("ai")).toBool();
    return player;
}

} // namespace

const PlayerState *GameState::activePlayer() const
{
    return currentPlayer >= 0 && currentPlayer < players.size() ? &players.at(currentPlayer) : nullptr;
}

PlayerState *GameState::activePlayer()
{
    return currentPlayer >= 0 && currentPlayer < players.size() ? &players[currentPlayer] : nullptr;
}

const TileDefinition *GameState::tileAt(int index) const
{
    return index >= 0 && index < tiles.size() ? &tiles.at(index) : nullptr;
}

PropertyState *GameState::propertyAt(int tileIndex)
{
    for (auto &property : properties) if (property.tileIndex == tileIndex) return &property;
    return nullptr;
}

const PropertyState *GameState::propertyAt(int tileIndex) const
{
    for (const auto &property : properties) if (property.tileIndex == tileIndex) return &property;
    return nullptr;
}

quint64 GameState::stableHash() const
{
    const auto digest = QCryptographicHash::hash(QCborValue(toCbor(false)).toCbor(), QCryptographicHash::Sha256);
    quint64 result = 0;
    for (int i = 0; i < qMin(8, digest.size()); ++i) result = (result << 8) | quint8(digest.at(i));
    return result;
}

QCborMap GameState::toCbor(bool includeLog) const
{
    QCborArray playerArray;
    for (const auto &player : players) playerArray.append(playerToCbor(player));

    QCborArray propertyArray;
    for (const auto &property : properties) {
        propertyArray.append(QCborMap{{QStringLiteral("tile"), property.tileIndex},
            {QStringLiteral("owner"), property.ownerId.toString(QUuid::WithoutBraces)},
            {QStringLiteral("level"), property.level}, {QStringLiteral("mortgaged"), property.mortgaged}});
    }

    QCborMap map{{QStringLiteral("matchId"), matchId.toString(QUuid::WithoutBraces)},
        {QStringLiteral("rulesVersion"), qint64(rulesVersion)}, {QStringLiteral("sequence"), qint64(sequence)},
        {QStringLiteral("seed"), qint64(randomSeed)}, {QStringLiteral("round"), round},
        {QStringLiteral("maxRounds"), maxRounds}, {QStringLiteral("currentPlayer"), currentPlayer},
        {QStringLiteral("lastDice"), lastDice}, {QStringLiteral("phase"), int(phase)},
        {QStringLiteral("activePulse"), activePulse}, {QStringLiteral("rentModifier"), rentModifierPercent},
        {QStringLiteral("buildingCost"), buildingCostPercent}, {QStringLiteral("purchaseRebate"), purchaseRebatePercent},
        {QStringLiteral("players"), playerArray}, {QStringLiteral("properties"), propertyArray}};

    if (includeLog) {
        QCborArray log;
        for (const auto &event : eventLog) {
            log.append(QCborMap{{QStringLiteral("sequence"), qint64(event.sequence)},
                {QStringLiteral("type"), event.type},
                {QStringLiteral("player"), event.playerId.toString(QUuid::WithoutBraces)},
                {QStringLiteral("data"), QCborValue::fromVariant(event.data)}});
        }
        map.insert(QStringLiteral("log"), log);
    }
    return map;
}

GameState GameState::fromCbor(const QCborMap &map, QString *error)
{
    GameState state;
    state.matchId = QUuid(map.value(QStringLiteral("matchId")).toString());
    state.rulesVersion = quint32(map.value(QStringLiteral("rulesVersion")).toInteger());
    if (state.rulesVersion != 1 || state.matchId.isNull()) {
        if (error) *error = QStringLiteral("Unsupported or invalid saved game");
        return {};
    }
    state.sequence = quint64(map.value(QStringLiteral("sequence")).toInteger());
    state.randomSeed = quint64(map.value(QStringLiteral("seed")).toInteger());
    state.round = int(map.value(QStringLiteral("round")).toInteger());
    state.maxRounds = int(map.value(QStringLiteral("maxRounds")).toInteger());
    state.currentPlayer = int(map.value(QStringLiteral("currentPlayer")).toInteger());
    state.lastDice = int(map.value(QStringLiteral("lastDice")).toInteger());
    state.activePulse = int(map.value(QStringLiteral("activePulse")).toInteger(-1));
    state.rentModifierPercent = int(map.value(QStringLiteral("rentModifier")).toInteger(100));
    state.buildingCostPercent = int(map.value(QStringLiteral("buildingCost")).toInteger(100));
    state.purchaseRebatePercent = int(map.value(QStringLiteral("purchaseRebate")).toInteger());
    state.phase = GamePhase(map.value(QStringLiteral("phase")).toInteger());
    for (const auto &value : map.value(QStringLiteral("players")).toArray()) state.players.append(playerFromCbor(value.toMap()));
    for (const auto &value : map.value(QStringLiteral("properties")).toArray()) {
        const auto item = value.toMap();
        state.properties.append({int(item.value(QStringLiteral("tile")).toInteger()),
            QUuid(item.value(QStringLiteral("owner")).toString()), int(item.value(QStringLiteral("level")).toInteger()),
            item.value(QStringLiteral("mortgaged")).toBool()});
    }
    state.tiles = CityContent::createNeonCityMap();
    for (const auto &value : map.value(QStringLiteral("log")).toArray()) {
        const auto item = value.toMap();
        state.eventLog.append({quint64(item.value(QStringLiteral("sequence")).toInteger()),
            item.value(QStringLiteral("type")).toString(), QUuid(item.value(QStringLiteral("player")).toString()),
            item.value(QStringLiteral("data")).toVariant().toMap()});
    }
    if (error) error->clear();
    return state;
}

} // namespace neon

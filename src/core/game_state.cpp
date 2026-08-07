#include "core/game_state.h"

#include "core/city_content.h"

#include <QCborArray>
#include <QCryptographicHash>

namespace neon {
namespace {

QCborArray intArray(const QList<int> &values)
{
    QCborArray array;
    for (int value : values) array.append(value);
    return array;
}

QList<int> intList(const QCborValue &value)
{
    QList<int> result;
    for (const auto &entry : value.toArray()) result.append(int(entry.toInteger()));
    return result;
}

QCborMap playerToCbor(const PlayerState &player)
{
    QCborArray cards = intArray(player.strategyCards);
    QCborArray commissions = intArray(player.commissions);
    QCborArray effects;
    for (const auto &effect : player.statusEffects) effects.append(effect);
    return {{QStringLiteral("id"), player.id.toString(QUuid::WithoutBraces)},
            {QStringLiteral("name"), player.name}, {QStringLiteral("color"), player.color.name(QColor::HexArgb)},
            {QStringLiteral("character"), player.characterIndex}, {QStringLiteral("position"), player.position},
            {QStringLiteral("previousPosition"), player.previousPosition}, {QStringLiteral("cash"), player.cash},
            {QStringLiteral("reputation"), player.reputation}, {QStringLiteral("culture"), player.culture},
            {QStringLiteral("livelihood"), player.livelihood}, {QStringLiteral("energy"), player.energy},
            {QStringLiteral("rerolls"), player.rerolls}, {QStringLiteral("movementBonus"), player.movementBonus},
            {QStringLiteral("shieldCharges"), player.shieldCharges},
            {QStringLiteral("skillCooldown"), player.skillCooldown},
            {QStringLiteral("upgradeDiscount"), player.upgradeDiscountPercent},
            {QStringLiteral("boostedIndustry"), player.boostedIndustryTile},
            {QStringLiteral("boostedCollections"), player.boostedCollections},
            {QStringLiteral("cards"), cards}, {QStringLiteral("commissions"), commissions},
            {QStringLiteral("effects"), effects}, {QStringLiteral("bankrupt"), player.bankrupt},
            {QStringLiteral("ai"), player.aiControlled}, {QStringLiteral("traded"), player.tradedThisTurn}};
}

PlayerState playerFromCbor(const QCborMap &map)
{
    PlayerState player;
    player.id = QUuid(map.value(QStringLiteral("id")).toString());
    player.name = map.value(QStringLiteral("name")).toString();
    player.color = QColor(map.value(QStringLiteral("color")).toString());
    player.characterIndex = int(map.value(QStringLiteral("character")).toInteger());
    player.position = int(map.value(QStringLiteral("position")).toInteger());
    player.previousPosition = int(map.value(QStringLiteral("previousPosition")).toInteger(-1));
    player.cash = int(map.value(QStringLiteral("cash")).toInteger());
    player.reputation = int(map.value(QStringLiteral("reputation")).toInteger());
    player.culture = int(map.value(QStringLiteral("culture")).toInteger());
    player.livelihood = int(map.value(QStringLiteral("livelihood")).toInteger());
    player.energy = int(map.value(QStringLiteral("energy")).toInteger());
    player.rerolls = int(map.value(QStringLiteral("rerolls")).toInteger());
    player.movementBonus = int(map.value(QStringLiteral("movementBonus")).toInteger());
    player.shieldCharges = int(map.value(QStringLiteral("shieldCharges")).toInteger());
    player.skillCooldown = int(map.value(QStringLiteral("skillCooldown")).toInteger());
    player.upgradeDiscountPercent = int(map.value(QStringLiteral("upgradeDiscount")).toInteger());
    player.boostedIndustryTile = int(map.value(QStringLiteral("boostedIndustry")).toInteger(-1));
    player.boostedCollections = int(map.value(QStringLiteral("boostedCollections")).toInteger());
    player.strategyCards = intList(map.value(QStringLiteral("cards")));
    player.commissions = intList(map.value(QStringLiteral("commissions")));
    for (const auto &effect : map.value(QStringLiteral("effects")).toArray()) player.statusEffects.append(effect.toString());
    player.bankrupt = map.value(QStringLiteral("bankrupt")).toBool();
    player.aiControlled = map.value(QStringLiteral("ai")).toBool();
    player.tradedThisTurn = map.value(QStringLiteral("traded")).toBool();
    return player;
}

QCborMap tradeToCbor(const TradeOfferState &trade)
{
    return {{QStringLiteral("active"), trade.active},
        {QStringLiteral("proposer"), trade.proposerId.toString(QUuid::WithoutBraces)},
        {QStringLiteral("recipient"), trade.recipientId.toString(QUuid::WithoutBraces)},
        {QStringLiteral("offeredCash"), trade.offeredCash}, {QStringLiteral("requestedCash"), trade.requestedCash},
        {QStringLiteral("offeredTile"), trade.offeredTile}, {QStringLiteral("requestedTile"), trade.requestedTile},
        {QStringLiteral("offeredCard"), trade.offeredCardSlot}, {QStringLiteral("requestedCard"), trade.requestedCardSlot},
        {QStringLiteral("deadline"), trade.deadlineMs}};
}

TradeOfferState tradeFromCbor(const QCborMap &map)
{
    TradeOfferState trade;
    trade.active = map.value(QStringLiteral("active")).toBool();
    trade.proposerId = QUuid(map.value(QStringLiteral("proposer")).toString());
    trade.recipientId = QUuid(map.value(QStringLiteral("recipient")).toString());
    trade.offeredCash = int(map.value(QStringLiteral("offeredCash")).toInteger());
    trade.requestedCash = int(map.value(QStringLiteral("requestedCash")).toInteger());
    trade.offeredTile = int(map.value(QStringLiteral("offeredTile")).toInteger(-1));
    trade.requestedTile = int(map.value(QStringLiteral("requestedTile")).toInteger(-1));
    trade.offeredCardSlot = int(map.value(QStringLiteral("offeredCard")).toInteger(-1));
    trade.requestedCardSlot = int(map.value(QStringLiteral("requestedCard")).toInteger(-1));
    trade.deadlineMs = map.value(QStringLiteral("deadline")).toInteger();
    return trade;
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
    QCborArray routes;
    for (const auto &route : routeOptions) routes.append(intArray(route));
    QCborArray passed;
    for (const auto &id : auctionPassedPlayers) passed.append(id.toString(QUuid::WithoutBraces));

    QCborMap map{{QStringLiteral("matchId"), matchId.toString(QUuid::WithoutBraces)},
        {QStringLiteral("rulesVersion"), qint64(rulesVersion)}, {QStringLiteral("contentVersion"), qint64(contentVersion)},
        {QStringLiteral("contentHash"), QString::fromLatin1(contentHash)}, {QStringLiteral("sequence"), qint64(sequence)},
        {QStringLiteral("seed"), qint64(randomSeed)}, {QStringLiteral("round"), round},
        {QStringLiteral("maxRounds"), maxRounds}, {QStringLiteral("currentPlayer"), currentPlayer},
        {QStringLiteral("lastDice"), lastDice}, {QStringLiteral("pendingDice"), pendingDice},
        {QStringLiteral("routes"), routes}, {QStringLiteral("phase"), int(phase)},
        {QStringLiteral("activePulse"), activePulse}, {QStringLiteral("rentModifier"), rentModifierPercent},
        {QStringLiteral("buildingCost"), buildingCostPercent}, {QStringLiteral("purchaseRebate"), purchaseRebatePercent},
        {QStringLiteral("auctionTile"), auctionTile}, {QStringLiteral("auctionBid"), auctionHighBid},
        {QStringLiteral("auctionBidder"), auctionHighBidder.toString(QUuid::WithoutBraces)},
        {QStringLiteral("auctionPassed"), passed}, {QStringLiteral("auctionDeadline"), auctionDeadlineMs},
        {QStringLiteral("trade"), tradeToCbor(tradeOffer)}, {QStringLiteral("players"), playerArray},
        {QStringLiteral("properties"), propertyArray}};
    if (includeLog) {
        QCborArray log;
        for (const auto &event : eventLog) {
            log.append(QCborMap{{QStringLiteral("sequence"), qint64(event.sequence)}, {QStringLiteral("type"), event.type},
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
    state.contentVersion = quint32(map.value(QStringLiteral("contentVersion")).toInteger());
    state.contentHash = map.value(QStringLiteral("contentHash")).toString().toLatin1();
    const QByteArray expectedContentHash = CityContent::contentHash();
    if (state.rulesVersion != 2 || state.contentVersion != CityContent::ContentVersion
        || state.contentHash != expectedContentHash || state.matchId.isNull()) {
        if (error) *error = QStringLiteral("存档规则或内容版本与《盛世百业》v0.2.0不兼容");
        return {};
    }
    state.sequence = quint64(map.value(QStringLiteral("sequence")).toInteger());
    state.randomSeed = quint64(map.value(QStringLiteral("seed")).toInteger());
    state.round = int(map.value(QStringLiteral("round")).toInteger());
    state.maxRounds = int(map.value(QStringLiteral("maxRounds")).toInteger());
    state.currentPlayer = int(map.value(QStringLiteral("currentPlayer")).toInteger());
    state.lastDice = int(map.value(QStringLiteral("lastDice")).toInteger());
    state.pendingDice = int(map.value(QStringLiteral("pendingDice")).toInteger());
    for (const auto &route : map.value(QStringLiteral("routes")).toArray()) state.routeOptions.append(intList(route));
    state.activePulse = int(map.value(QStringLiteral("activePulse")).toInteger(-1));
    state.rentModifierPercent = int(map.value(QStringLiteral("rentModifier")).toInteger(100));
    state.buildingCostPercent = int(map.value(QStringLiteral("buildingCost")).toInteger(100));
    state.purchaseRebatePercent = int(map.value(QStringLiteral("purchaseRebate")).toInteger());
    state.phase = GamePhase(map.value(QStringLiteral("phase")).toInteger());
    state.auctionTile = int(map.value(QStringLiteral("auctionTile")).toInteger(-1));
    state.auctionHighBid = int(map.value(QStringLiteral("auctionBid")).toInteger());
    state.auctionHighBidder = QUuid(map.value(QStringLiteral("auctionBidder")).toString());
    for (const auto &id : map.value(QStringLiteral("auctionPassed")).toArray()) state.auctionPassedPlayers.append(QUuid(id.toString()));
    state.auctionDeadlineMs = map.value(QStringLiteral("auctionDeadline")).toInteger();
    state.tradeOffer = tradeFromCbor(map.value(QStringLiteral("trade")).toMap());
    for (const auto &value : map.value(QStringLiteral("players")).toArray()) state.players.append(playerFromCbor(value.toMap()));
    for (const auto &value : map.value(QStringLiteral("properties")).toArray()) {
        const auto item = value.toMap();
        state.properties.append({int(item.value(QStringLiteral("tile")).toInteger()),
            QUuid(item.value(QStringLiteral("owner")).toString()), int(item.value(QStringLiteral("level")).toInteger()),
            item.value(QStringLiteral("mortgaged")).toBool()});
    }
    state.tiles = CityContent::createProsperousCityMap();
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

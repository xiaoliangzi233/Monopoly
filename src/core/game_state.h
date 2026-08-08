#pragma once

#include "core/game_types.h"

#include <QCborMap>

namespace neon {

class GameState final {
public:
    QUuid matchId;
    quint32 rulesVersion = 2;
    quint32 contentVersion = 3;
    QByteArray contentHash;
    quint64 sequence = 0;
    quint64 randomSeed = 0;
    int round = 1;
    int maxRounds = 120;
    int currentPlayer = 0;
    int lastDice = 0;
    int pendingDice = 0;
    QList<QList<int>> routeOptions;
    QUuid movingPlayerId;
    QList<int> pendingMovePath;
    int pendingMoveIndex = 0;
    quint64 movementSerial = 0;
    int activePulse = -1;
    int rentModifierPercent = 100;
    int buildingCostPercent = 100;
    int purchaseRebatePercent = 0;
    GamePhase phase = GamePhase::Waiting;
    int auctionTile = -1;
    int auctionHighBid = 0;
    QUuid auctionHighBidder;
    QList<QUuid> auctionPassedPlayers;
    qint64 auctionDeadlineMs = 0;
    TradeOfferState tradeOffer;
    QList<TileDefinition> tiles;
    QList<PropertyState> properties;
    QList<PlayerState> players;
    QList<GameEvent> eventLog;

    [[nodiscard]] const PlayerState *activePlayer() const;
    [[nodiscard]] PlayerState *activePlayer();
    [[nodiscard]] const TileDefinition *tileAt(int index) const;
    [[nodiscard]] PropertyState *propertyAt(int tileIndex);
    [[nodiscard]] const PropertyState *propertyAt(int tileIndex) const;
    [[nodiscard]] quint64 stableHash() const;
    [[nodiscard]] QCborMap toCbor(bool includeLog = true) const;
    static GameState fromCbor(const QCborMap &map, QString *error = nullptr);
};

} // namespace neon

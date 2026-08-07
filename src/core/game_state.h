#pragma once

#include "core/game_types.h"

#include <QCborMap>

namespace neon {

class GameState final {
public:
    QUuid matchId;
    quint32 rulesVersion = 1;
    quint64 sequence = 0;
    quint64 randomSeed = 0;
    int round = 1;
    int maxRounds = 24;
    int currentPlayer = 0;
    int lastDice = 0;
    int activePulse = -1;
    int rentModifierPercent = 100;
    int buildingCostPercent = 100;
    int purchaseRebatePercent = 0;
    GamePhase phase = GamePhase::Waiting;
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

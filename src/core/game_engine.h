#pragma once

#include "core/game_state.h"

#include <QRandomGenerator>
#include <QSet>

namespace neon {

class GameEngine final {
public:
    GameEngine();

    bool createGame(const QStringList &playerNames, int aiPlayers = 0, int maxRounds = 32,
                    quint64 seed = 0, QString *error = nullptr);
    CommandResult apply(const GameCommand &command);
    QList<GameEvent> expireTimedPhase(qint64 nowMs);
    void restore(GameState state);
    bool setAiControlled(const QUuid &playerId, bool controlled);

    [[nodiscard]] const GameState &state() const { return m_state; }
    [[nodiscard]] int finalScore(const QUuid &playerId) const;

private:
    CommandResult roll(const GameCommand &command);
    CommandResult reroll(const GameCommand &command);
    CommandResult chooseRoute(const GameCommand &command);
    CommandResult buy(const GameCommand &command);
    CommandResult upgrade(const GameCommand &command);
    CommandResult endTurn(const GameCommand &command);
    CommandResult mortgage(const GameCommand &command);
    CommandResult useCard(const GameCommand &command);
    CommandResult useSkill(const GameCommand &command);
    CommandResult placeBid(const GameCommand &command);
    CommandResult passAuction(const GameCommand &command);
    CommandResult proposeTrade(const GameCommand &command);
    CommandResult respondTrade(const GameCommand &command);
    CommandResult sellProperty(const GameCommand &command);
    CommandResult contributeCivic(const GameCommand &command);
    QList<QList<int>> enumerateRoutes(const PlayerState &player, int steps) const;
    void moveAlongRoute(PlayerState &player, const QList<int> &route, QList<GameEvent> &events);
    void startAuction(int tileIndex, QList<GameEvent> &events);
    void settleAuction(QList<GameEvent> &events);
    bool validateTradeAssets(const TradeOfferState &trade, QString *error) const;
    bool ensureFunds(PlayerState &player, int amount, QList<GameEvent> &events);
    void resolveLanding(PlayerState &player, QList<GameEvent> &events);
    void emitEvent(QList<GameEvent> &events, const QString &type, const QUuid &playerId,
                   const QVariantMap &data = {});
    void advancePlayer(QList<GameEvent> &events);
    void activateCityPulse(QList<GameEvent> &events);
    bool validateCommon(const GameCommand &command, QString *error) const;
    PlayerState *playerById(const QUuid &id);
    const PlayerState *playerById(const QUuid &id) const;

    GameState m_state;
    QRandomGenerator64 m_random;
    QSet<quint64> m_processedCommands;
};

} // namespace neon

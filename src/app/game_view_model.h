#pragma once

#include "core/ai_player.h"
#include "core/save_manager.h"
#include "network/game_client.h"
#include "network/host_server.h"
#include "network/lan_discovery.h"

#include <QObject>
#include <QTimer>
#include <QVariantList>

namespace neon {

class GameViewModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentPlayerName READ currentPlayerName NOTIFY stateChanged)
    Q_PROPERTY(QString currentPlayerColor READ currentPlayerColor NOTIFY stateChanged)
    Q_PROPERTY(int currentCash READ currentCash NOTIFY stateChanged)
    Q_PROPERTY(int currentRound READ currentRound NOTIFY stateChanged)
    Q_PROPERTY(int maxRounds READ maxRounds NOTIFY stateChanged)
    Q_PROPERTY(int lastDice READ lastDice NOTIFY stateChanged)
    Q_PROPERTY(int currentEnergy READ currentEnergy NOTIFY stateChanged)
    Q_PROPERTY(int currentCardCount READ currentCardCount NOTIFY stateChanged)
    Q_PROPERTY(QString firstCardName READ firstCardName NOTIFY stateChanged)
    Q_PROPERTY(QString tileName READ tileName NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(QString networkStatus READ networkStatus NOTIFY networkStatusChanged)
    Q_PROPERTY(bool canRoll READ canRoll NOTIFY stateChanged)
    Q_PROPERTY(bool canBuy READ canBuy NOTIFY stateChanged)
    Q_PROPERTY(bool canUpgrade READ canUpgrade NOTIFY stateChanged)
    Q_PROPERTY(bool canEndTurn READ canEndTurn NOTIFY stateChanged)
    Q_PROPERTY(bool canUseSkill READ canUseSkill NOTIFY stateChanged)
    Q_PROPERTY(bool canUseCard READ canUseCard NOTIFY stateChanged)
    Q_PROPERTY(bool canMortgage READ canMortgage NOTIFY stateChanged)
    Q_PROPERTY(bool aiThinking READ aiThinking NOTIFY stateChanged)
    Q_PROPERTY(QVariantList players READ players NOTIFY stateChanged)
    Q_PROPERTY(QStringList eventLog READ eventLog NOTIFY stateChanged)

public:
    explicit GameViewModel(QObject *parent = nullptr);

    [[nodiscard]] const GameState &state() const { return m_engine.state(); }
    [[nodiscard]] QString currentPlayerName() const;
    [[nodiscard]] QString currentPlayerColor() const;
    [[nodiscard]] int currentCash() const;
    [[nodiscard]] int currentRound() const { return m_engine.state().round; }
    [[nodiscard]] int maxRounds() const { return m_engine.state().maxRounds; }
    [[nodiscard]] int lastDice() const { return m_engine.state().lastDice; }
    [[nodiscard]] int currentEnergy() const;
    [[nodiscard]] int currentCardCount() const;
    [[nodiscard]] QString firstCardName() const;
    [[nodiscard]] QString tileName() const;
    [[nodiscard]] QString statusText() const;
    [[nodiscard]] QString networkStatus() const { return m_networkStatus; }
    [[nodiscard]] bool canRoll() const;
    [[nodiscard]] bool canBuy() const;
    [[nodiscard]] bool canUpgrade() const;
    [[nodiscard]] bool canEndTurn() const;
    [[nodiscard]] bool canUseSkill() const;
    [[nodiscard]] bool canUseCard() const;
    [[nodiscard]] bool canMortgage() const;
    [[nodiscard]] bool aiThinking() const;
    [[nodiscard]] QVariantList players() const;
    [[nodiscard]] QStringList eventLog() const;

    Q_INVOKABLE void newGame(int totalPlayers = 4, int aiPlayers = 3, int rounds = 24);
    Q_INVOKABLE void rollDice();
    Q_INVOKABLE void buyCurrentProperty();
    Q_INVOKABLE void upgradeCurrentProperty();
    Q_INVOKABLE void endTurn();
    Q_INVOKABLE void useSkill();
    Q_INVOKABLE void useFirstCard();
    Q_INVOKABLE void mortgageCurrentProperty();
    Q_INVOKABLE bool saveGame();
    Q_INVOKABLE bool loadGame();
    Q_INVOKABLE void hostGame(int totalPlayers = 4, int aiPlayers = 2, quint16 port = net::DefaultPort);
    Q_INVOKABLE void joinGame(const QString &address, quint16 port = net::DefaultPort);
    Q_INVOKABLE QString tileDescription(int index) const;

signals:
    void stateChanged();
    void networkStatusChanged();
    void toastRequested(QString message);

private:
    void submit(CommandType type, const QVariantMap &arguments = {});
    void scheduleAi();
    void runAiStep();
    void setNetworkStatus(const QString &status);
    void autoSave();
    [[nodiscard]] QString quickSavePath() const;
    [[nodiscard]] bool localCanControlActivePlayer() const;

    GameEngine m_engine;
    AiPlayer m_ai{AiPlayer::Difficulty::Standard};
    QTimer m_aiTimer;
    quint64 m_nextCommandId = 0;
    QString m_networkStatus = QStringLiteral("本地模式");
    std::unique_ptr<net::HostServer> m_host;
    net::GameClient m_client;
    net::LanDiscovery m_discovery;
    bool m_clientMode = false;
    QUuid m_localPlayerId;
};

} // namespace neon

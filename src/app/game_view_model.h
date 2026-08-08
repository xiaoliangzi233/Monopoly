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
    Q_PROPERTY(int currentPosition READ currentPosition NOTIFY stateChanged)
    Q_PROPERTY(int currentCash READ currentCash NOTIFY stateChanged)
    Q_PROPERTY(int currentReputation READ currentReputation NOTIFY stateChanged)
    Q_PROPERTY(int currentCulture READ currentCulture NOTIFY stateChanged)
    Q_PROPERTY(int currentLivelihood READ currentLivelihood NOTIFY stateChanged)
    Q_PROPERTY(int currentProsperity READ currentProsperity NOTIFY stateChanged)
    Q_PROPERTY(int currentRound READ currentRound NOTIFY stateChanged)
    Q_PROPERTY(int maxRounds READ maxRounds NOTIFY stateChanged)
    Q_PROPERTY(int lastDice READ lastDice NOTIFY stateChanged)
    Q_PROPERTY(int currentEnergy READ currentEnergy NOTIFY stateChanged)
    Q_PROPERTY(QString firstCardName READ firstCardName NOTIFY stateChanged)
    Q_PROPERTY(QString tileName READ tileName NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(QString networkStatus READ networkStatus NOTIFY networkStatusChanged)
    Q_PROPERTY(bool canRoll READ canRoll NOTIFY stateChanged)
    Q_PROPERTY(bool canReroll READ canReroll NOTIFY stateChanged)
    Q_PROPERTY(bool canBuy READ canBuy NOTIFY stateChanged)
    Q_PROPERTY(bool canUpgrade READ canUpgrade NOTIFY stateChanged)
    Q_PROPERTY(bool canEndTurn READ canEndTurn NOTIFY stateChanged)
    Q_PROPERTY(bool canUseSkill READ canUseSkill NOTIFY stateChanged)
    Q_PROPERTY(bool canUseCard READ canUseCard NOTIFY stateChanged)
    Q_PROPERTY(bool canMortgage READ canMortgage NOTIFY stateChanged)
    Q_PROPERTY(bool canContribute READ canContribute NOTIFY stateChanged)
    Q_PROPERTY(bool canBidAuction READ canBidAuction NOTIFY stateChanged)
    Q_PROPERTY(bool tradePending READ tradePending NOTIFY stateChanged)
    Q_PROPERTY(bool aiThinking READ aiThinking NOTIFY stateChanged)
    Q_PROPERTY(bool presentationBusy READ presentationBusy NOTIFY stateChanged)
    Q_PROPERTY(bool diceAnimating READ diceAnimating NOTIFY stateChanged)
    Q_PROPERTY(int diceValue READ diceValue NOTIFY stateChanged)
    Q_PROPERTY(QString thinkingText READ thinkingText NOTIFY stateChanged)
    Q_PROPERTY(bool routeSelectionVisible READ routeSelectionVisible NOTIFY stateChanged)
    Q_PROPERTY(int auctionBid READ auctionBid NOTIFY stateChanged)
    Q_PROPERTY(int auctionSeconds READ auctionSeconds NOTIFY countdownChanged)
    Q_PROPERTY(QString auctionTileName READ auctionTileName NOTIFY stateChanged)
    Q_PROPERTY(QVariantList routeOptions READ routeOptions NOTIFY stateChanged)
    Q_PROPERTY(QVariantList players READ players NOTIFY stateChanged)
    Q_PROPERTY(QVariantList characters READ characters CONSTANT)
    Q_PROPERTY(QStringList eventLog READ eventLog NOTIFY stateChanged)

public:
    explicit GameViewModel(QObject *parent = nullptr);
    const GameState &state() const { return m_engine.state(); }
    QString currentPlayerName() const;
    QString currentPlayerColor() const;
    int currentPosition() const;
    int currentCash() const;
    int currentReputation() const;
    int currentCulture() const;
    int currentLivelihood() const;
    int currentProsperity() const;
    int currentRound() const { return m_engine.state().round; }
    int maxRounds() const { return m_engine.state().maxRounds; }
    int lastDice() const { return m_engine.state().lastDice; }
    int currentEnergy() const;
    QString firstCardName() const;
    QString tileName() const;
    QString statusText() const;
    QString networkStatus() const { return m_networkStatus; }
    bool canRoll() const;
    bool canReroll() const;
    bool canBuy() const;
    bool canUpgrade() const;
    bool canEndTurn() const;
    bool canUseSkill() const;
    bool canUseCard() const;
    bool canMortgage() const;
    bool canContribute() const;
    bool canBidAuction() const;
    bool tradePending() const { return m_engine.state().phase == GamePhase::Trade; }
    bool aiThinking() const;
    bool presentationBusy() const { return m_diceAnimating || m_engine.state().phase == GamePhase::Moving; }
    bool diceAnimating() const { return m_diceAnimating; }
    int diceValue() const { return m_diceValue; }
    QString thinkingText() const;
    bool routeSelectionVisible() const;
    int auctionBid() const { return m_engine.state().auctionHighBid; }
    int auctionSeconds() const;
    QString auctionTileName() const;
    QVariantList routeOptions() const;
    QVariantList players() const;
    QVariantList characters() const;
    QStringList eventLog() const;

    Q_INVOKABLE void newGame(int totalPlayers = 4, int aiPlayers = 3, int rounds = 120,
                             int characterIndex = 0);
    Q_INVOKABLE void rollDice();
    Q_INVOKABLE void rerollDice();
    Q_INVOKABLE void chooseRoute(int option);
    Q_INVOKABLE void buyCurrentProperty();
    Q_INVOKABLE void upgradeCurrentProperty();
    Q_INVOKABLE void endTurn();
    Q_INVOKABLE void useSkill();
    Q_INVOKABLE void useFirstCard();
    Q_INVOKABLE void mortgageCurrentProperty();
    Q_INVOKABLE void contributeCivic();
    Q_INVOKABLE void bidAuction(int amount);
    Q_INVOKABLE void passAuction();
    Q_INVOKABLE void proposeSimpleTrade(int recipientIndex, int offeredCash, int requestedCash);
    Q_INVOKABLE void respondTrade(bool accept);
    Q_INVOKABLE bool saveGame();
    Q_INVOKABLE bool loadGame();
    Q_INVOKABLE void hostGame(int totalPlayers = 4, int aiPlayers = 2, int rounds = 120,
                              int characterIndex = 0, quint16 port = net::DefaultPort);
    Q_INVOKABLE void joinGame(const QString &address, quint16 port = net::DefaultPort);
    Q_INVOKABLE QString tileDescription(int index) const;

signals:
    void stateChanged();
    void countdownChanged();
    void networkStatusChanged();
    void toastRequested(QString message);
    void cameraFocusRequested(int tileIndex);

private:
    void submit(CommandType type, const QVariantMap &arguments = {});
    void submitAs(const QUuid &playerId, CommandType type, const QVariantMap &arguments = {});
    void handleAcceptedState(int previousCurrent, int previousPosition);
    void observePresentationState(int previousCurrent, int previousPosition);
    void finishDicePresentation();
    void advanceMovementPresentation();
    void scheduleAi();
    void runAiStep();
    void tickTimedPhase();
    void setNetworkStatus(const QString &status);
    void autoSave();
    QString quickSavePath() const;
    bool localCanControlActivePlayer() const;
    QUuid localAuctionPlayer() const;

    GameEngine m_engine;
    AiPlayer m_ai{AiPlayer::Difficulty::Standard};
    QTimer m_aiTimer;
    QTimer m_phaseTimer;
    QTimer m_diceTimer;
    QTimer m_movementTimer;
    quint64 m_nextCommandId = 0;
    QString m_networkStatus = QStringLiteral("本地模式");
    std::unique_ptr<net::HostServer> m_host;
    net::GameClient m_client;
    net::LanDiscovery m_discovery;
    bool m_clientMode = false;
    QUuid m_localPlayerId;
    bool m_diceAnimating = false;
    int m_diceValue = 1;
    int m_observedDice = 0;
    quint64 m_observedRollSequence = 0;
    quint64 m_observedMovementSerial = 0;
};

} // namespace neon

#include "app/game_view_model.h"

#include "core/city_content.h"

#include <QDateTime>
#include <QDir>
#include <QRandomGenerator>
#include <algorithm>

namespace neon {

GameViewModel::GameViewModel(QObject *parent) : QObject(parent)
{
    m_aiTimer.setSingleShot(true);
    m_aiTimer.setInterval(900);
    connect(&m_aiTimer, &QTimer::timeout, this, &GameViewModel::runAiStep);
    m_diceTimer.setSingleShot(true);
    m_diceTimer.setInterval(900);
    connect(&m_diceTimer, &QTimer::timeout, this, &GameViewModel::finishDicePresentation);
    m_movementTimer.setSingleShot(true);
    m_movementTimer.setInterval(280);
    connect(&m_movementTimer, &QTimer::timeout, this, &GameViewModel::advanceMovementPresentation);
    m_phaseTimer.setInterval(250);
    connect(&m_phaseTimer, &QTimer::timeout, this, &GameViewModel::tickTimedPhase);
    m_phaseTimer.start();
    connect(&m_client, &net::GameClient::connected, this, [this] { setNetworkStatus(QStringLiteral("已连接，正在同步")); });
    connect(&m_client, &net::GameClient::disconnected, this, [this] { setNetworkStatus(QStringLiteral("连接已断开，可重新连接")); });
    connect(&m_client, &net::GameClient::errorOccurred, this, [this](const QString &error) {
        setNetworkStatus(QStringLiteral("网络错误")); emit toastRequested(error);
    });
    connect(&m_client, &net::GameClient::commandRejected, this, &GameViewModel::toastRequested);
    connect(&m_client, &net::GameClient::snapshotReceived, this, [this] {
        const int previousCurrent = m_engine.state().currentPlayer;
        const int previousPosition = currentPosition();
        m_engine.restore(m_client.state());
        if (!m_client.assignedPlayerId().isNull()) m_localPlayerId = m_client.assignedPlayerId();
        setNetworkStatus(QStringLiteral("联机同步正常"));
        observePresentationState(previousCurrent, previousPosition);
    });
    newGame();
}

QString GameViewModel::currentPlayerName() const
{
    const auto *player = m_engine.state().activePlayer();
    return player ? player->name : QStringLiteral("—");
}

QString GameViewModel::currentPlayerColor() const
{
    const auto *player = m_engine.state().activePlayer();
    return player ? player->color.name() : QStringLiteral("#B83A2D");
}

int GameViewModel::currentPosition() const
{
    const auto *player = m_engine.state().activePlayer();
    return player ? player->position : -1;
}

int GameViewModel::currentCash() const { const auto *p = m_engine.state().activePlayer(); return p ? p->cash : 0; }
int GameViewModel::currentReputation() const { const auto *p = m_engine.state().activePlayer(); return p ? p->reputation : 0; }
int GameViewModel::currentCulture() const { const auto *p = m_engine.state().activePlayer(); return p ? p->culture : 0; }
int GameViewModel::currentLivelihood() const { const auto *p = m_engine.state().activePlayer(); return p ? p->livelihood : 0; }
int GameViewModel::currentProsperity() const
{
    const auto *p = m_engine.state().activePlayer();
    return p ? m_engine.finalScore(p->id) : 0;
}
int GameViewModel::currentEnergy() const { const auto *p = m_engine.state().activePlayer(); return p ? p->energy : 0; }

QString GameViewModel::firstCardName() const
{
    const auto *player = m_engine.state().activePlayer();
    if (!player || player->strategyCards.isEmpty()) return QStringLiteral("暂无筹策");
    return CityContent::strategyCards().at(player->strategyCards.first() % 48);
}

QString GameViewModel::tileName() const
{
    const auto *tile = m_engine.state().tileAt(currentPosition());
    return tile ? tile->name : QStringLiteral("承平门");
}

QString GameViewModel::statusText() const
{
    switch (m_engine.state().phase) {
    case GamePhase::AwaitingRoll: return QStringLiteral("等待投掷骰子");
    case GamePhase::AwaitingRoute: return QStringLiteral("选择本回合路线");
    case GamePhase::Moving: return QStringLiteral("正在行进");
    case GamePhase::AwaitingDecision: return QStringLiteral("经营决策");
    case GamePhase::Auction: return QStringLiteral("公开竞价");
    case GamePhase::Trade: return QStringLiteral("产业交易");
    case GamePhase::ForcedSettlement: return QStringLiteral("资产处置");
    case GamePhase::Finished: return QStringLiteral("城市经营结算完成");
    case GamePhase::Waiting: return QStringLiteral("等待玩家");
    }
    return QStringLiteral("城市运行中");
}

bool GameViewModel::localCanControlActivePlayer() const
{
    const auto *active = m_engine.state().activePlayer();
    if (!active || active->aiControlled) return false;
    if (!m_clientMode && !m_host) return true;
    return active->id == m_localPlayerId;
}

QUuid GameViewModel::localAuctionPlayer() const
{
    if (m_clientMode || m_host) return m_localPlayerId;
    const auto *active = m_engine.state().activePlayer();
    return active && !active->aiControlled ? active->id : QUuid{};
}

bool GameViewModel::canRoll() const { return localCanControlActivePlayer() && m_engine.state().phase == GamePhase::AwaitingRoll; }
bool GameViewModel::canReroll() const
{
    const auto *p = m_engine.state().activePlayer();
    return localCanControlActivePlayer() && m_engine.state().phase == GamePhase::AwaitingRoute && p && p->rerolls > 0;
}
bool GameViewModel::canEndTurn() const { return localCanControlActivePlayer() && m_engine.state().phase == GamePhase::AwaitingDecision; }
bool GameViewModel::canUseSkill() const
{
    const auto *p = m_engine.state().activePlayer();
    return canRoll() && p && p->energy >= 3 && p->skillCooldown == 0;
}
bool GameViewModel::canUseCard() const
{
    const auto *p = m_engine.state().activePlayer();
    return localCanControlActivePlayer() && p && !p->strategyCards.isEmpty()
        && m_engine.state().phase != GamePhase::Finished;
}
bool GameViewModel::canMortgage() const
{
    if (!canEndTurn()) return false;
    const auto *p = m_engine.state().activePlayer();
    const auto *property = p ? m_engine.state().propertyAt(p->position) : nullptr;
    return p && property && property->ownerId == p->id;
}
bool GameViewModel::canBuy() const
{
    if (!canEndTurn()) return false;
    const auto *p = m_engine.state().activePlayer();
    const auto *tile = p ? m_engine.state().tileAt(p->position) : nullptr;
    const auto *property = p ? m_engine.state().propertyAt(p->position) : nullptr;
    return tile && property && property->ownerId.isNull() && p->cash >= tile->price;
}
bool GameViewModel::canUpgrade() const
{
    if (!canEndTurn()) return false;
    const auto *p = m_engine.state().activePlayer();
    const auto *tile = p ? m_engine.state().tileAt(p->position) : nullptr;
    const auto *property = p ? m_engine.state().propertyAt(p->position) : nullptr;
    return tile && property && property->ownerId == p->id && !property->mortgaged && property->level < 3;
}
bool GameViewModel::canContribute() const
{
    const auto *p = m_engine.state().activePlayer();
    const auto *tile = p ? m_engine.state().tileAt(p->position) : nullptr;
    return canEndTurn() && tile && tile->type == TileType::Civic && p->cash >= 800;
}
bool GameViewModel::canBidAuction() const
{
    const QUuid id = localAuctionPlayer();
    const auto it = std::find_if(m_engine.state().players.cbegin(), m_engine.state().players.cend(),
                                 [&](const auto &p) { return p.id == id; });
    return m_engine.state().phase == GamePhase::Auction && it != m_engine.state().players.cend()
        && !it->bankrupt && it->cash >= m_engine.state().auctionHighBid + 100;
}

bool GameViewModel::aiThinking() const
{
    if (m_engine.state().phase == GamePhase::Auction)
        return std::any_of(m_engine.state().players.cbegin(), m_engine.state().players.cend(), [&](const auto &p) {
            return p.aiControlled && !p.bankrupt && p.id != m_engine.state().auctionHighBidder
                && !m_engine.state().auctionPassedPlayers.contains(p.id);
        });
    if (m_engine.state().phase == GamePhase::Trade && m_engine.state().tradeOffer.active) {
        return std::any_of(m_engine.state().players.cbegin(), m_engine.state().players.cend(), [&](const auto &p) {
            return p.aiControlled && p.id == m_engine.state().tradeOffer.recipientId;
        });
    }
    const auto *p = m_engine.state().activePlayer();
    return p && p->aiControlled;
}

QString GameViewModel::thinkingText() const
{
    const auto *player = m_engine.state().activePlayer();
    if (!player) return QStringLiteral("正在同步城市状态…");
    if (m_engine.state().phase == GamePhase::Moving)
        return QStringLiteral("%1 正在前往下一站…").arg(player->name);
    if (m_diceAnimating) return QStringLiteral("%1 正在投掷骰子…").arg(player->name);
    if (player->aiControlled) return QStringLiteral("%1 正在思考…").arg(player->name);
    if (!localCanControlActivePlayer()) return QStringLiteral("等待 %1 操作…").arg(player->name);
    return QStringLiteral("轮到你了 · 选择下方可用操作");
}

bool GameViewModel::routeSelectionVisible() const
{
    return !m_diceAnimating && localCanControlActivePlayer()
        && m_engine.state().phase == GamePhase::AwaitingRoute;
}

int GameViewModel::auctionSeconds() const
{
    if (m_engine.state().phase != GamePhase::Auction) return 0;
    return qMax(0, int((m_engine.state().auctionDeadlineMs - QDateTime::currentMSecsSinceEpoch() + 999) / 1000));
}

QString GameViewModel::auctionTileName() const
{
    const auto *tile = m_engine.state().tileAt(m_engine.state().auctionTile);
    return tile ? tile->name : QString();
}

QVariantList GameViewModel::routeOptions() const
{
    QVariantList result;
    for (int i = 0; i < m_engine.state().routeOptions.size(); ++i) {
        const auto &route = m_engine.state().routeOptions.at(i);
        const auto *tile = route.isEmpty() ? nullptr : m_engine.state().tileAt(route.last());
        result.append(QVariantMap{{QStringLiteral("option"), i}, {QStringLiteral("endpoint"), route.isEmpty() ? -1 : route.last()},
            {QStringLiteral("name"), tile ? tile->name : QStringLiteral("未知路线")},
            {QStringLiteral("steps"), route.size()}});
    }
    return result;
}

QVariantList GameViewModel::players() const
{
    QVariantList result;
    for (int i = 0; i < m_engine.state().players.size(); ++i) {
        const auto &p = m_engine.state().players.at(i);
        result.append(QVariantMap{{QStringLiteral("index"), i}, {QStringLiteral("name"), p.name},
            {QStringLiteral("color"), p.color}, {QStringLiteral("cash"), p.cash},
            {QStringLiteral("reputation"), p.reputation}, {QStringLiteral("culture"), p.culture},
            {QStringLiteral("livelihood"), p.livelihood}, {QStringLiteral("position"), p.position},
            {QStringLiteral("score"), m_engine.finalScore(p.id)}, {QStringLiteral("ai"), p.aiControlled},
            {QStringLiteral("bankrupt"), p.bankrupt}});
    }
    return result;
}

QVariantList GameViewModel::characters() const
{
    QVariantList result;
    const auto names = CityContent::characterNames();
    const auto bios = CityContent::characterBiographies();
    for (int i = 0; i < names.size(); ++i)
        result.append(QVariantMap{{QStringLiteral("index"), i}, {QStringLiteral("name"), names.at(i)},
            {QStringLiteral("bio"), bios.value(i)}});
    return result;
}

QStringList GameViewModel::eventLog() const
{
    static const QHash<QString, QString> labels = {
        {QStringLiteral("rolled"), QStringLiteral("投掷骰子")},
        {QStringLiteral("movementStarted"), QStringLiteral("开始移动")},
        {QStringLiteral("movementStep"), QStringLiteral("到达下一站")},
        {QStringLiteral("movementCompleted"), QStringLiteral("移动完成")},
        {QStringLiteral("propertyBought"), QStringLiteral("购买产业")},
        {QStringLiteral("propertyUpgraded"), QStringLiteral("升级产业")},
        {QStringLiteral("auctionStarted"), QStringLiteral("公开竞价开始")},
        {QStringLiteral("tradeProposed"), QStringLiteral("发起产业交易")},
        {QStringLiteral("cityPulse"), QStringLiteral("城市脉冲")},
        {QStringLiteral("turnStarted"), QStringLiteral("新回合开始")},
        {QStringLiteral("gameFinished"), QStringLiteral("对局结算")}
    };
    QStringList result;
    const auto &log = m_engine.state().eventLog;
    for (int i = qMax(0, log.size() - 20); i < log.size(); ++i)
        result.prepend(QStringLiteral("记录 %1 · %2").arg(log.at(i).sequence)
                           .arg(labels.value(log.at(i).type, QStringLiteral("城市状态更新"))));
    return result;
}

void GameViewModel::newGame(int totalPlayers, int aiPlayers, int rounds, int characterIndex)
{
    if (m_host) { m_host->close(); m_host.reset(); }
    if (m_clientMode) m_client.disconnectFromHost();
    totalPlayers = qBound(2, totalPlayers, 6);
    aiPlayers = qBound(0, aiPlayers, totalPlayers - 1);
    QStringList names = CityContent::characterNames();
    characterIndex = qBound(0, characterIndex, names.size() - 1);
    names.move(characterIndex, 0);
    QString error;
    if (!m_engine.createGame(names.mid(0, totalPlayers), aiPlayers, rounds, 0, &error)) {
        emit toastRequested(error); return;
    }
    m_clientMode = false;
    m_localPlayerId = m_engine.state().players.first().id;
    m_diceAnimating = false;
    m_observedDice = 0;
    m_observedRollSequence = 0;
    m_observedMovementSerial = 0;
    setNetworkStatus(QStringLiteral("本地模式"));
    emit stateChanged();
    emit cameraFocusRequested(0);
    scheduleAi();
}

void GameViewModel::submit(CommandType type, const QVariantMap &arguments)
{
    const auto *active = m_engine.state().activePlayer();
    if (active) submitAs(active->id, type, arguments);
}

void GameViewModel::submitAs(const QUuid &playerId, CommandType type, const QVariantMap &arguments)
{
    if (playerId.isNull()) return;
    GameCommand command{m_engine.state().matchId, playerId, ++m_nextCommandId, type, arguments};
    if (m_clientMode) { m_client.sendCommand(command); return; }
    const int previousCurrent = m_engine.state().currentPlayer;
    const int previousPosition = currentPosition();
    const auto result = m_engine.apply(command);
    if (!result.accepted) emit toastRequested(result.error);
    else handleAcceptedState(previousCurrent, previousPosition);
}

void GameViewModel::handleAcceptedState(int previousCurrent, int previousPosition)
{
    autoSave();
    if (m_host) m_host->publishState();
    observePresentationState(previousCurrent, previousPosition);
    scheduleAi();
}

void GameViewModel::observePresentationState(int previousCurrent, int previousPosition)
{
    quint64 newestRoll = 0;
    const auto &events = m_engine.state().eventLog;
    for (auto it = events.crbegin(); it != events.crend(); ++it) {
        if (it->type == QStringLiteral("rolled")) { newestRoll = it->sequence; break; }
    }
    if (newestRoll > m_observedRollSequence) {
        m_observedRollSequence = newestRoll;
        m_observedDice = m_engine.state().lastDice;
        m_diceValue = qBound(1, m_observedDice, 6);
        m_diceAnimating = true;
        m_diceTimer.start();
    }
    const bool movementJustStarted = m_engine.state().movementSerial != m_observedMovementSerial;
    if (movementJustStarted) {
        m_observedMovementSerial = m_engine.state().movementSerial;
        emit cameraFocusRequested(currentPosition());
    }
    emit stateChanged();
    if (previousCurrent != m_engine.state().currentPlayer || previousPosition != currentPosition())
        emit cameraFocusRequested(currentPosition());
    if (!m_clientMode && m_engine.state().phase == GamePhase::Moving
        && !m_diceAnimating && !m_movementTimer.isActive())
        m_movementTimer.start(movementJustStarted ? 450 : 280);
}

void GameViewModel::finishDicePresentation()
{
    m_diceAnimating = false;
    emit stateChanged();
    if (!m_clientMode && m_engine.state().phase == GamePhase::Moving)
        m_movementTimer.start(450);
    else scheduleAi();
}

void GameViewModel::advanceMovementPresentation()
{
    if (m_clientMode || m_engine.state().phase != GamePhase::Moving) return;
    const int previousCurrent = m_engine.state().currentPlayer;
    const int previousPosition = currentPosition();
    const auto result = m_engine.advanceMovementStep();
    if (!result.accepted) { emit toastRequested(result.error); return; }
    handleAcceptedState(previousCurrent, previousPosition);
    if (m_engine.state().phase == GamePhase::Moving) m_movementTimer.start(280);
}

void GameViewModel::rollDice() { submit(CommandType::Roll); }
void GameViewModel::rerollDice() { submit(CommandType::UseReroll); }
void GameViewModel::chooseRoute(int option) { submit(CommandType::ChooseRoute, {{QStringLiteral("option"), option}}); }
void GameViewModel::buyCurrentProperty() { submit(CommandType::BuyProperty); }
void GameViewModel::upgradeCurrentProperty() { submit(CommandType::UpgradeProperty); }
void GameViewModel::endTurn() { submit(CommandType::EndTurn); }
void GameViewModel::useSkill() { submit(CommandType::UseSkill); }
void GameViewModel::useFirstCard() { submit(CommandType::UseCard, {{QStringLiteral("slot"), 0}}); }
void GameViewModel::mortgageCurrentProperty()
{
    if (currentPosition() >= 0) submit(CommandType::MortgageProperty, {{QStringLiteral("tile"), currentPosition()}});
}
void GameViewModel::contributeCivic() { submit(CommandType::ContributeCivic); }
void GameViewModel::bidAuction(int amount)
{
    submitAs(localAuctionPlayer(), CommandType::PlaceBid, {{QStringLiteral("amount"), amount}});
}
void GameViewModel::passAuction() { submitAs(localAuctionPlayer(), CommandType::PassAuction); }

void GameViewModel::proposeSimpleTrade(int recipientIndex, int offeredCash, int requestedCash)
{
    if (recipientIndex < 0 || recipientIndex >= m_engine.state().players.size()) return;
    submit(CommandType::ProposeTrade, {{QStringLiteral("recipient"), m_engine.state().players[recipientIndex].id.toString()},
        {QStringLiteral("offeredCash"), offeredCash}, {QStringLiteral("requestedCash"), requestedCash}});
}

void GameViewModel::respondTrade(bool accept)
{
    const QUuid id = m_engine.state().tradeOffer.recipientId;
    submitAs(id, CommandType::RespondTrade, {{QStringLiteral("accept"), accept}});
}

void GameViewModel::scheduleAi()
{
    if (!m_clientMode && !m_diceAnimating && m_engine.state().phase != GamePhase::Moving
        && aiThinking() && m_engine.state().phase != GamePhase::Finished) {
        m_aiTimer.start(int(QRandomGenerator::global()->bounded(700, 1301)));
    }
}

void GameViewModel::runAiStep()
{
    if (m_diceAnimating || m_engine.state().phase == GamePhase::Moving || !aiThinking()) return;
    const auto command = m_ai.chooseCommand(m_engine.state(), ++m_nextCommandId);
    if (command.playerId.isNull()) return;
    const int previousCurrent = m_engine.state().currentPlayer;
    const int previousPosition = currentPosition();
    const auto result = m_engine.apply(command);
    if (!result.accepted) emit toastRequested(result.error);
    else handleAcceptedState(previousCurrent, previousPosition);
}

void GameViewModel::tickTimedPhase()
{
    emit countdownChanged();
    if (m_clientMode) return;
    const int previousCurrent = m_engine.state().currentPlayer;
    const int previousPosition = currentPosition();
    const auto events = m_engine.expireTimedPhase(QDateTime::currentMSecsSinceEpoch());
    if (!events.isEmpty()) handleAcceptedState(previousCurrent, previousPosition);
}

QString GameViewModel::quickSavePath() const
{
    return SaveManager::defaultSaveDirectory() + QStringLiteral("/quick.ntsave");
}

bool GameViewModel::saveGame()
{
    QString error;
    const bool ok = SaveManager::saveAtomic(m_engine.state(), quickSavePath(), &error);
    emit toastRequested(ok ? QStringLiteral("对局已安全保存") : error);
    return ok;
}

void GameViewModel::autoSave()
{
    QString ignored;
    SaveManager::saveAtomic(m_engine.state(), quickSavePath(), &ignored);
}

bool GameViewModel::loadGame()
{
    QString error;
    auto state = SaveManager::load(quickSavePath(), &error);
    if (!error.isEmpty()) { emit toastRequested(error); return false; }
    m_engine.restore(std::move(state));
    m_clientMode = false;
    m_localPlayerId = m_engine.state().players.isEmpty() ? QUuid{} : m_engine.state().players.first().id;
    m_diceAnimating = false;
    m_observedDice = m_engine.state().lastDice;
    m_observedMovementSerial = m_engine.state().movementSerial;
    m_observedRollSequence = 0;
    for (auto it = m_engine.state().eventLog.crbegin(); it != m_engine.state().eventLog.crend(); ++it)
        if (it->type == QStringLiteral("rolled")) { m_observedRollSequence = it->sequence; break; }
    emit stateChanged(); emit cameraFocusRequested(currentPosition());
    if (m_engine.state().phase == GamePhase::Moving) m_movementTimer.start(450);
    emit toastRequested(QStringLiteral("对局已恢复"));
    scheduleAi();
    return true;
}

void GameViewModel::hostGame(int totalPlayers, int aiPlayers, int rounds, int characterIndex, quint16 port)
{
    totalPlayers = qBound(2, totalPlayers, 6);
    aiPlayers = qBound(0, aiPlayers, totalPlayers - 2);
    newGame(totalPlayers, aiPlayers, rounds, characterIndex);
    m_host = std::make_unique<net::HostServer>(&m_engine, this);
    connect(m_host.get(), &net::HostServer::authoritativeStateChanged, this, [this] {
        const int previousCurrent = m_engine.state().currentPlayer;
        const int previousPosition = currentPosition();
        autoSave(); observePresentationState(previousCurrent, previousPosition); scheduleAi();
    });
    connect(m_host.get(), &net::HostServer::playerSeatConnected, this, [this](const QUuid &) {
        emit toastRequested(QStringLiteral("远程玩家已入席")); m_host->publishState();
    });
    connect(m_host.get(), &net::HostServer::playerSeatDisconnected, this, [this](const QUuid &playerId) {
        emit toastRequested(QStringLiteral("玩家掉线，60秒后由AI托管"));
        QTimer::singleShot(60000, this, [this, playerId] {
            if (!m_host || m_host->hasConnectionForPlayer(playerId)) return;
            if (m_engine.setAiControlled(playerId, true)) {
                emit toastRequested(QStringLiteral("掉线玩家已由AI托管，可重新接管"));
                m_host->publishState(); emit stateChanged(); scheduleAi();
            }
        });
    });
    QString error;
    if (!m_host->listen(port, &error)) { m_host.reset(); emit toastRequested(error); return; }
    setNetworkStatus(QStringLiteral("房间已开启 · 端口 %1").arg(port));
}

void GameViewModel::joinGame(const QString &address, quint16 port)
{
    m_clientMode = true; m_localPlayerId = QUuid{};
    setNetworkStatus(QStringLiteral("正在连接 %1:%2").arg(address).arg(port));
    m_client.connectToHost(address, port);
}

QString GameViewModel::tileDescription(int index) const
{
    const auto *tile = m_engine.state().tileAt(index);
    if (!tile) return {};
    QString result = tile->name;
    if (tile->type == TileType::Property) {
        const auto *property = m_engine.state().propertyAt(index);
        result += QStringLiteral("\n购入 %1 · 基础收益 %2").arg(tile->price).arg(tile->baseRent);
        if (property && !property->ownerId.isNull())
            result += QStringLiteral("\n产业等级 %1%2").arg(property->level).arg(property->mortgaged ? QStringLiteral(" · 已抵押") : QString());
    }
    return result;
}

void GameViewModel::setNetworkStatus(const QString &status)
{
    if (m_networkStatus == status) return;
    m_networkStatus = status;
    emit networkStatusChanged();
}

} // namespace neon

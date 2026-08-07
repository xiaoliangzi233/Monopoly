#include "app/game_view_model.h"

#include "core/city_content.h"

#include <QDir>

namespace neon {

GameViewModel::GameViewModel(QObject *parent) : QObject(parent)
{
    m_aiTimer.setSingleShot(true);
    m_aiTimer.setInterval(420);
    connect(&m_aiTimer, &QTimer::timeout, this, &GameViewModel::runAiStep);
    connect(&m_client, &net::GameClient::connected, this, [this] { setNetworkStatus(QStringLiteral("已连接，正在同步")); });
    connect(&m_client, &net::GameClient::disconnected, this, [this] { setNetworkStatus(QStringLiteral("连接已断开，可重新连接")); });
    connect(&m_client, &net::GameClient::errorOccurred, this, [this](const QString &error) {
        setNetworkStatus(QStringLiteral("网络错误")); emit toastRequested(error);
    });
    connect(&m_client, &net::GameClient::commandRejected, this, &GameViewModel::toastRequested);
    connect(&m_client, &net::GameClient::snapshotReceived, this, [this] {
        m_engine.restore(m_client.state());
        if (!m_client.assignedPlayerId().isNull()) m_localPlayerId = m_client.assignedPlayerId();
        setNetworkStatus(QStringLiteral("联机同步正常"));
        emit stateChanged();
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
    return player ? player->color.name() : QStringLiteral("#52F7FF");
}

int GameViewModel::currentCash() const
{
    const auto *player = m_engine.state().activePlayer();
    return player ? player->cash : 0;
}

int GameViewModel::currentEnergy() const
{
    const auto *player = m_engine.state().activePlayer();
    return player ? player->energy : 0;
}

int GameViewModel::currentCardCount() const
{
    const auto *player = m_engine.state().activePlayer();
    return player ? player->strategyCards.size() : 0;
}

QString GameViewModel::firstCardName() const
{
    const auto *player = m_engine.state().activePlayer();
    if (!player || player->strategyCards.isEmpty()) return QStringLiteral("无策略卡");
    const auto cards = CityContent::strategyCards();
    return cards.at(player->strategyCards.first() % cards.size());
}

QString GameViewModel::tileName() const
{
    const auto *player = m_engine.state().activePlayer();
    const auto *tile = player ? m_engine.state().tileAt(player->position) : nullptr;
    return tile ? tile->name : QStringLiteral("霓虹广场");
}

QString GameViewModel::statusText() const
{
    switch (m_engine.state().phase) {
    case GamePhase::AwaitingRoll: return QStringLiteral("等待掷骰");
    case GamePhase::AwaitingDecision: return QStringLiteral("选择本回合行动");
    case GamePhase::Finished: return QStringLiteral("对局结束");
    case GamePhase::Waiting: return QStringLiteral("等待玩家");
    default: return QStringLiteral("城市运行中");
    }
}

bool GameViewModel::aiThinking() const
{
    const auto *player = m_engine.state().activePlayer();
    return player && player->aiControlled;
}

bool GameViewModel::localCanControlActivePlayer() const
{
    const auto *active = m_engine.state().activePlayer();
    if (!active || active->aiControlled) return false;
    if (!m_clientMode && !m_host) return true;
    return active->id == m_localPlayerId;
}

bool GameViewModel::canRoll() const { return localCanControlActivePlayer() && m_engine.state().phase == GamePhase::AwaitingRoll; }
bool GameViewModel::canEndTurn() const { return localCanControlActivePlayer() && m_engine.state().phase == GamePhase::AwaitingDecision; }

bool GameViewModel::canUseSkill() const
{
    const auto *player = m_engine.state().activePlayer();
    return localCanControlActivePlayer() && m_engine.state().phase == GamePhase::AwaitingRoll
        && player && player->energy >= 2 && player->skillCooldown == 0;
}

bool GameViewModel::canUseCard() const
{
    const auto *player = m_engine.state().activePlayer();
    return localCanControlActivePlayer() && player && !player->strategyCards.isEmpty()
        && m_engine.state().phase != GamePhase::Finished;
}

bool GameViewModel::canMortgage() const
{
    if (!canEndTurn()) return false;
    const auto *player = m_engine.state().activePlayer();
    const auto *property = player ? m_engine.state().propertyAt(player->position) : nullptr;
    return player && property && property->ownerId == player->id;
}

bool GameViewModel::canBuy() const
{
    if (!canEndTurn()) return false;
    const auto *player = m_engine.state().activePlayer();
    const auto *tile = player ? m_engine.state().tileAt(player->position) : nullptr;
    const auto *property = player ? m_engine.state().propertyAt(player->position) : nullptr;
    return tile && property && property->ownerId.isNull() && player->cash >= tile->price;
}

bool GameViewModel::canUpgrade() const
{
    if (!canEndTurn()) return false;
    const auto *player = m_engine.state().activePlayer();
    const auto *tile = player ? m_engine.state().tileAt(player->position) : nullptr;
    const auto *property = player ? m_engine.state().propertyAt(player->position) : nullptr;
    if (!tile || !property || property->ownerId != player->id || property->level >= 3) return false;
    const int cost = tile->price / 2 * (property->level + 1) * m_engine.state().buildingCostPercent / 100;
    return player->cash >= cost;
}

QVariantList GameViewModel::players() const
{
    QVariantList result;
    for (const auto &player : m_engine.state().players) {
        QVariantMap item{{QStringLiteral("name"), player.name}, {QStringLiteral("color"), player.color},
            {QStringLiteral("cash"), player.cash}, {QStringLiteral("position"), player.position},
            {QStringLiteral("score"), m_engine.finalScore(player.id)}, {QStringLiteral("ai"), player.aiControlled},
            {QStringLiteral("bankrupt"), player.bankrupt}};
        result.append(item);
    }
    return result;
}

QStringList GameViewModel::eventLog() const
{
    QStringList result;
    const auto &log = m_engine.state().eventLog;
    for (int i = qMax(0, log.size() - 12); i < log.size(); ++i) {
        const auto &event = log.at(i);
        result.prepend(QStringLiteral("#%1  %2").arg(event.sequence).arg(event.type));
    }
    return result;
}

void GameViewModel::newGame(int totalPlayers, int aiPlayers, int rounds)
{
    if (m_host) { m_host->close(); m_host.reset(); }
    if (m_clientMode) m_client.disconnectFromHost();
    totalPlayers = qBound(2, totalPlayers, 6);
    aiPlayers = qBound(0, aiPlayers, totalPlayers - 1);
    const auto names = CityContent::characterNames().mid(0, totalPlayers);
    QString error;
    if (!m_engine.createGame(names, aiPlayers, rounds, 0, &error)) {
        emit toastRequested(error);
        return;
    }
    m_clientMode = false;
    m_localPlayerId = m_engine.state().players.first().id;
    setNetworkStatus(QStringLiteral("本地模式"));
    emit stateChanged();
    scheduleAi();
}

void GameViewModel::submit(CommandType type, const QVariantMap &arguments)
{
    const auto *active = m_engine.state().activePlayer();
    if (!active) return;
    if (!localCanControlActivePlayer() && !active->aiControlled) {
        emit toastRequested(QStringLiteral("正在等待其他玩家"));
        return;
    }
    GameCommand command{m_engine.state().matchId, active->id, ++m_nextCommandId, type, arguments};
    if (m_clientMode) {
        if (active->id != m_localPlayerId) {
            emit toastRequested(QStringLiteral("正在等待其他玩家"));
            return;
        }
        m_client.sendCommand(command);
        return;
    }
    const auto result = m_engine.apply(command);
    if (!result.accepted) emit toastRequested(result.error);
    else {
        autoSave();
        if (m_host) m_host->publishState();
        emit stateChanged();
        scheduleAi();
    }
}

void GameViewModel::rollDice() { submit(CommandType::Roll); }
void GameViewModel::buyCurrentProperty() { submit(CommandType::BuyProperty); }
void GameViewModel::upgradeCurrentProperty() { submit(CommandType::UpgradeProperty); }
void GameViewModel::endTurn() { submit(CommandType::EndTurn); }
void GameViewModel::useSkill() { submit(CommandType::UseSkill); }
void GameViewModel::useFirstCard() { submit(CommandType::UseCard, {{QStringLiteral("slot"), 0}}); }
void GameViewModel::mortgageCurrentProperty()
{
    const auto *player = m_engine.state().activePlayer();
    if (player) submit(CommandType::MortgageProperty, {{QStringLiteral("tile"), player->position}});
}

void GameViewModel::scheduleAi()
{
    if (!m_clientMode && aiThinking() && m_engine.state().phase != GamePhase::Finished) m_aiTimer.start();
}

void GameViewModel::runAiStep()
{
    if (!aiThinking()) return;
    const auto command = m_ai.chooseCommand(m_engine.state(), ++m_nextCommandId);
    const auto result = m_engine.apply(command);
    if (!result.accepted) emit toastRequested(result.error);
    else {
        autoSave();
        if (m_host) m_host->publishState();
        emit stateChanged();
        scheduleAi();
    }
}

QString GameViewModel::quickSavePath() const { return SaveManager::defaultSaveDirectory() + QStringLiteral("/quick.ntsave"); }

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
    emit stateChanged();
    emit toastRequested(QStringLiteral("对局已恢复"));
    scheduleAi();
    return true;
}

void GameViewModel::hostGame(int totalPlayers, int aiPlayers, quint16 port)
{
    totalPlayers = qBound(2, totalPlayers, 6);
    aiPlayers = qBound(0, aiPlayers, totalPlayers - 2);
    newGame(totalPlayers, aiPlayers, 24);
    m_host = std::make_unique<net::HostServer>(&m_engine, this);
    connect(m_host.get(), &net::HostServer::authoritativeStateChanged, this, [this] {
        autoSave(); emit stateChanged(); scheduleAi();
    });
    connect(m_host.get(), &net::HostServer::playerSeatConnected, this, [this](const QUuid &) {
        emit toastRequested(QStringLiteral("远程玩家已连接"));
        m_host->publishState();
    });
    connect(m_host.get(), &net::HostServer::playerSeatDisconnected, this, [this](const QUuid &playerId) {
        emit toastRequested(QStringLiteral("玩家掉线，60秒后由AI托管"));
        QTimer::singleShot(60000, this, [this, playerId] {
            if (!m_host || m_host->hasConnectionForPlayer(playerId)) return;
            if (m_engine.setAiControlled(playerId, true)) {
                emit toastRequested(QStringLiteral("掉线玩家已由AI托管，可随时重连接管"));
                m_host->publishState();
                emit stateChanged();
                scheduleAi();
            }
        });
    });
    QString error;
    if (!m_host->listen(port, &error)) {
        m_host.reset();
        emit toastRequested(error);
        return;
    }
    setNetworkStatus(QStringLiteral("房间已开启 · 端口 %1").arg(port));
}

void GameViewModel::joinGame(const QString &address, quint16 port)
{
    m_clientMode = true;
    m_localPlayerId = QUuid{};
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
        result += QStringLiteral("\n地价 %1 · 基础租金 %2").arg(tile->price).arg(tile->baseRent);
        if (property && !property->ownerId.isNull()) result += QStringLiteral("\n建筑等级 %1").arg(property->level);
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

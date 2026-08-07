#pragma once

#include "core/game_state.h"
#include "network/protocol.h"

#include <QTcpSocket>

namespace neon::net {

class GameClient final : public QObject {
    Q_OBJECT
public:
    explicit GameClient(QObject *parent = nullptr);
    void connectToHost(const QString &address, quint16 port = DefaultPort);
    void disconnectFromHost();
    void sendCommand(const GameCommand &command);
    [[nodiscard]] bool isConnected() const;
    [[nodiscard]] const GameState &state() const { return m_state; }
    [[nodiscard]] QUuid assignedPlayerId() const { return m_assignedPlayerId; }

signals:
    void connected();
    void disconnected();
    void snapshotReceived();
    void commandRejected(QString reason);
    void errorOccurred(QString error);

private:
    void onReadyRead();
    void processEnvelope(const ProtocolEnvelope &envelope);
    void send(const ProtocolEnvelope &envelope);

    QTcpSocket m_socket;
    QByteArray m_buffer;
    quint64 m_requestId = 0;
    GameState m_state;
    QUuid m_assignedPlayerId;
};

} // namespace neon::net

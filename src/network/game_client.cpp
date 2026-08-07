#include "network/game_client.h"

namespace neon::net {

GameClient::GameClient(QObject *parent) : QObject(parent)
{
    connect(&m_socket, &QTcpSocket::connected, this, [this] {
        QCborMap payload{{QStringLiteral("clientVersion"), QStringLiteral(NEON_VERSION)}};
        if (!m_assignedPlayerId.isNull())
            payload.insert(QStringLiteral("playerId"), m_assignedPlayerId.toString(QUuid::WithoutBraces));
        send({ProtocolVersion, m_assignedPlayerId.isNull() ? MessageType::Hello : MessageType::Resume,
              ++m_requestId, payload});
        emit connected();
    });
    connect(&m_socket, &QTcpSocket::disconnected, this, &GameClient::disconnected);
    connect(&m_socket, &QTcpSocket::readyRead, this, &GameClient::onReadyRead);
    connect(&m_socket, &QTcpSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) { emit errorOccurred(m_socket.errorString()); });
}

void GameClient::connectToHost(const QString &address, quint16 port)
{
    m_socket.connectToHost(address, port);
}

void GameClient::disconnectFromHost() { m_socket.disconnectFromHost(); }
bool GameClient::isConnected() const { return m_socket.state() == QAbstractSocket::ConnectedState; }

void GameClient::sendCommand(const GameCommand &command)
{
    send({ProtocolVersion, MessageType::Command, ++m_requestId,
          {{QStringLiteral("command"), commandToCbor(command)}}});
}

void GameClient::send(const ProtocolEnvelope &envelope)
{
    if (isConnected()) m_socket.write(framePayload(encodeEnvelope(envelope)));
}

void GameClient::onReadyRead()
{
    m_buffer.append(m_socket.readAll());
    QByteArray frame;
    QString error;
    while (takeFrame(m_buffer, &frame, &error)) {
        ProtocolEnvelope envelope;
        if (!decodeEnvelope(frame, &envelope, &error)) {
            emit errorOccurred(error);
            disconnectFromHost();
            return;
        }
        processEnvelope(envelope);
    }
    if (!error.isEmpty()) emit errorOccurred(error);
}

void GameClient::processEnvelope(const ProtocolEnvelope &envelope)
{
    if (envelope.messageType == MessageType::Welcome) {
        m_assignedPlayerId = QUuid(envelope.payload.value(QStringLiteral("playerId")).toString());
    } else if (envelope.messageType == MessageType::CommandRejected) {
        emit commandRejected(envelope.payload.value(QStringLiteral("reason")).toString());
    } else if (envelope.messageType == MessageType::Snapshot) {
        QString error;
        auto state = GameState::fromCbor(envelope.payload.value(QStringLiteral("state")).toMap(), &error);
        if (!error.isEmpty()) {
            emit errorOccurred(error);
            return;
        }
        const auto expected = quint64(envelope.payload.value(QStringLiteral("hash")).toInteger());
        if (state.stableHash() != expected) {
            emit errorOccurred(QStringLiteral("Authoritative snapshot hash mismatch"));
            return;
        }
        m_state = std::move(state);
        emit snapshotReceived();
    }
}

} // namespace neon::net

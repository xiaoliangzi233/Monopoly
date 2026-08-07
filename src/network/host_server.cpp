#include "network/host_server.h"

#include <QTcpSocket>
#include <algorithm>
#include <utility>

namespace neon::net {

HostServer::HostServer(GameEngine *engine, QObject *parent) : QObject(parent), m_engine(engine)
{
    if (m_engine) {
        for (int i = 1; i < m_engine->state().players.size(); ++i) {
            const auto &player = m_engine->state().players.at(i);
            if (!player.aiControlled) m_remoteSeats.append(player.id);
        }
    }
    connect(&m_server, &QTcpServer::newConnection, this, &HostServer::onNewConnection);
}

bool HostServer::listen(quint16 port, QString *error)
{
    if (!m_engine) {
        if (error) *error = QStringLiteral("Host has no game engine");
        return false;
    }
    if (!m_server.listen(QHostAddress::AnyIPv4, port)) {
        if (error) *error = m_server.errorString();
        return false;
    }
    if (error) error->clear();
    return true;
}

void HostServer::close()
{
    for (auto *socket : m_buffers.keys()) socket->disconnectFromHost();
    m_server.close();
}

void HostServer::publishState()
{
    broadcastSnapshot();
    emit authoritativeStateChanged();
}

bool HostServer::hasConnectionForPlayer(const QUuid &playerId) const
{
    return m_playerAssignments.values().contains(playerId);
}

void HostServer::onNewConnection()
{
    while (auto *socket = m_server.nextPendingConnection()) {
        m_buffers.insert(socket, {});
        connect(socket, &QTcpSocket::readyRead, this, [this, socket] { onReadyRead(socket); });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket] {
            const QUuid assigned = m_playerAssignments.value(socket);
            emit clientDisconnected(socket->peerAddress().toString());
            if (!assigned.isNull()) emit playerSeatDisconnected(assigned);
            m_buffers.remove(socket);
            m_playerAssignments.remove(socket);
            socket->deleteLater();
        });
        emit clientConnected(socket->peerAddress().toString());
    }
}

void HostServer::onReadyRead(QTcpSocket *socket)
{
    auto &buffer = m_buffers[socket];
    buffer.append(socket->readAll());
    QByteArray frame;
    QString error;
    while (takeFrame(buffer, &frame, &error)) {
        ProtocolEnvelope envelope;
        if (!decodeEnvelope(frame, &envelope, &error)) {
            emit protocolError(error);
            socket->disconnectFromHost();
            return;
        }
        processEnvelope(socket, envelope);
    }
    if (!error.isEmpty()) {
        emit protocolError(error);
        socket->disconnectFromHost();
    }
}

void HostServer::processEnvelope(QTcpSocket *socket, const ProtocolEnvelope &envelope)
{
    if (envelope.messageType == MessageType::Hello || envelope.messageType == MessageType::Resume) {
        QUuid assigned;
        const QUuid requested(envelope.payload.value(QStringLiteral("playerId")).toString());
        if (!requested.isNull() && !m_playerAssignments.values().contains(requested)) {
            if (m_remoteSeats.contains(requested)) assigned = requested;
        }
        if (assigned.isNull()) {
            for (const auto &seat : std::as_const(m_remoteSeats)) {
                if (!m_playerAssignments.values().contains(seat)) {
                    assigned = seat;
                    break;
                }
            }
        }
        if (assigned.isNull()) {
            send(socket, {ProtocolVersion, MessageType::CommandRejected, envelope.requestId,
                          {{QStringLiteral("reason"), QStringLiteral("房间没有可用的远程玩家席位")}}});
            socket->disconnectFromHost();
            return;
        }
        m_playerAssignments.insert(socket, assigned);
        m_engine->setAiControlled(assigned, false);
        emit playerSeatConnected(assigned);
        send(socket, {ProtocolVersion, MessageType::Welcome, envelope.requestId,
                      {{QStringLiteral("match"), m_engine->state().matchId.toString(QUuid::WithoutBraces)},
                       {QStringLiteral("playerId"), assigned.toString(QUuid::WithoutBraces)}}});
        send(socket, {ProtocolVersion, MessageType::Snapshot, envelope.requestId,
                      {{QStringLiteral("state"), m_engine->state().toCbor(true)},
                       {QStringLiteral("hash"), qint64(m_engine->state().stableHash())}}});
        return;
    }
    if (envelope.messageType == MessageType::Ping) {
        send(socket, {ProtocolVersion, MessageType::Pong, envelope.requestId, {}});
        return;
    }
    if (envelope.messageType != MessageType::Command) return;

    QString error;
    const auto command = commandFromCbor(envelope.payload.value(QStringLiteral("command")).toMap(), &error);
    if (!error.isEmpty()) {
        send(socket, {ProtocolVersion, MessageType::CommandRejected, envelope.requestId,
                      {{QStringLiteral("reason"), error}}});
        return;
    }
    if (!m_playerAssignments.contains(socket) || m_playerAssignments.value(socket) != command.playerId) {
        send(socket, {ProtocolVersion, MessageType::CommandRejected, envelope.requestId,
                      {{QStringLiteral("reason"), QStringLiteral("客户端无权控制该玩家")}}});
        return;
    }
    const auto result = m_engine->apply(command);
    if (!result.accepted) {
        send(socket, {ProtocolVersion, MessageType::CommandRejected, envelope.requestId,
                      {{QStringLiteral("reason"), result.error}}});
        return;
    }
    broadcastSnapshot(envelope.requestId);
    emit authoritativeStateChanged();
}

void HostServer::send(QTcpSocket *socket, const ProtocolEnvelope &envelope)
{
    socket->write(framePayload(encodeEnvelope(envelope)));
}

void HostServer::broadcastSnapshot(quint64 requestId)
{
    const ProtocolEnvelope snapshot{ProtocolVersion, MessageType::Snapshot, requestId,
        {{QStringLiteral("state"), m_engine->state().toCbor(true)},
         {QStringLiteral("hash"), qint64(m_engine->state().stableHash())}}};
    for (auto *socket : m_buffers.keys()) send(socket, snapshot);
}

} // namespace neon::net

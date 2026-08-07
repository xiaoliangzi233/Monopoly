#pragma once

#include "core/game_engine.h"
#include "network/protocol.h"

#include <QHash>
#include <QTcpServer>

class QTcpSocket;

namespace neon::net {

class HostServer final : public QObject {
    Q_OBJECT
public:
    explicit HostServer(GameEngine *engine, QObject *parent = nullptr);
    bool listen(quint16 port = DefaultPort, QString *error = nullptr);
    void close();
    [[nodiscard]] bool isListening() const { return m_server.isListening(); }
    [[nodiscard]] quint16 port() const { return m_server.serverPort(); }
    void publishState();
    [[nodiscard]] bool hasConnectionForPlayer(const QUuid &playerId) const;

signals:
    void clientConnected(QString address);
    void clientDisconnected(QString address);
    void playerSeatConnected(QUuid playerId);
    void playerSeatDisconnected(QUuid playerId);
    void authoritativeStateChanged();
    void protocolError(QString error);

private:
    void onNewConnection();
    void onReadyRead(QTcpSocket *socket);
    void processEnvelope(QTcpSocket *socket, const ProtocolEnvelope &envelope);
    void send(QTcpSocket *socket, const ProtocolEnvelope &envelope);
    void broadcastSnapshot(quint64 requestId = 0);

    GameEngine *m_engine = nullptr;
    QTcpServer m_server;
    QHash<QTcpSocket *, QByteArray> m_buffers;
    QHash<QTcpSocket *, QUuid> m_playerAssignments;
    QList<QUuid> m_remoteSeats;
};

} // namespace neon::net

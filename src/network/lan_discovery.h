#pragma once

#include "network/protocol.h"

#include <QUdpSocket>

namespace neon::net {

class LanDiscovery final : public QObject {
    Q_OBJECT
public:
    explicit LanDiscovery(QObject *parent = nullptr);
    bool startResponder(const QString &roomName, quint16 gamePort = DefaultPort);
    bool startBrowser(quint16 discoveryPort = DefaultPort + 1);
    void search();
    void stop();

signals:
    void roomFound(QString roomName, QString address, quint16 port);

private:
    void readPendingDatagrams();

    QUdpSocket m_socket;
    QString m_roomName;
    quint16 m_gamePort = DefaultPort;
    bool m_responder = false;
};

} // namespace neon::net

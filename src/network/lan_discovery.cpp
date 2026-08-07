#include "network/lan_discovery.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace neon::net {

LanDiscovery::LanDiscovery(QObject *parent) : QObject(parent)
{
    connect(&m_socket, &QUdpSocket::readyRead, this, &LanDiscovery::readPendingDatagrams);
}

bool LanDiscovery::startResponder(const QString &roomName, quint16 gamePort)
{
    stop();
    m_roomName = roomName;
    m_gamePort = gamePort;
    m_responder = true;
    return m_socket.bind(QHostAddress::AnyIPv4, DefaultPort + 1,
                         QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
}

bool LanDiscovery::startBrowser(quint16 discoveryPort)
{
    stop();
    m_responder = false;
    return m_socket.bind(QHostAddress::AnyIPv4, discoveryPort,
                         QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
}

void LanDiscovery::search()
{
    const QByteArray request("NEON_TYCOON_DISCOVER_V1");
    m_socket.writeDatagram(request, QHostAddress::Broadcast, DefaultPort + 1);
}

void LanDiscovery::stop()
{
    m_socket.close();
    m_roomName.clear();
}

void LanDiscovery::readPendingDatagrams()
{
    while (m_socket.hasPendingDatagrams()) {
        QHostAddress sender;
        quint16 senderPort = 0;
        QByteArray data(int(m_socket.pendingDatagramSize()), Qt::Uninitialized);
        m_socket.readDatagram(data.data(), data.size(), &sender, &senderPort);
        if (m_responder && data == "NEON_TYCOON_DISCOVER_V1") {
            const QJsonObject response{{QStringLiteral("game"), QStringLiteral("NeonTycoon")},
                {QStringLiteral("protocol"), int(ProtocolVersion)}, {QStringLiteral("room"), m_roomName},
                {QStringLiteral("port"), int(m_gamePort)}};
            m_socket.writeDatagram(QJsonDocument(response).toJson(QJsonDocument::Compact), sender, senderPort);
        } else if (!m_responder) {
            const auto document = QJsonDocument::fromJson(data);
            const auto object = document.object();
            if (object.value(QStringLiteral("game")).toString() == QStringLiteral("NeonTycoon")
                && object.value(QStringLiteral("protocol")).toInt() == int(ProtocolVersion)) {
                emit roomFound(object.value(QStringLiteral("room")).toString(), sender.toString(),
                               quint16(object.value(QStringLiteral("port")).toInt()));
            }
        }
    }
}

} // namespace neon::net

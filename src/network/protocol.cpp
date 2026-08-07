#include "network/protocol.h"

#include <QCborValue>
#include <QtEndian>

namespace neon::net {

QByteArray encodeEnvelope(const ProtocolEnvelope &envelope)
{
    const QCborMap map{{QStringLiteral("version"), qint64(envelope.protocolVersion)},
        {QStringLiteral("type"), int(envelope.messageType)},
        {QStringLiteral("request"), qint64(envelope.requestId)},
        {QStringLiteral("payload"), envelope.payload}};
    return QCborValue(map).toCbor();
}

bool decodeEnvelope(const QByteArray &frame, ProtocolEnvelope *envelope, QString *error)
{
    if (!envelope || frame.isEmpty() || frame.size() > int(MaximumFrameSize)) {
        if (error) *error = QStringLiteral("Invalid network frame size");
        return false;
    }
    QCborParserError parserError;
    const auto value = QCborValue::fromCbor(frame, &parserError);
    if (parserError.error != QCborError::NoError || !value.isMap()) {
        if (error) *error = QStringLiteral("Malformed CBOR envelope");
        return false;
    }
    const auto map = value.toMap();
    envelope->protocolVersion = quint32(map.value(QStringLiteral("version")).toInteger());
    envelope->messageType = MessageType(map.value(QStringLiteral("type")).toInteger());
    envelope->requestId = quint64(map.value(QStringLiteral("request")).toInteger());
    envelope->payload = map.value(QStringLiteral("payload")).toMap();
    if (envelope->protocolVersion != ProtocolVersion) {
        if (error) *error = QStringLiteral("Incompatible protocol version");
        return false;
    }
    if (error) error->clear();
    return true;
}

QByteArray framePayload(const QByteArray &payload)
{
    QByteArray framed(sizeof(quint32), Qt::Uninitialized);
    qToBigEndian<quint32>(quint32(payload.size()), reinterpret_cast<uchar *>(framed.data()));
    framed.append(payload);
    return framed;
}

bool takeFrame(QByteArray &buffer, QByteArray *frame, QString *error)
{
    if (!frame || buffer.size() < int(sizeof(quint32))) return false;
    const quint32 size = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(buffer.constData()));
    if (size == 0 || size > MaximumFrameSize) {
        if (error) *error = QStringLiteral("Network frame exceeds safety limit");
        buffer.clear();
        return false;
    }
    if (buffer.size() < int(sizeof(quint32) + size)) return false;
    *frame = buffer.mid(sizeof(quint32), size);
    buffer.remove(0, sizeof(quint32) + size);
    if (error) error->clear();
    return true;
}

QCborMap commandToCbor(const GameCommand &command)
{
    return {{QStringLiteral("match"), command.matchId.toString(QUuid::WithoutBraces)},
        {QStringLiteral("player"), command.playerId.toString(QUuid::WithoutBraces)},
        {QStringLiteral("id"), qint64(command.commandId)}, {QStringLiteral("type"), int(command.type)},
        {QStringLiteral("arguments"), QCborValue::fromVariant(command.arguments)}};
}

GameCommand commandFromCbor(const QCborMap &map, QString *error)
{
    GameCommand command;
    command.matchId = QUuid(map.value(QStringLiteral("match")).toString());
    command.playerId = QUuid(map.value(QStringLiteral("player")).toString());
    command.commandId = quint64(map.value(QStringLiteral("id")).toInteger());
    command.type = CommandType(map.value(QStringLiteral("type")).toInteger());
    command.arguments = map.value(QStringLiteral("arguments")).toVariant().toMap();
    if (command.matchId.isNull() || command.playerId.isNull() || command.commandId == 0) {
        if (error) *error = QStringLiteral("Invalid command envelope");
        return {};
    }
    if (error) error->clear();
    return command;
}

} // namespace neon::net

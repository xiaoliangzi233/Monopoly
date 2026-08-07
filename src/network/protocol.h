#pragma once

#include "core/game_types.h"

#include <QCborMap>

namespace neon::net {

constexpr quint32 ProtocolVersion = 1;
constexpr quint16 DefaultPort = 29450;
constexpr quint32 MaximumFrameSize = 4 * 1024 * 1024;

enum class MessageType : quint16 {
    Hello = 1,
    Welcome,
    Command,
    CommandRejected,
    Snapshot,
    EventBatch,
    Ping,
    Pong,
    Resume
};

struct ProtocolEnvelope {
    quint32 protocolVersion = ProtocolVersion;
    MessageType messageType = MessageType::Ping;
    quint64 requestId = 0;
    QCborMap payload;
};

QByteArray encodeEnvelope(const ProtocolEnvelope &envelope);
bool decodeEnvelope(const QByteArray &frame, ProtocolEnvelope *envelope, QString *error = nullptr);
QByteArray framePayload(const QByteArray &payload);
bool takeFrame(QByteArray &buffer, QByteArray *frame, QString *error = nullptr);

QCborMap commandToCbor(const GameCommand &command);
GameCommand commandFromCbor(const QCborMap &map, QString *error = nullptr);

} // namespace neon::net

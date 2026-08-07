#include "core/save_manager.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace neon {
namespace {
constexpr auto MagicV2 = "SHENGSHIBAIYE-SAVE-V2\n";
constexpr auto MagicV1 = "NEONTYCOON-SAVE\n";
constexpr auto GameVersion = "0.2.0";
}

QString SaveManager::defaultSaveDirectory()
{
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/saves");
    QDir().mkpath(directory);
    return directory;
}

QString SaveManager::legacyArchiveDirectory()
{
    const QString directory = defaultSaveDirectory() + QStringLiteral("/archive-v1");
    QDir().mkpath(directory);
    return directory;
}

bool SaveManager::archiveLegacySave(const QString &path, QString *archivedPath)
{
    if (!QFileInfo::exists(path)) return false;
    const QFileInfo source(path);
    QString target = legacyArchiveDirectory() + QLatin1Char('/') + source.completeBaseName()
        + QStringLiteral("-%1.").arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss")))
        + source.suffix();
    int suffix = 1;
    while (QFileInfo::exists(target))
        target = legacyArchiveDirectory() + QLatin1Char('/') + source.completeBaseName()
            + QStringLiteral("-%1-%2.").arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss")))
                  .arg(suffix++) + source.suffix();
    const bool moved = QFile::rename(path, target);
    if (moved && archivedPath) *archivedPath = target;
    return moved;
}

bool SaveManager::saveAtomic(const GameState &state, const QString &path, QString *error)
{
    const QByteArray payload = QCborValue(state.toCbor(true)).toCbor();
    const QByteArray checksum = QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex();
    const QJsonObject header{{QStringLiteral("formatVersion"), 2},
        {QStringLiteral("rulesVersion"), int(state.rulesVersion)},
        {QStringLiteral("contentHash"), QString::fromLatin1(state.contentHash)},
        {QStringLiteral("gameVersion"), QString::fromLatin1(GameVersion)},
        {QStringLiteral("checksum"), QString::fromLatin1(checksum)},
        {QStringLiteral("matchId"), state.matchId.toString(QUuid::WithoutBraces)},
        {QStringLiteral("timestampMs"), QJsonValue(QDateTime::currentMSecsSinceEpoch())}};
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) { if (error) *error = file.errorString(); return false; }
    file.write(MagicV2);
    file.write(QJsonDocument(header).toJson(QJsonDocument::Compact));
    file.write("\n");
    file.write(payload);
    if (!file.commit()) { if (error) *error = file.errorString(); return false; }
    if (error) error->clear();
    return true;
}

GameState SaveManager::load(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { if (error) *error = file.errorString(); return {}; }
    const QByteArray magic = file.readLine();
    if (magic == MagicV1) {
        file.close();
        QString archived;
        const bool moved = archiveLegacySave(path, &archived);
        if (error) *error = moved
            ? QStringLiteral("v0.1.0旧存档不兼容，已安全归档到：%1").arg(archived)
            : QStringLiteral("检测到v0.1.0旧存档，但无法移动到archive-v1目录");
        return {};
    }
    const auto headerDocument = QJsonDocument::fromJson(file.readLine().trimmed());
    const QByteArray payload = file.readAll();
    const auto header = headerDocument.object();
    const QByteArray expected = header.value(QStringLiteral("checksum")).toString().toLatin1();
    const QByteArray actual = QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex();
    if (magic != MagicV2 || !headerDocument.isObject() || header.value(QStringLiteral("formatVersion")).toInt() != 2
        || expected != actual) {
        if (error) *error = QStringLiteral("存档头或SHA-256校验无效");
        return {};
    }
    QCborParserError parserError;
    const auto value = QCborValue::fromCbor(payload, &parserError);
    if (parserError.error != QCborError::NoError || !value.isMap()) {
        if (error) *error = QStringLiteral("存档内容无效"); return {};
    }
    return GameState::fromCbor(value.toMap(), error);
}

} // namespace neon

#include "core/save_manager.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QStandardPaths>

namespace neon {
namespace {
constexpr auto Magic = "NEONTYCOON-SAVE\n";
}

QString SaveManager::defaultSaveDirectory()
{
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/saves");
    QDir().mkpath(directory);
    return directory;
}

bool SaveManager::saveAtomic(const GameState &state, const QString &path, QString *error)
{
    const QByteArray payload = QCborValue(state.toCbor(true)).toCbor();
    const QByteArray hash = QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex();
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    file.write(Magic);
    file.write(hash);
    file.write("\n");
    file.write(payload);
    if (!file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    if (error) error->clear();
    return true;
}

GameState SaveManager::load(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
        return {};
    }
    const QByteArray magic = file.readLine();
    const QByteArray expectedHash = file.readLine().trimmed();
    const QByteArray payload = file.readAll();
    if (magic != Magic || expectedHash != QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex()) {
        if (error) *error = QStringLiteral("Save file is corrupt");
        return {};
    }
    QCborParserError parserError;
    const auto value = QCborValue::fromCbor(payload, &parserError);
    if (parserError.error != QCborError::NoError || !value.isMap()) {
        if (error) *error = QStringLiteral("Save file payload is invalid");
        return {};
    }
    return GameState::fromCbor(value.toMap(), error);
}

} // namespace neon

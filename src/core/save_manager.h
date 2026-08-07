#pragma once

#include "core/game_state.h"

namespace neon {

struct SaveHeader {
    quint32 formatVersion = 2;
    quint32 rulesVersion = 2;
    QByteArray contentHash;
    QString gameVersion;
    QByteArray checksum;
    QUuid matchId;
    qint64 timestampMs = 0;
};

class SaveManager final {
public:
    static QString defaultSaveDirectory();
    static QString legacyArchiveDirectory();
    static bool saveAtomic(const GameState &state, const QString &path, QString *error = nullptr);
    static GameState load(const QString &path, QString *error = nullptr);
    static bool archiveLegacySave(const QString &path, QString *archivedPath = nullptr);
};

} // namespace neon

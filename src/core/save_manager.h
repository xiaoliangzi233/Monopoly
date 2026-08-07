#pragma once

#include "core/game_state.h"

namespace neon {

class SaveManager final {
public:
    static QString defaultSaveDirectory();
    static bool saveAtomic(const GameState &state, const QString &path, QString *error = nullptr);
    static GameState load(const QString &path, QString *error = nullptr);
};

} // namespace neon

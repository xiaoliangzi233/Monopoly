#pragma once

#include "core/game_engine.h"

namespace neon {

class AiPlayer final {
public:
    enum class Difficulty { Easy, Standard, Expert };

    explicit AiPlayer(Difficulty difficulty = Difficulty::Standard) : m_difficulty(difficulty) {}
    [[nodiscard]] GameCommand chooseCommand(const GameState &state, quint64 commandId) const;

private:
    Difficulty m_difficulty;
};

} // namespace neon

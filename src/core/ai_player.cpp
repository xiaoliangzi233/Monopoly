#include "core/ai_player.h"

namespace neon {

GameCommand AiPlayer::chooseCommand(const GameState &state, quint64 commandId) const
{
    GameCommand command;
    command.matchId = state.matchId;
    command.commandId = commandId;
    const auto *player = state.activePlayer();
    if (!player) return command;
    command.playerId = player->id;
    if (state.phase == GamePhase::AwaitingRoll) {
        if (m_difficulty != Difficulty::Easy && player->energy >= 2 && player->skillCooldown == 0) {
            command.type = CommandType::UseSkill;
            return command;
        }
        command.type = CommandType::Roll;
        return command;
    }

    const auto *tile = state.tileAt(player->position);
    const auto *property = state.propertyAt(player->position);
    if (tile && property && tile->type == TileType::Property) {
        if (property->ownerId.isNull()) {
            const int reserve = m_difficulty == Difficulty::Expert ? 3500 : (m_difficulty == Difficulty::Standard ? 5000 : 7000);
            if (player->cash - tile->price >= reserve) {
                command.type = CommandType::BuyProperty;
                return command;
            }
        } else if (property->ownerId == player->id && property->level < 3 && m_difficulty != Difficulty::Easy) {
            const int cost = tile->price / 2 * (property->level + 1);
            if (player->cash - cost >= 4500) {
                command.type = CommandType::UpgradeProperty;
                command.arguments.insert(QStringLiteral("tile"), tile->index);
                return command;
            }
        }
    }
    if (!player->strategyCards.isEmpty() && m_difficulty != Difficulty::Easy) {
        command.type = CommandType::UseCard;
        command.arguments.insert(QStringLiteral("slot"), 0);
        return command;
    }
    command.type = CommandType::EndTurn;
    return command;
}

} // namespace neon

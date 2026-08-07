#include "core/ai_player.h"

#include <algorithm>

namespace neon {

GameCommand AiPlayer::chooseCommand(const GameState &state, quint64 commandId) const
{
    GameCommand command;
    command.matchId = state.matchId;
    command.commandId = commandId;

    if (state.phase == GamePhase::Auction) {
        const PlayerState *bidder = nullptr;
        for (const auto &player : state.players) {
            if (!player.bankrupt && player.aiControlled && player.id != state.auctionHighBidder
                && !state.auctionPassedPlayers.contains(player.id)) { bidder = &player; break; }
        }
        if (!bidder && !state.auctionHighBidder.isNull()) {
            for (const auto &player : state.players)
                if (!player.bankrupt && player.aiControlled && !state.auctionPassedPlayers.contains(player.id)) {
                    bidder = &player; break;
                }
        }
        if (!bidder) return command;
        command.playerId = bidder->id;
        const auto *tile = state.tileAt(state.auctionTile);
        const int nextBid = state.auctionHighBid + (m_difficulty == Difficulty::Expert ? 300 : 150);
        const int reserve = m_difficulty == Difficulty::Easy ? 7000 : 4500;
        if (tile && nextBid <= tile->price * 115 / 100 && bidder->cash - nextBid >= reserve) {
            command.type = CommandType::PlaceBid;
            command.arguments.insert(QStringLiteral("amount"), nextBid);
        } else command.type = CommandType::PassAuction;
        return command;
    }

    if (state.phase == GamePhase::Trade && state.tradeOffer.active) {
        auto it = std::find_if(state.players.cbegin(), state.players.cend(), [&](const auto &p) {
            return p.id == state.tradeOffer.recipientId && p.aiControlled;
        });
        if (it == state.players.cend()) return command;
        command.playerId = it->id;
        command.type = CommandType::RespondTrade;
        const bool acceptable = state.tradeOffer.requestedCash <= state.tradeOffer.offeredCash + 800;
        command.arguments.insert(QStringLiteral("accept"), acceptable);
        return command;
    }

    const auto *player = state.activePlayer();
    if (!player) return command;
    command.playerId = player->id;
    if (state.phase == GamePhase::AwaitingRoll) {
        if (m_difficulty != Difficulty::Easy && player->energy >= 3 && player->skillCooldown == 0) {
            command.type = CommandType::UseSkill; return command;
        }
        command.type = CommandType::Roll; return command;
    }
    if (state.phase == GamePhase::AwaitingRoute) {
        int best = 0;
        int bestValue = -1;
        for (int i = 0; i < state.routeOptions.size(); ++i) {
            const auto &route = state.routeOptions.at(i);
            const auto *tile = route.isEmpty() ? nullptr : state.tileAt(route.last());
            int value = 0;
            if (tile && tile->type == TileType::Property) {
                const auto *property = state.propertyAt(tile->index);
                value = property && property->ownerId.isNull() ? 10000 - tile->price : 1000;
            } else if (tile && tile->type == TileType::Commission) value = 3500;
            else if (tile && tile->type == TileType::Festival) value = 3000;
            if (value > bestValue) { bestValue = value; best = i; }
        }
        command.type = CommandType::ChooseRoute;
        command.arguments.insert(QStringLiteral("option"), best);
        return command;
    }

    const auto *tile = state.tileAt(player->position);
    const auto *property = state.propertyAt(player->position);
    if (tile && property && tile->type == TileType::Property) {
        if (property->ownerId.isNull()) {
            const int reserve = m_difficulty == Difficulty::Expert ? 4000 : 6000;
            if (player->cash - tile->price >= reserve) { command.type = CommandType::BuyProperty; return command; }
        } else if (property->ownerId == player->id && !property->mortgaged
                   && property->level < 3 && m_difficulty != Difficulty::Easy) {
            const int cost = tile->price / 2 * (property->level + 1);
            if (player->cash - cost >= 5000) {
                command.type = CommandType::UpgradeProperty;
                command.arguments.insert(QStringLiteral("tile"), tile->index);
                return command;
            }
        }
    }
    if (tile && tile->type == TileType::Civic && player->cash >= 7000 && player->livelihood < 90) {
        command.type = CommandType::ContributeCivic; return command;
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

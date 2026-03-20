#pragma once

#include "game/game_state.hpp"

namespace z1m {

void resolve_npc_collisions(const GameState* play, Player* player,
                            const glm::vec2& previous_position);
void tick_npcs(GameState* play, Player* player, float dt_seconds);
void update_npc_messages(GameState* play, const Player* player);

} // namespace z1m

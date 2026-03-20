#pragma once

#include "game/play.hpp"

namespace z1m {

void resolve_npc_collisions(const Play* play, Player* player, const glm::vec2& previous_position);
void tick_npcs(Play* play, Player* player, float dt_seconds);
void update_npc_messages(Play* play, const Player* player);

} // namespace z1m

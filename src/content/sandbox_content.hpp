#pragma once

#include "game/play.hpp"

namespace z1m {

void build_enemy_zoo_world(World* world);
void build_item_zoo_world(World* world);
void populate_sandbox_entities(Play* play);
bool get_enemy_zoo_pen_bounds(int respawn_group, glm::vec2* min_position, glm::vec2* max_position);
int gather_sandbox_portals(const Play* play, std::array<AreaPortal, kMaxAreaPortals>* portals);

} // namespace z1m

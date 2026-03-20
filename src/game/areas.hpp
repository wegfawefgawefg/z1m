#pragma once

#include "game/play.hpp"

namespace z1m {

void try_enter_overworld_cave(Play* play, const World* overworld_world, Player* player);
void try_exit_cave(Play* play, Player* player);
void try_area_portals(Play* play, Player* player);
const World* get_active_world(const Play* play, const World* overworld_world);
void set_area_kind(Play* play, Player* player, AreaKind area_kind, int cave_id,
                   const glm::vec2& position);
int gather_area_portals(const Play* play, std::array<AreaPortal, kMaxAreaPortals>* portals);
const char* area_name(const Play* play);

} // namespace z1m

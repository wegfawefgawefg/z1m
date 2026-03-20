#pragma once

#include "game/game_state.hpp"

namespace z1m {

void try_enter_overworld_cave(GameState* play, const World* overworld_world, Player* player);
void try_exit_cave(GameState* play, Player* player);
void try_area_portals(GameState* play, Player* player);
const World* get_active_world(const GameState* play, const World* overworld_world);
void set_area_kind(GameState* play, Player* player, AreaKind area_kind, int cave_id,
                   const glm::vec2& position);
int gather_area_portals(const GameState* play, std::array<AreaPortal, kMaxAreaPortals>* portals);
const char* area_name(const GameState* play);

} // namespace z1m

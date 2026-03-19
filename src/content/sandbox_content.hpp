#pragma once

#include "game/game_session.hpp"

namespace z1m {

void build_enemy_zoo_world(World* world);
void build_item_zoo_world(World* world);
void populate_sandbox_entities(GameSession* session);
int gather_sandbox_portals(const GameSession* session,
                           std::array<AreaPortal, kMaxAreaPortals>* portals);

} // namespace z1m

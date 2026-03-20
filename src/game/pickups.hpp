#pragma once

#include "game/play.hpp"

namespace z1m {

void apply_player_pickup(Player* player, Play* play, Pickup* pickup);
void tick_pickups(Play* play, const World* overworld_world, Player* player, float dt_seconds);
void trigger_explosion(Play* play, const Projectile& bomb);
void tick_projectiles(Play* play, const World* overworld_world, Player* player, float dt_seconds);
void compact_vectors(Play* play);

} // namespace z1m

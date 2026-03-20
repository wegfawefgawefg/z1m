#pragma once

#include "game/game_state.hpp"

namespace z1m {

void apply_player_pickup(Player* player, GameState* play, Pickup* pickup);
void tick_pickups(GameState* play, const World* overworld_world, Player* player, float dt_seconds);
void trigger_explosion(GameState* play, const Projectile& bomb);
void tick_projectiles(GameState* play, const World* overworld_world, Player* player,
                      float dt_seconds);
void compact_vectors(GameState* play);

} // namespace z1m

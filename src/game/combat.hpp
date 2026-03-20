#pragma once

#include "game/game_state.hpp"

namespace z1m {

void ensure_sword_cave_pickup(GameState* play);
void init_opening_overworld_enemies(GameState* play);
void spawn_pickup_drop(GameState* play, const Enemy& enemy);
void damage_player_from(GameState* play, const World* world, Player* player, int damage,
                        const glm::vec2& source_position);
void damage_enemy(GameState* play, Enemy* enemy, int damage);
void process_player_attacks(GameState* play, Player* player);

} // namespace z1m

#pragma once

#include "game/play.hpp"

namespace z1m {

void ensure_sword_cave_pickup(Play* play);
void init_opening_overworld_enemies(Play* play);
void spawn_pickup_drop(Play* play, const Enemy& enemy);
void damage_player_from(Play* play, const World* world, Player* player, int damage,
                        const glm::vec2& source_position);
void damage_enemy(Play* play, Enemy* enemy, int damage);
void process_player_attacks(Play* play, Player* player);

} // namespace z1m

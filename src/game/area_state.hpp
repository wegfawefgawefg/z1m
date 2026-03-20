#pragma once

#include "game/play.hpp"

namespace z1m {

void set_message(Play* play, const std::string& text, float seconds);
Projectile* find_active_food(Play* play, AreaKind area_kind, int cave_id);
const World* get_world_for_area(const Play* play, const World* overworld_world, AreaKind area_kind,
                                int cave_id);
bool in_area(const Play* play, AreaKind area_kind, int cave_id);
bool enemy_in_current_area(const Play* play, const Enemy& enemy);
bool pickup_in_current_area(const Play* play, const Pickup& pickup);
int get_room_from_position(const glm::vec2& position);
void update_current_room(Play* play, const Player* player);

} // namespace z1m

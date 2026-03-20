#pragma once

#include "game/game_state.hpp"

namespace z1m {

void set_message(GameState* play, const std::string& text, float seconds);
Projectile* find_active_food(GameState* play, AreaKind area_kind, int cave_id);
const World* get_world_for_area(const GameState* play, const World* overworld_world,
                                AreaKind area_kind, int cave_id);
bool in_area(const GameState* play, AreaKind area_kind, int cave_id);
bool enemy_in_current_area(const GameState* play, const Enemy& enemy);
bool pickup_in_current_area(const GameState* play, const Pickup& pickup);
int get_room_from_position(const glm::vec2& position);
void update_current_room(GameState* play, const Player* player);

} // namespace z1m

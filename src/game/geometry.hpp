#pragma once

#include "game/play.hpp"

namespace z1m {

glm::vec2 facing_vector(Facing facing);
float qspeed_to_speed(int qspeed);
bool near_tile_center(const glm::vec2& position);
void snap_to_tile_center(glm::vec2* position);
Facing axis_facing_toward(const glm::vec2& from, const glm::vec2& to, bool vertical);
glm::vec2 eight_way_direction_toward(const glm::vec2& from, const glm::vec2& to);
glm::vec2 flyer_direction_toward(const glm::vec2& from, const glm::vec2& to);
int dir8_index_from_vector(const glm::vec2& direction);
glm::vec2 rotate_dir8_once_toward(const glm::vec2& current, const glm::vec2& target);
glm::vec2 rotate_dir8_random(Play* play, const glm::vec2& current);
bool choose_cardinal_shot_direction(const Play* play, const Enemy& enemy, const Player& player,
                                    Facing* facing_out);
bool overlaps_circle(const glm::vec2& a, const glm::vec2& b, float radius);

} // namespace z1m

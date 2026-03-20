#pragma once

#include "game/play.hpp"

namespace z1m {

void clamp_enemy_to_zoo_pen(Enemy* enemy);
void reset_enemy_state(Play* play, Enemy* enemy);

} // namespace z1m

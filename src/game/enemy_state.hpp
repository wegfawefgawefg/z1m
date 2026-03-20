#pragma once

#include "game/game_state.hpp"

namespace z1m {

void clamp_enemy_to_zoo_pen(Enemy* enemy);
void reset_enemy_state(GameState* play, Enemy* enemy);

} // namespace z1m

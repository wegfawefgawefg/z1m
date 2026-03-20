#pragma once

#include "game/game_state.hpp"

namespace z1m {

std::uint32_t next_random(GameState* play);
float random_unit(GameState* play);
int random_int(GameState* play, int max_value);

} // namespace z1m

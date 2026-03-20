#pragma once

#include "game/play.hpp"

namespace z1m {

std::uint32_t next_random(Play* play);
float random_unit(Play* play);
int random_int(Play* play, int max_value);

} // namespace z1m

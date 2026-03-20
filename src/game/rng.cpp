#include "game/rng.hpp"

namespace z1m {

std::uint32_t next_random(GameState* play) {
    play->rng_state = play->rng_state * 1664525U + 1013904223U;
    return play->rng_state;
}

float random_unit(GameState* play) {
    const std::uint32_t value = next_random(play) >> 8;
    return static_cast<float>(value & 0x00FFFFFFU) / static_cast<float>(0x01000000U);
}

int random_int(GameState* play, int max_value) {
    if (max_value <= 0) {
        return 0;
    }

    return static_cast<int>(next_random(play) % static_cast<std::uint32_t>(max_value));
}

} // namespace z1m

#pragma once

#include <glm/vec2.hpp>

namespace z1m {

struct InputState {
    glm::vec2 move_axis = glm::vec2(0.0F, 0.0F);
    bool attack_pressed = false;
    bool quit_requested = false;
};

} // namespace z1m

#pragma once

#include <glm/vec2.hpp>

namespace z1m {

struct Camera {
    glm::vec2 center_world = glm::vec2(0.0F, 0.0F);
    glm::vec2 viewport_pixels = glm::vec2(0.0F, 0.0F);
    float pixels_per_world_unit = 48.0F;
};

Camera make_camera(const glm::vec2& center_world, const glm::vec2& viewport_pixels, float zoom);
void clamp_camera_to_world(Camera* camera, const glm::vec2& world_size);
glm::vec2 world_to_screen(const Camera* camera, const glm::vec2& world_position);

} // namespace z1m

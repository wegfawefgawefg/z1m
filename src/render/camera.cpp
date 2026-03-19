#include "render/camera.hpp"

namespace z1m {

Camera make_camera(const glm::vec2& center_world, const glm::vec2& viewport_pixels, float zoom) {
    Camera camera;
    camera.center_world = center_world;
    camera.viewport_pixels = viewport_pixels;
    camera.pixels_per_world_unit = 48.0F * zoom;
    return camera;
}

glm::vec2 world_to_screen(const Camera* camera, const glm::vec2& world_position) {
    const glm::vec2 viewport_center = camera->viewport_pixels * 0.5F;
    return (world_position - camera->center_world) * camera->pixels_per_world_unit +
           viewport_center;
}

} // namespace z1m

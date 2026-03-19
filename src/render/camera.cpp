#include "render/camera.hpp"

#include <glm/common.hpp>

namespace z1m {

Camera make_camera(const glm::vec2& center_world, const glm::vec2& viewport_pixels, float zoom) {
    Camera camera;
    camera.center_world = center_world;
    camera.viewport_pixels = viewport_pixels;
    camera.pixels_per_world_unit = 48.0F * zoom;
    return camera;
}

void clamp_camera_to_world(Camera* camera, const glm::vec2& world_size) {
    const glm::vec2 half_view =
        (camera->viewport_pixels * 0.5F) / glm::max(camera->pixels_per_world_unit, 0.001F);

    if (world_size.x <= half_view.x * 2.0F) {
        camera->center_world.x = world_size.x * 0.5F;
    } else {
        camera->center_world.x =
            glm::clamp(camera->center_world.x, half_view.x, world_size.x - half_view.x);
    }

    if (world_size.y <= half_view.y * 2.0F) {
        camera->center_world.y = world_size.y * 0.5F;
    } else {
        camera->center_world.y =
            glm::clamp(camera->center_world.y, half_view.y, world_size.y - half_view.y);
    }
}

glm::vec2 world_to_screen(const Camera* camera, const glm::vec2& world_position) {
    const glm::vec2 viewport_center = camera->viewport_pixels * 0.5F;
    return (world_position - camera->center_world) * camera->pixels_per_world_unit +
           viewport_center;
}

} // namespace z1m

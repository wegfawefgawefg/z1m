#include "game/world.hpp"

#include <cmath>

namespace z1m {

namespace {

bool is_border_wall(int x, int y) {
    return x == 0 || y == 0;
}

bool is_water_band(int x, int y) {
    return y == 9 && x > 7 && x < 22;
}

bool is_ruin_wall(int x, int y) {
    return x > 27 && x < 35 && y > 6 && y < 16 && (x == 28 || x == 34 || y == 7 || y == 15);
}

bool is_trees(int x, int y) {
    return y > 20 && x > 6 && x < 18 && (x + y) % 3 == 0;
}

bool is_rock_patch(int x, int y) {
    return y > 18 && y < 24 && x > 25 && x < 40 && (x + 2 * y) % 4 == 0;
}

} // namespace

World make_world() {
    return World{};
}

int world_width(const World* world) {
    return world->width;
}

int world_height(const World* world) {
    return world->height;
}

bool world_is_walkable_tile(const World* world, int x, int y) {
    return world_tile_at(world, x, y) == TileKind::Ground;
}

bool world_is_walkable_tile(const World* world, const glm::vec2& position) {
    const int tile_x = static_cast<int>(std::floor(position.x));
    const int tile_y = static_cast<int>(std::floor(position.y));
    return world_is_walkable_tile(world, tile_x, tile_y);
}

TileKind world_tile_at(const World* world, int x, int y) {
    if (x < 0 || y < 0 || x >= world->width || y >= world->height) {
        return TileKind::Wall;
    }

    if (is_border_wall(x, y) || x == world->width - 1 || y == world->height - 1) {
        return TileKind::Wall;
    }

    if (is_water_band(x, y)) {
        return TileKind::Water;
    }

    if (is_ruin_wall(x, y)) {
        return TileKind::Wall;
    }

    if (is_trees(x, y)) {
        return TileKind::Tree;
    }

    if (is_rock_patch(x, y)) {
        return TileKind::Rock;
    }

    return TileKind::Ground;
}

} // namespace z1m

#pragma once

#include <glm/vec2.hpp>

namespace z1m {

enum class TileKind {
    Ground,
    Wall,
    Water,
    Tree,
    Rock,
};

struct World {
    int width = 48;
    int height = 33;
};

World make_world();
int world_width(const World* world);
int world_height(const World* world);
bool world_is_walkable_tile(const World* world, int x, int y);
bool world_is_walkable_tile(const World* world, const glm::vec2& position);
TileKind world_tile_at(const World* world, int x, int y);

} // namespace z1m

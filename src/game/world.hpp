#pragma once

#include "content/world_data.hpp"

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
    int width = kWorldTileWidth;
    int height = kWorldTileHeight;
    WorldData overworld = {};
};

World make_world();
int world_width(const World* world);
int world_height(const World* world);
bool world_is_walkable_tile(const World* world, int x, int y);
bool world_is_walkable_tile(const World* world, const glm::vec2& position);
TileKind world_tile_at(const World* world, int x, int y);
bool load_world_overworld(const char* path, World* world);
int get_room_id_at_world_tile(int tile_x, int tile_y);

} // namespace z1m

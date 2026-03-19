#pragma once

#include "content/world_data.hpp"

#include <cstdint>
#include <glm/vec2.hpp>
#include <vector>

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
    bool use_custom_tiles = false;
    std::vector<TileKind> custom_tiles = {};
};

World make_world();
int world_width(const World* world);
int world_height(const World* world);
void resize_world(World* world, int width, int height);
void fill_world(World* world, TileKind tile_kind);
void set_world_tile(World* world, int x, int y, TileKind tile_kind);
void fill_world_rect(World* world, int x, int y, int width, int height, TileKind tile_kind);
bool world_is_walkable_tile(const World* world, int x, int y);
bool world_is_walkable_tile(const World* world, const glm::vec2& position);
TileKind world_tile_at(const World* world, int x, int y);
bool load_world_overworld(const char* path, World* world);
int get_room_id_at_world_tile(int tile_x, int tile_y);

} // namespace z1m

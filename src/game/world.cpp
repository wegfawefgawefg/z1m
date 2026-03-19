#include "game/world.hpp"

#include <algorithm>
#include <cmath>

namespace z1m {

World make_world() {
    return World{};
}

int world_width(const World* world) {
    return world->width;
}

int world_height(const World* world) {
    return world->height;
}

void resize_world(World* world, int width, int height) {
    world->width = width;
    world->height = height;
    world->use_custom_tiles = true;
    world->overworld.loaded = false;
    world->custom_tiles.assign(static_cast<std::size_t>(width * height), TileKind::Ground);
}

void fill_world(World* world, TileKind tile_kind) {
    if (!world->use_custom_tiles) {
        return;
    }

    std::fill(world->custom_tiles.begin(), world->custom_tiles.end(), tile_kind);
}

void set_world_tile(World* world, int x, int y, TileKind tile_kind) {
    if (!world->use_custom_tiles) {
        return;
    }

    if (x < 0 || y < 0 || x >= world->width || y >= world->height) {
        return;
    }

    const std::size_t index = static_cast<std::size_t>(y * world->width + x);
    world->custom_tiles[index] = tile_kind;
}

void fill_world_rect(World* world, int x, int y, int width, int height, TileKind tile_kind) {
    for (int tile_y = y; tile_y < y + height; ++tile_y) {
        for (int tile_x = x; tile_x < x + width; ++tile_x) {
            set_world_tile(world, tile_x, tile_y, tile_kind);
        }
    }
}

bool world_is_walkable_tile(const World* world, int x, int y) {
    if (x < 0 || y < 0 || x >= world->width || y >= world->height) {
        return false;
    }

    switch (world_tile_at(world, x, y)) {
    case TileKind::Ground:
        return true;
    case TileKind::Wall:
    case TileKind::Water:
    case TileKind::Tree:
    case TileKind::Rock:
        return false;
    }

    return false;
}

bool world_is_walkable_tile(const World* world, const glm::vec2& position) {
    const int tile_x = static_cast<int>(std::floor(position.x));
    const int tile_y = static_cast<int>(std::floor(position.y));
    return world_is_walkable_tile(world, tile_x, tile_y);
}

TileKind world_tile_at(const World* world, int x, int y) {
    if (world->use_custom_tiles) {
        if (x < 0 || y < 0 || x >= world->width || y >= world->height) {
            return TileKind::Wall;
        }

        const std::size_t index = static_cast<std::size_t>(y * world->width + x);
        return world->custom_tiles[index];
    }

    if (world->overworld.loaded) {
        const std::uint8_t tile = get_world_tile(&world->overworld, x, y);
        if (tile == 0x24 || tile == 0x6F) {
            return TileKind::Ground;
        }

        if (tile == 0x74 || tile == 0x75 || tile == 0x76 || tile == 0x77) {
            return TileKind::Water;
        }

        if (tile >= 0xBC && tile <= 0xDE) {
            return TileKind::Wall;
        }

        if (tile >= 0xE5 && tile <= 0xEA) {
            return TileKind::Tree;
        }

        if ((tile >= 0x78 && tile <= 0x80) || (tile >= 0x84 && tile <= 0x98)) {
            return TileKind::Rock;
        }

        return TileKind::Ground;
    }

    if (x < 0 || y < 0 || x >= world->width || y >= world->height) {
        return TileKind::Wall;
    }

    if (x == 0 || y == 0 || x == world->width - 1 || y == world->height - 1) {
        return TileKind::Wall;
    }

    return TileKind::Ground;
}

bool load_world_overworld(const char* path, World* world) {
    if (!load_world_data_text(path, &world->overworld)) {
        return false;
    }

    world->width = kWorldTileWidth;
    world->height = kWorldTileHeight;
    world->use_custom_tiles = false;
    world->custom_tiles.clear();
    return true;
}

int get_room_id_at_world_tile(int tile_x, int tile_y) {
    if (tile_x < 0 || tile_y < 0 || tile_x >= kWorldTileWidth || tile_y >= kWorldTileHeight) {
        return -1;
    }

    const int screen_x = tile_x / kScreenTileWidth;
    const int screen_y = tile_y / kScreenTileHeight;
    return screen_y * kScreenColumns + screen_x;
}

} // namespace z1m

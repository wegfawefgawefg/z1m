#pragma once

#include <array>
#include <cstdint>

namespace z1m {

constexpr int kScreenCount = 128;
constexpr int kScreenColumns = 16;
constexpr int kScreenRows = 8;
constexpr int kScreenTileWidth = 32;
constexpr int kScreenTileHeight = 22;
constexpr int kScreenTileCount = kScreenTileWidth * kScreenTileHeight;
constexpr int kWorldTileWidth = kScreenColumns * kScreenTileWidth;
constexpr int kWorldTileHeight = kScreenRows * kScreenTileHeight;

struct WorldScreen {
    std::uint8_t room_id = 0;
    std::uint8_t unique_room_id = 0;
    std::array<std::uint8_t, 6> attrs = {};
    std::array<std::uint8_t, kScreenTileCount> tiles = {};
};

struct WorldData {
    bool loaded = false;
    std::array<WorldScreen, kScreenCount> screens = {};
};

bool load_world_data_text(const char* path, WorldData* world_data);
const WorldScreen* get_world_screen(const WorldData* world_data, int room_id);
std::uint8_t get_world_tile(const WorldData* world_data, int tile_x, int tile_y);

} // namespace z1m

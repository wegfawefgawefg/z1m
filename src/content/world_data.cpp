#include "content/world_data.hpp"

#include <cstddef>
#include <fstream>
#include <string>

namespace z1m {

bool load_world_data_text(const char* path, WorldData* world_data) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return false;
    }

    std::string token;
    std::string version;
    input >> token >> version;
    if (token != "z1m_overworld_q1" || version != "1") {
        return false;
    }

    int screen_count = 0;
    int screen_columns = 0;
    int screen_rows = 0;
    int screen_tile_width = 0;
    int screen_tile_height = 0;

    input >> token >> screen_count;
    input >> token >> screen_columns;
    input >> token >> screen_rows;
    input >> token >> screen_tile_width;
    input >> token >> screen_tile_height;

    if (screen_count != kScreenCount || screen_columns != kScreenColumns ||
        screen_rows != kScreenRows || screen_tile_width != kScreenTileWidth ||
        screen_tile_height != kScreenTileHeight) {
        return false;
    }

    for (int screen_index = 0; screen_index < kScreenCount; ++screen_index) {
        int room_id = 0;
        input >> token >> std::hex >> room_id >> std::dec;
        if (token != "screen") {
            return false;
        }

        WorldScreen& screen = world_data->screens[static_cast<std::size_t>(screen_index)];
        screen.room_id = static_cast<std::uint8_t>(room_id);

        input >> token;
        if (token != "attrs") {
            return false;
        }

        for (std::uint8_t& attr : screen.attrs) {
            int value = 0;
            input >> std::hex >> value >> std::dec;
            attr = static_cast<std::uint8_t>(value);
        }

        int unique_room_id = 0;
        input >> token >> std::hex >> unique_room_id >> std::dec;
        if (token != "unique_room") {
            return false;
        }
        screen.unique_room_id = static_cast<std::uint8_t>(unique_room_id);

        input >> token;
        if (token != "tiles") {
            return false;
        }

        for (std::uint8_t& tile : screen.tiles) {
            int value = 0;
            input >> std::hex >> value >> std::dec;
            tile = static_cast<std::uint8_t>(value);
        }

        input >> token;
        if (token != "end") {
            return false;
        }
    }

    world_data->loaded = true;
    return true;
}

const WorldScreen* get_world_screen(const WorldData* world_data, int room_id) {
    if (room_id < 0 || room_id >= kScreenCount) {
        return nullptr;
    }

    return &world_data->screens[static_cast<std::size_t>(room_id)];
}

std::uint8_t get_world_tile(const WorldData* world_data, int tile_x, int tile_y) {
    if (!world_data->loaded) {
        return 0;
    }

    if (tile_x < 0 || tile_y < 0 || tile_x >= kWorldTileWidth || tile_y >= kWorldTileHeight) {
        return 0;
    }

    const int screen_x = tile_x / kScreenTileWidth;
    const int screen_y = tile_y / kScreenTileHeight;
    const int screen_id = screen_y * kScreenColumns + screen_x;
    const WorldScreen* screen = get_world_screen(world_data, screen_id);
    if (screen == nullptr) {
        return 0;
    }

    const int local_x = tile_x % kScreenTileWidth;
    const int local_y = tile_y % kScreenTileHeight;
    const int tile_index = local_y * kScreenTileWidth + local_x;
    return screen->tiles[static_cast<std::size_t>(tile_index)];
}

std::uint8_t get_screen_palette_selector(const WorldScreen* screen, int local_tile_x,
                                         int local_tile_y) {
    if (screen == nullptr) {
        return 0;
    }

    if (local_tile_x < 0 || local_tile_y < 0 || local_tile_x >= kScreenTileWidth ||
        local_tile_y >= kScreenTileHeight) {
        return 0;
    }

    const std::uint8_t outer_selector = static_cast<std::uint8_t>(screen->attrs[0] & 0x03);
    const std::uint8_t inner_selector = static_cast<std::uint8_t>(screen->attrs[1] & 0x03);
    const int attr_x = local_tile_x / 4;
    const int attr_y = local_tile_y / 4;
    if (attr_x <= 0 || attr_x >= 7) {
        return outer_selector;
    }

    if (attr_y >= 1 && attr_y <= 3) {
        return inner_selector;
    }

    if (attr_y == 4 && (local_tile_y % 4) < 2) {
        return inner_selector;
    }

    return outer_selector;
}

std::uint8_t get_world_palette_selector(const WorldData* world_data, int tile_x, int tile_y) {
    if (!world_data->loaded) {
        return 0;
    }

    if (tile_x < 0 || tile_y < 0 || tile_x >= kWorldTileWidth || tile_y >= kWorldTileHeight) {
        return 0;
    }

    const int screen_x = tile_x / kScreenTileWidth;
    const int screen_y = tile_y / kScreenTileHeight;
    const int screen_id = screen_y * kScreenColumns + screen_x;
    const WorldScreen* screen = get_world_screen(world_data, screen_id);
    if (screen == nullptr) {
        return 0;
    }

    return get_screen_palette_selector(screen, tile_x % kScreenTileWidth,
                                       tile_y % kScreenTileHeight);
}

} // namespace z1m

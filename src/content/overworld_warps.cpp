#include "content/overworld_warps.hpp"

#include <algorithm>
#include <cstddef>

namespace z1m {

namespace {

bool is_overworld_warp_tile(std::uint8_t tile) {
    return tile == 0x24 || tile == 0x88 || (tile >= 0x70 && tile <= 0x73);
}

OverworldWarpType warp_type_from_attr(std::uint8_t attr_b) {
    const std::uint8_t cave_attr = static_cast<std::uint8_t>(attr_b & 0xFC);
    if (cave_attr == 0x50) {
        return OverworldWarpType::Shortcut;
    }

    if (cave_attr < 0x40) {
        return OverworldWarpType::Dungeon;
    }

    return OverworldWarpType::Cave;
}

glm::vec2 room_origin(int room_id) {
    const float room_x = static_cast<float>((room_id % kScreenColumns) * kScreenTileWidth);
    const float room_y = static_cast<float>((room_id / kScreenColumns) * kScreenTileHeight);
    return glm::vec2(room_x, room_y);
}

glm::vec2 make_trigger_position(int room_id, int tile_x, int tile_y, int tile_width) {
    const glm::vec2 origin = room_origin(room_id);
    const float center_x = static_cast<float>(tile_x) + static_cast<float>(tile_width) * 0.5F;
    const float center_y = static_cast<float>(tile_y) + 13.0F / 16.0F;
    return origin + glm::vec2(center_x, center_y);
}

glm::vec2 make_return_position(int room_id, int tile_x, int tile_y, int tile_width) {
    const glm::vec2 origin = room_origin(room_id);
    const float center_x = static_cast<float>(tile_x) + static_cast<float>(tile_width) * 0.5F;
    const float base_y = static_cast<float>(tile_y) + 5.4F;
    const float local_y = std::min(base_y, static_cast<float>(kScreenTileHeight) - 2.0F);
    return origin + glm::vec2(center_x, local_y);
}

glm::vec2 make_trigger_half_size(int tile_width) {
    const float half_width = std::max(0.28F, static_cast<float>(tile_width) * 0.5F - 0.12F);
    return glm::vec2(half_width, 0.22F);
}

void push_hidden_warp(std::array<OverworldWarp, kMaxRoomWarps>* warps, int* count, int room_id,
                      int cave_id, OverworldWarpType type) {
    if (*count >= kMaxRoomWarps) {
        return;
    }

    OverworldWarp& warp = (*warps)[static_cast<std::size_t>(*count)];
    warp = OverworldWarp{};
    warp.room_id = room_id;
    warp.cave_id = cave_id;
    warp.type = OverworldWarpType::Hidden;
    warp.visible = false;
    warp.uses_stairs = false;
    warp.tile = 0;
    warp.local_tile_x = 0;
    warp.local_tile_y = 0;
    warp.tile_width = 0;
    warp.trigger_position = room_origin(room_id) + glm::vec2(16.0F, 10.0F);
    warp.trigger_half_size = glm::vec2(0.0F, 0.0F);
    warp.return_position = room_origin(room_id) + glm::vec2(16.0F, 10.0F);
    if (type == OverworldWarpType::Shortcut || type == OverworldWarpType::Dungeon ||
        type == OverworldWarpType::Cave) {
        warp.type = type;
    }
    warp.type = OverworldWarpType::Hidden;
    ++(*count);
}

void push_visible_warp(std::array<OverworldWarp, kMaxRoomWarps>* warps, int* count,
                       const WorldScreen* screen, int room_id, int tile_x, int tile_y,
                       int tile_width) {
    if (*count >= kMaxRoomWarps) {
        return;
    }

    OverworldWarp& warp = (*warps)[static_cast<std::size_t>(*count)];
    warp = OverworldWarp{};
    warp.room_id = room_id;
    warp.cave_id = static_cast<int>((screen->attrs[1] & 0xFC) >> 2);
    warp.type = warp_type_from_attr(screen->attrs[1]);
    warp.visible = true;
    warp.tile = screen->tiles[static_cast<std::size_t>(tile_y * kScreenTileWidth + tile_x)];
    warp.uses_stairs = warp.tile >= 0x70 && warp.tile <= 0x73;
    warp.local_tile_x = tile_x;
    warp.local_tile_y = tile_y;
    warp.tile_width = tile_width;
    warp.trigger_position = make_trigger_position(room_id, tile_x, tile_y, tile_width);
    warp.trigger_half_size = make_trigger_half_size(tile_width);
    warp.return_position = make_return_position(room_id, tile_x, tile_y, tile_width);
    ++(*count);
}

} // namespace

int get_room_cave_index(const WorldData* world_data, int room_id) {
    const WorldScreen* screen = get_world_screen(world_data, room_id);
    if (screen == nullptr) {
        return -1;
    }

    const std::uint8_t cave_attr = static_cast<std::uint8_t>(screen->attrs[1] & 0xFC);
    if (cave_attr == 0) {
        return -1;
    }

    return static_cast<int>(cave_attr >> 2);
}

int gather_overworld_warps(const WorldData* world_data, int room_id,
                           std::array<OverworldWarp, kMaxRoomWarps>* warps) {
    warps->fill(OverworldWarp{});

    const WorldScreen* screen = get_world_screen(world_data, room_id);
    if (screen == nullptr) {
        return 0;
    }

    const int cave_id = get_room_cave_index(world_data, room_id);
    if (cave_id < 0) {
        return 0;
    }

    int warp_count = 0;
    for (int tile_y = 0; tile_y < kScreenTileHeight; ++tile_y) {
        for (int tile_x = 0; tile_x < kScreenTileWidth;) {
            const std::uint8_t tile =
                screen->tiles[static_cast<std::size_t>(tile_y * kScreenTileWidth + tile_x)];
            if (!is_overworld_warp_tile(tile)) {
                ++tile_x;
                continue;
            }

            int tile_width = 1;
            while (tile_x + tile_width < kScreenTileWidth) {
                const std::uint8_t next_tile = screen->tiles[static_cast<std::size_t>(
                    tile_y * kScreenTileWidth + tile_x + tile_width)];
                if (next_tile != tile) {
                    break;
                }

                ++tile_width;
            }

            push_visible_warp(warps, &warp_count, screen, room_id, tile_x, tile_y, tile_width);
            tile_x += tile_width;
        }
    }

    if (warp_count == 0) {
        push_hidden_warp(warps, &warp_count, room_id, cave_id,
                         warp_type_from_attr(screen->attrs[1]));
    }

    return warp_count;
}

const OverworldWarp* find_triggered_overworld_warp(const WorldData* world_data, int room_id,
                                                   const glm::vec2& world_position,
                                                   std::array<OverworldWarp, kMaxRoomWarps>* warps,
                                                   int* warp_count) {
    *warp_count = gather_overworld_warps(world_data, room_id, warps);

    for (int index = 0; index < *warp_count; ++index) {
        const OverworldWarp& warp = (*warps)[static_cast<std::size_t>(index)];
        if (!warp.visible) {
            continue;
        }

        const glm::vec2 delta = world_position - warp.trigger_position;
        if (delta.x < -warp.trigger_half_size.x || delta.x > warp.trigger_half_size.x) {
            continue;
        }

        if (delta.y < -warp.trigger_half_size.y || delta.y > warp.trigger_half_size.y) {
            continue;
        }

        return &warp;
    }

    return nullptr;
}

const char* overworld_warp_type_name(OverworldWarpType type) {
    switch (type) {
    case OverworldWarpType::Cave:
        return "cave";
    case OverworldWarpType::Dungeon:
        return "dungeon";
    case OverworldWarpType::Shortcut:
        return "shortcut";
    case OverworldWarpType::Hidden:
        return "hidden";
    }

    return "unknown";
}

} // namespace z1m

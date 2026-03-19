#pragma once

#include "content/world_data.hpp"

#include <array>
#include <glm/vec2.hpp>

namespace z1m {

enum class OverworldWarpType {
    Cave,
    Dungeon,
    Shortcut,
    Hidden,
};

struct OverworldWarp {
    int room_id = -1;
    int cave_id = -1;
    OverworldWarpType type = OverworldWarpType::Cave;
    bool visible = false;
    bool uses_stairs = false;
    std::uint8_t tile = 0;
    int local_tile_x = 0;
    int local_tile_y = 0;
    int tile_width = 0;
    glm::vec2 trigger_position = glm::vec2(0.0F, 0.0F);
    glm::vec2 trigger_half_size = glm::vec2(0.0F, 0.0F);
    glm::vec2 return_position = glm::vec2(0.0F, 0.0F);
};

constexpr int kMaxRoomWarps = 4;

int get_room_cave_index(const WorldData* world_data, int room_id);
int gather_overworld_warps(const WorldData* world_data, int room_id,
                           std::array<OverworldWarp, kMaxRoomWarps>* warps);
const OverworldWarp* find_triggered_overworld_warp(const WorldData* world_data, int room_id,
                                                   const glm::vec2& world_position,
                                                   std::array<OverworldWarp, kMaxRoomWarps>* warps,
                                                   int* warp_count);
const char* overworld_warp_type_name(OverworldWarpType type);

} // namespace z1m

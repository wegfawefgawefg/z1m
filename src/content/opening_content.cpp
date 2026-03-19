#include "content/opening_content.hpp"

#include "content/world_data.hpp"

#include <array>

namespace z1m {

namespace {

constexpr int kStartRoomId = 0x77;
constexpr int kSwordCaveId = 0x10;

constexpr std::array<CaveDef, 1> kCaves = {
    CaveDef{
        .cave_id = kSwordCaveId,
        .player_spawn = glm::vec2(8.0F, 8.8F),
        .exit_center = glm::vec2(8.0F, 9.3F),
        .exit_half_size = glm::vec2(2.0F, 0.8F),
        .reward_position = glm::vec2(8.0F, 4.7F),
    },
};

constexpr std::array<EnemySpawnDef, 14> kEnemySpawns = {
    EnemySpawnDef{.room_id = 0x67, .local_position = glm::vec2(10.0F, 8.0F)},
    EnemySpawnDef{.room_id = 0x67, .local_position = glm::vec2(18.0F, 8.5F)},
    EnemySpawnDef{.room_id = 0x67, .local_position = glm::vec2(24.0F, 10.0F)},
    EnemySpawnDef{.room_id = 0x68, .local_position = glm::vec2(9.0F, 9.0F)},
    EnemySpawnDef{.room_id = 0x68, .local_position = glm::vec2(16.0F, 7.5F)},
    EnemySpawnDef{.room_id = 0x68, .local_position = glm::vec2(23.0F, 10.0F)},
    EnemySpawnDef{.room_id = 0x76, .local_position = glm::vec2(8.0F, 9.5F)},
    EnemySpawnDef{.room_id = 0x76, .local_position = glm::vec2(16.0F, 8.5F)},
    EnemySpawnDef{.room_id = 0x76, .local_position = glm::vec2(23.0F, 10.0F)},
    EnemySpawnDef{.room_id = 0x78, .local_position = glm::vec2(8.0F, 10.0F)},
    EnemySpawnDef{.room_id = 0x78, .local_position = glm::vec2(16.0F, 9.0F)},
    EnemySpawnDef{.room_id = 0x78, .local_position = glm::vec2(24.0F, 10.5F)},
    EnemySpawnDef{.room_id = 0x79, .local_position = glm::vec2(11.0F, 8.5F)},
    EnemySpawnDef{.room_id = 0x79, .local_position = glm::vec2(21.0F, 9.5F)},
};

} // namespace

int get_opening_start_room_id() {
    return kStartRoomId;
}

glm::vec2 get_opening_start_position() {
    return make_world_position(kStartRoomId, glm::vec2(15.5F, 10.2F));
}

glm::vec2 make_world_position(int room_id, const glm::vec2& local_position) {
    const float room_x = static_cast<float>((room_id % kScreenColumns) * kScreenTileWidth);
    const float room_y = static_cast<float>((room_id / kScreenColumns) * kScreenTileHeight);
    return glm::vec2(room_x, room_y) + local_position;
}

const CaveDef* get_cave_def(int cave_id) {
    for (const CaveDef& cave : kCaves) {
        if (cave.cave_id == cave_id) {
            return &cave;
        }
    }

    return nullptr;
}

const EnemySpawnDef* get_room_enemy_spawns(int room_id, int* spawn_count) {
    int first_index = -1;
    int count = 0;

    for (int index = 0; index < static_cast<int>(kEnemySpawns.size()); ++index) {
        if (kEnemySpawns[static_cast<std::size_t>(index)].room_id != room_id) {
            continue;
        }

        if (first_index < 0) {
            first_index = index;
        }

        ++count;
    }

    *spawn_count = count;
    if (first_index < 0) {
        return nullptr;
    }

    return &kEnemySpawns[static_cast<std::size_t>(first_index)];
}

} // namespace z1m

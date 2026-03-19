#pragma once

#include <glm/vec2.hpp>

namespace z1m {

struct EnemySpawnDef {
    int room_id = -1;
    glm::vec2 local_position = glm::vec2(0.0F, 0.0F);
};

struct CaveDef {
    int cave_id = -1;
    glm::vec2 player_spawn = glm::vec2(0.0F, 0.0F);
    glm::vec2 exit_center = glm::vec2(0.0F, 0.0F);
    glm::vec2 exit_half_size = glm::vec2(0.0F, 0.0F);
    glm::vec2 reward_position = glm::vec2(0.0F, 0.0F);
};

int get_opening_start_room_id();
glm::vec2 get_opening_start_position();
const CaveDef* get_cave_def(int cave_id);
const EnemySpawnDef* get_room_enemy_spawns(int room_id, int* spawn_count);
glm::vec2 make_world_position(int room_id, const glm::vec2& local_position);

} // namespace z1m

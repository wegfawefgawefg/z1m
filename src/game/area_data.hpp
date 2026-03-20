#pragma once

#include <glm/vec2.hpp>

namespace z1m {

enum class AreaKind {
    Overworld,
    Cave,
    EnemyZoo,
    ItemZoo,
};

struct AreaPortal {
    AreaKind source_area_kind = AreaKind::Overworld;
    int source_cave_id = -1;
    glm::vec2 center = glm::vec2(0.0F, 0.0F);
    glm::vec2 half_size = glm::vec2(0.0F, 0.0F);
    AreaKind target_area_kind = AreaKind::Overworld;
    int target_cave_id = -1;
    glm::vec2 target_position = glm::vec2(0.0F, 0.0F);
    bool requires_raft = false;
    const char* label = "";
};

struct RoomRuntime {
    bool cleared = false;
};

constexpr int kMaxAreaPortals = 8;

} // namespace z1m

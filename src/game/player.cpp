#include "game/player.hpp"

#include "game/world.hpp"

#include <array>
#include <cmath>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace z1m {

namespace {

constexpr float kMoveSpeedTilesPerSecond = 4.5F;
constexpr float kCollisionRadius = 0.28F;
constexpr float kSwordDurationSeconds = 0.16F;

glm::vec2 normalize_move_axis(const glm::vec2& move_axis) {
    const float length = glm::length(move_axis);
    if (length <= 0.0F) {
        return glm::vec2(0.0F, 0.0F);
    }

    if (length <= 1.0F) {
        return move_axis;
    }

    return move_axis / length;
}

bool can_occupy(const World* world, const glm::vec2& position) {
    const std::array<glm::vec2, 4> offsets = {
        glm::vec2(-kCollisionRadius, -kCollisionRadius),
        glm::vec2(kCollisionRadius, -kCollisionRadius),
        glm::vec2(-kCollisionRadius, kCollisionRadius),
        glm::vec2(kCollisionRadius, kCollisionRadius),
    };

    for (const glm::vec2& offset : offsets) {
        if (!world_is_walkable_tile(world, position + offset)) {
            return false;
        }
    }

    return true;
}

void face_direction(Player* player, const glm::vec2& move_axis) {
    if (move_axis.x < 0.0F) {
        player->facing = Facing::Left;
        return;
    }

    if (move_axis.x > 0.0F) {
        player->facing = Facing::Right;
        return;
    }

    if (move_axis.y < 0.0F) {
        player->facing = Facing::Up;
        return;
    }

    if (move_axis.y > 0.0F) {
        player->facing = Facing::Down;
    }
}

void move_with_collision(Player* player, const World* world, const glm::vec2& move_delta) {
    const glm::vec2 candidate_x = player->position + glm::vec2(move_delta.x, 0.0F);
    if (can_occupy(world, candidate_x)) {
        player->position.x = candidate_x.x;
    }

    const glm::vec2 candidate_y = player->position + glm::vec2(0.0F, move_delta.y);
    if (can_occupy(world, candidate_y)) {
        player->position.y = candidate_y.y;
    }
}

} // namespace

Player make_player() {
    return Player{};
}

void tick_player(Player* player, const World* world, const PlayerCommand* command,
                 float dt_seconds) {
    if (player->sword_seconds_remaining > 0.0F) {
        player->sword_seconds_remaining =
            glm::max(0.0F, player->sword_seconds_remaining - dt_seconds);
    }

    const glm::vec2 move_axis = normalize_move_axis(command->move_axis);
    if (move_axis != glm::vec2(0.0F, 0.0F)) {
        face_direction(player, move_axis);
        move_with_collision(player, world, move_axis * kMoveSpeedTilesPerSecond * dt_seconds);
    }

    if (command->attack_pressed) {
        player->sword_seconds_remaining = kSwordDurationSeconds;
    }
}

bool is_sword_active(const Player* player) {
    return player->sword_seconds_remaining > 0.0F;
}

glm::vec2 sword_world_position(const Player* player) {
    glm::vec2 offset(0.0F, 0.0F);

    switch (player->facing) {
    case Facing::Up:
        offset = glm::vec2(0.0F, -0.8F);
        break;
    case Facing::Down:
        offset = glm::vec2(0.0F, 0.8F);
        break;
    case Facing::Left:
        offset = glm::vec2(-0.8F, 0.0F);
        break;
    case Facing::Right:
        offset = glm::vec2(0.8F, 0.0F);
        break;
    }

    return player->position + offset;
}

} // namespace z1m

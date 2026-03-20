#include "game/enemies/state.hpp"
#include "game/enemies/ticks.hpp"
#include "game/geometry.hpp"
#include "game/items.hpp"
#include "game/rng.hpp"
#include "game/tuning.hpp"

#include <array>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace z1m {

void spawn_zol_children(GameState* play, const Enemy& enemy) {
    const bool facing_vertical = enemy.facing == Facing::Up || enemy.facing == Facing::Down;
    const std::array<glm::vec2, 2> offsets = facing_vertical
                                                  ? std::array<glm::vec2, 2>{
                                                        glm::vec2(-0.5F, 0.0F),
                                                        glm::vec2(0.5F, 0.0F),
                                                    }
                                                  : std::array<glm::vec2, 2>{
                                                        glm::vec2(0.0F, -0.5F),
                                                        glm::vec2(0.0F, 0.5F),
                                                    };
    const std::array<Facing, 2> facings = facing_vertical
                                              ? std::array<Facing, 2>{Facing::Left, Facing::Right}
                                              : std::array<Facing, 2>{Facing::Up, Facing::Down};

    for (int index = 0; index < 2; ++index) {
        Enemy child;
        child.active = true;
        child.kind = EnemyKind::Gel;
        child.area_kind = enemy.area_kind;
        child.cave_id = enemy.cave_id;
        child.room_id = enemy.room_id;
        child.position = enemy.position + offsets[static_cast<std::size_t>(index)];
        child.spawn_position = child.position;
        child.origin = child.position;
        child.max_health = 1;
        child.health = 1;
        child.facing = facings[static_cast<std::size_t>(index)];
        child.subtype = 1;
        reset_enemy_state(play, &child);
        play->enemies.push_back(child);
    }
}

void tick_goriya(GameState* play, const World* world, Enemy* enemy, const Player* player,
                 float dt_seconds) {
    tick_rom_goriya_like(play, world, enemy, player, dt_seconds, kGoriyaSpeed,
                         ProjectileKind::Boomerang);
}

float zol_or_gel_edge_delay_seconds(GameState* play, EnemyKind kind) {
    constexpr std::array<int, 4> kZolFrames = {0x18, 0x28, 0x38, 0x48};
    constexpr std::array<int, 4> kGelFrames = {0x08, 0x18, 0x28, 0x38};
    const int index = random_int(play, 4);
    const int frames = kind == EnemyKind::Zol ? kZolFrames[static_cast<std::size_t>(index)]
                                              : kGelFrames[static_cast<std::size_t>(index)];
    return frames_to_seconds(frames);
}

void tick_rom_zol_or_gel(GameState* play, const World* world, Enemy* enemy, const Player* player,
                         float dt_seconds) {
    if (enemy->kind == EnemyKind::Zol && enemy->special_counter == 1) {
        const glm::vec2 candidate =
            enemy->position + facing_vector(enemy->facing) * qspeed_to_speed(0xFF) * dt_seconds;
        if (enemy_can_move_to(enemy, world, candidate)) {
            enemy->position = candidate;
            return;
        }

        enemy->special_counter = 2;
    }

    if (enemy->kind == EnemyKind::Zol && enemy->special_counter == 2) {
        enemy->active = false;
        spawn_zol_children(play, *enemy);
        return;
    }

    if (enemy->kind == EnemyKind::Gel && enemy->special_counter == 0) {
        enemy->special_counter = 1;
        enemy->action_seconds_remaining = frames_to_seconds(5);
        return;
    }

    if (enemy->kind == EnemyKind::Gel && enemy->special_counter == 1) {
        enemy->action_seconds_remaining =
            glm::max(0.0F, enemy->action_seconds_remaining - dt_seconds);
        const glm::vec2 candidate =
            enemy->position + facing_vector(enemy->facing) * qspeed_to_speed(0xFF) * dt_seconds;
        if (enemy_can_move_to(enemy, world, candidate)) {
            enemy->position = candidate;
        } else {
            enemy->action_seconds_remaining = 0.0F;
        }

        if (enemy->action_seconds_remaining <= 0.0F) {
            snap_to_tile_center(&enemy->position);
            enemy->special_counter = 2;
            enemy->state_seconds_remaining = 0.0F;
        }
        return;
    }

    if (!near_tile_center(enemy->position)) {
        enemy->state_seconds_remaining = 0.0F;
    } else {
        snap_to_tile_center(&enemy->position);
        if (enemy->state_seconds_remaining <= 0.0F && enemy->action_seconds_remaining <= 0.0F) {
            enemy->action_seconds_remaining = zol_or_gel_edge_delay_seconds(play, enemy->kind);
            enemy->state_seconds_remaining = 1.0F;
            return;
        }
    }

    if (enemy->action_seconds_remaining > 0.0F) {
        enemy->action_seconds_remaining =
            glm::max(0.0F, enemy->action_seconds_remaining - dt_seconds);
        return;
    }

    const float speed =
        enemy->kind == EnemyKind::Zol ? qspeed_to_speed(0x18) : qspeed_to_speed(0x40);
    tick_rom_wanderer_shooter(play, world, enemy, player, dt_seconds, speed, 0x20,
                              ProjectileKind::Rock, false, false);
}

void tick_trap(const World* world, Enemy* enemy, const Player* player, float dt_seconds) {
    if (glm::length(enemy->velocity) > 0.0F) {
        const glm::vec2 candidate = enemy->position + enemy->velocity * dt_seconds;
        if (!enemy_can_move_to(enemy, world, candidate)) {
            enemy->velocity *= -1.0F;
        } else {
            enemy->position = candidate;
        }

        if (glm::length(enemy->position - enemy->origin) < 0.4F &&
            glm::dot(enemy->velocity, enemy->origin - enemy->position) <= 0.0F) {
            enemy->position = enemy->origin;
            enemy->velocity = glm::vec2(0.0F);
        }
        return;
    }

    if (player == nullptr) {
        return;
    }

    const glm::vec2 delta = player->position - enemy->position;
    if (std::abs(delta.x) <= kShootAlignThreshold) {
        enemy->velocity = glm::vec2(0.0F, delta.y < 0.0F ? -kTrapSpeed : kTrapSpeed);
    } else if (std::abs(delta.y) <= kShootAlignThreshold) {
        enemy->velocity = glm::vec2(delta.x < 0.0F ? -kTrapSpeed : kTrapSpeed, 0.0F);
    }
}

void tick_armos(GameState* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds) {
    if (enemy->special_counter == 0) {
        if (player != nullptr &&
            overlaps_circle(player->position, enemy->position, kArmosWakeRadius)) {
            enemy->special_counter = 1;
        } else {
            return;
        }
    }

    tick_rom_goriya_movement(play, world, enemy, player, dt_seconds, kArmosSpeed);
}

} // namespace z1m

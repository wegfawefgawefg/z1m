#include "game/enemy_ticks.hpp"
#include "game/geometry.hpp"
#include "game/items.hpp"
#include "game/rng.hpp"
#include "game/tuning.hpp"

#include <array>
#include <cmath>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace z1m {

namespace {

constexpr std::array<glm::vec2, 4> kBlueWizzrobeTeleportOffsets = {
    glm::vec2(-4.0F, -4.0F),
    glm::vec2(4.0F, -4.0F),
    glm::vec2(-4.0F, 4.0F),
    glm::vec2(4.0F, 4.0F),
};
constexpr std::array<glm::vec2, 16> kRedWizzrobeSpawnOffsets = {
    glm::vec2(0.0F, -4.0F),  glm::vec2(0.0F, 4.0F),  glm::vec2(-4.0F, 0.0F),
    glm::vec2(4.0F, 0.0F),   glm::vec2(0.0F, -8.0F), glm::vec2(0.0F, 8.0F),
    glm::vec2(-8.0F, 0.0F),  glm::vec2(8.0F, 0.0F),  glm::vec2(0.0F, -6.0F),
    glm::vec2(0.0F, 6.0F),   glm::vec2(-6.0F, 0.0F), glm::vec2(6.0F, 0.0F),
    glm::vec2(0.0F, -10.0F), glm::vec2(0.0F, 10.0F), glm::vec2(-10.0F, 0.0F),
    glm::vec2(10.0F, 0.0F),
};
constexpr std::array<Facing, 4> kRedWizzrobeFacings = {
    Facing::Down,
    Facing::Up,
    Facing::Right,
    Facing::Left,
};

glm::vec2 blue_wizzrobe_dir_vector(const Enemy& enemy) {
    if (glm::length(enemy.velocity) > 0.001F) {
        return glm::normalize(enemy.velocity);
    }
    return facing_vector(enemy.facing);
}

Facing blue_wizzrobe_axis_facing(const Enemy& enemy) {
    const glm::vec2 dir = blue_wizzrobe_dir_vector(enemy);
    if (std::abs(dir.x) >= std::abs(dir.y)) {
        return dir.x < 0.0F ? Facing::Left : Facing::Right;
    }
    return dir.y < 0.0F ? Facing::Up : Facing::Down;
}

void set_blue_wizzrobe_axis_facing(Enemy* enemy, const Player* player) {
    if (player == nullptr) {
        return;
    }

    const bool vertical_turn = (enemy->special_counter & 0x40) != 0;
    enemy->facing = axis_facing_toward(enemy->position, player->position, vertical_turn);
    enemy->velocity = facing_vector(enemy->facing) * qspeed_to_speed(0x20);
}

void begin_blue_wizzrobe_teleport(Enemy* enemy, const glm::vec2& direction) {
    enemy->velocity = glm::normalize(direction) * qspeed_to_speed(0x40);
    enemy->move_seconds_remaining = 4.0F;
}

bool choose_blue_wizzrobe_teleport_target(GameState* play, const World* world, Enemy* enemy) {
    const int base_index = random_int(play, 4);
    for (int attempt = 0; attempt < 4; ++attempt) {
        const glm::vec2 offset =
            kBlueWizzrobeTeleportOffsets[static_cast<std::size_t>((base_index + attempt) % 4)];
        const glm::vec2 target = enemy->position + offset;
        if (!enemy_can_move_to(enemy, world, target)) {
            continue;
        }

        begin_blue_wizzrobe_teleport(enemy, offset);
        enemy->special_counter ^= 0x40;
        enemy->action_seconds_remaining = 0.0F;
        return true;
    }

    enemy->position.x = std::floor(enemy->position.x) + 0.5F;
    enemy->position.y = std::floor(enemy->position.y) + 0.5F;
    enemy->action_seconds_remaining = frames_to_seconds(0x70 | random_byte(play));
    enemy->velocity = facing_vector(enemy->facing) * qspeed_to_speed(0x20);
    return false;
}

bool try_place_red_wizzrobe(GameState* play, const World* world, Enemy* enemy,
                            const Player* player) {
    if (player == nullptr) {
        return false;
    }

    enemy->facing = kRedWizzrobeFacings[static_cast<std::size_t>(random_int(play, 4))];
    const int base_index = random_int(play, 16);
    for (int attempt = 0; attempt < 16; ++attempt) {
        const glm::vec2 offset =
            kRedWizzrobeSpawnOffsets[static_cast<std::size_t>((base_index + attempt) % 16)];
        glm::vec2 target = player->position + offset;
        target.x = std::floor(target.x) + 0.5F;
        target.y = std::floor(target.y) + 0.5F;
        if (target.y < 2.5F || target.y > static_cast<float>(world_height(world)) - 1.5F) {
            continue;
        }
        if (!enemy_can_move_to(enemy, world, target)) {
            continue;
        }

        enemy->position = target;
        enemy->spawn_position = target;
        enemy->origin = target;
        return true;
    }

    return false;
}

} // namespace

void tick_blue_wizzrobe(GameState* play, const World* world, Enemy* enemy, const Player* player,
                        float dt_seconds) {
    if (enemy->move_seconds_remaining > 0.0F) {
        const glm::vec2 step =
            blue_wizzrobe_dir_vector(*enemy) * qspeed_to_speed(0x40) * dt_seconds;
        const glm::vec2 candidate = enemy->position + step;
        enemy->move_seconds_remaining =
            glm::max(0.0F, enemy->move_seconds_remaining - glm::length(step));
        if (enemy_can_move_to(enemy, world, candidate)) {
            enemy->position = candidate;
        }
        if (enemy->move_seconds_remaining <= 0.0F) {
            enemy->position.x = std::floor(enemy->position.x) + 0.5F;
            enemy->position.y = std::floor(enemy->position.y) + 0.5F;
            enemy->action_seconds_remaining = frames_to_seconds(0x70 | random_byte(play));
            enemy->velocity =
                facing_vector(blue_wizzrobe_axis_facing(*enemy)) * qspeed_to_speed(0x20);
        }
        return;
    }

    enemy->action_seconds_remaining = glm::max(0.0F, enemy->action_seconds_remaining - dt_seconds);
    if (enemy->action_seconds_remaining <= 0.0F ||
        enemy->action_seconds_remaining <= frames_to_seconds(1)) {
        choose_blue_wizzrobe_teleport_target(play, world, enemy);
        return;
    }

    if (enemy->action_seconds_remaining < frames_to_seconds(16)) {
        return;
    }

    enemy->state_seconds_remaining += dt_seconds;
    if (enemy->state_seconds_remaining < frames_to_seconds(2)) {
        return;
    }
    enemy->state_seconds_remaining = 0.0F;
    enemy->special_counter += 1;
    if ((enemy->special_counter & 0x3F) == 0) {
        set_blue_wizzrobe_axis_facing(enemy, player);
    }

    const glm::vec2 candidate =
        enemy->position + facing_vector(enemy->facing) * qspeed_to_speed(0x20) * dt_seconds * 2.0F;
    if (enemy_can_move_to(enemy, world, candidate)) {
        enemy->position = candidate;
    } else {
        begin_blue_wizzrobe_teleport(enemy, facing_vector(enemy->facing));
        enemy->special_counter ^= 0x40;
        return;
    }

    if ((static_cast<int>(std::round(enemy->action_seconds_remaining * 60.0F)) & 0x1F) == 0 &&
        player != nullptr) {
        Facing shot_facing = enemy->facing;
        if (choose_cardinal_shot_direction(play, *enemy, *player, &shot_facing) &&
            shot_facing == enemy->facing) {
            make_projectile(play, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                            enemy->position + facing_vector(enemy->facing) * 0.7F,
                            facing_vector(enemy->facing) * kFireSpeed, kProjectileLifetimeSeconds,
                            kFireRadius, 1);
        }
    }
}

void tick_red_wizzrobe(GameState* play, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds) {
    enemy->action_seconds_remaining = glm::max(0.0F, enemy->action_seconds_remaining - dt_seconds);
    if (enemy->special_counter == 0) {
        if (!try_place_red_wizzrobe(play, world, enemy, player)) {
            return;
        }
        enemy->hidden = false;
        enemy->special_counter = 1;
        enemy->action_seconds_remaining = frames_to_seconds(0x10);
        return;
    }

    if (enemy->special_counter == 1) {
        if (enemy->action_seconds_remaining > 0.0F) {
            return;
        }
        enemy->special_counter = 2;
        enemy->action_seconds_remaining = frames_to_seconds(0x40);
        return;
    }

    if (enemy->special_counter == 2) {
        const int frames_left =
            static_cast<int>(std::round(enemy->action_seconds_remaining * 60.0F));
        if (frames_left == 0x30) {
            make_projectile(play, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                            enemy->position + facing_vector(enemy->facing) * 0.8F,
                            facing_vector(enemy->facing) * kFireSpeed, kProjectileLifetimeSeconds,
                            kFireRadius, 1);
        }
        if (enemy->action_seconds_remaining > 0.0F) {
            return;
        }
        enemy->special_counter = 3;
        enemy->action_seconds_remaining = frames_to_seconds(0x10);
        return;
    }

    if (enemy->action_seconds_remaining > 0.0F) {
        return;
    }

    enemy->hidden = true;
    enemy->special_counter = 0;
}

} // namespace z1m

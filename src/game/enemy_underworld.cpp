#include "content/sandbox_content.hpp"
#include "game/enemy_ticks.hpp"
#include "game/geometry.hpp"
#include "game/items.hpp"
#include "game/rng.hpp"
#include "game/tuning.hpp"

#include <cmath>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace z1m {

namespace {

constexpr float kPolsVoiceWalkSpeed = 3.75F;
constexpr float kPolsVoiceJumpSeconds = 0.55F;
constexpr float kPolsVoiceJumpHeight = 1.2F;

void get_wallmaster_bounds(const Enemy& enemy, const World* world, glm::vec2* min_position,
                           glm::vec2* max_position) {
    *min_position = glm::vec2(0.5F, 0.5F);
    *max_position = glm::vec2(static_cast<float>(world_width(world)) - 0.5F,
                              static_cast<float>(world_height(world)) - 0.5F);

    if (enemy.area_kind == AreaKind::EnemyZoo && enemy.respawn_group >= 0) {
        get_enemy_zoo_pen_bounds(enemy.respawn_group, min_position, max_position);
    }
}

bool try_spawn_wallmaster(const World* world, Enemy* enemy, const Player* player) {
    if (player == nullptr) {
        return false;
    }

    glm::vec2 min_position(0.5F);
    glm::vec2 max_position(0.5F);
    get_wallmaster_bounds(*enemy, world, &min_position, &max_position);

    const float left_distance = std::abs(player->position.x - min_position.x);
    const float right_distance = std::abs(player->position.x - max_position.x);
    const float top_distance = std::abs(player->position.y - min_position.y);
    const float bottom_distance = std::abs(player->position.y - max_position.y);

    if (left_distance <= 0.6F) {
        enemy->position = glm::vec2(min_position.x, player->position.y);
        enemy->velocity = glm::vec2(kWallmasterSpeed, 0.0F);
    } else if (right_distance <= 0.6F) {
        enemy->position = glm::vec2(max_position.x, player->position.y);
        enemy->velocity = glm::vec2(-kWallmasterSpeed, 0.0F);
    } else if (top_distance <= 0.6F) {
        enemy->position = glm::vec2(player->position.x, min_position.y);
        enemy->velocity = glm::vec2(0.0F, kWallmasterSpeed);
    } else if (bottom_distance <= 0.6F) {
        enemy->position = glm::vec2(player->position.x, max_position.y);
        enemy->velocity = glm::vec2(0.0F, -kWallmasterSpeed);
    } else {
        return false;
    }

    enemy->hidden = false;
    enemy->origin = enemy->position;
    enemy->move_seconds_remaining = kWallmasterTravelTiles;
    enemy->action_seconds_remaining = 0.0F;
    enemy->special_counter = 1;
    return true;
}

void begin_pols_voice_jump(const World* world, Enemy* enemy) {
    enemy->special_counter = 1;
    enemy->origin = enemy->position;
    enemy->action_seconds_remaining = 0.0F;

    float target_y = enemy->position.y;
    if (enemy->position.y < 7.5F) {
        target_y += 2.0F;
        enemy->facing = Facing::Down;
    } else if (enemy->position.y > static_cast<float>(world_height(world)) - 3.0F) {
        target_y -= 2.0F;
        enemy->facing = Facing::Up;
    } else if (enemy->facing == Facing::Up) {
        target_y -= 2.0F;
    } else if (enemy->facing == Facing::Down) {
        target_y += 2.0F;
    } else {
        target_y += 2.0F;
        enemy->facing = Facing::Down;
    }

    enemy->state_seconds_remaining =
        glm::clamp(target_y, 0.5F, static_cast<float>(world_height(world)) - 0.5F);
}

void finish_pols_voice_jump(GameState* play, Enemy* enemy) {
    enemy->special_counter = 0;
    enemy->position.y = enemy->state_seconds_remaining;
    snap_to_tile_center(&enemy->position);

    switch (random_int(play, 4)) {
    case 0:
        enemy->facing = Facing::Right;
        break;
    case 1:
        enemy->facing = Facing::Left;
        break;
    case 2:
        enemy->facing = Facing::Down;
        break;
    default:
        enemy->facing = Facing::Up;
        break;
    }

    enemy->move_seconds_remaining = (random_byte(play) & 0x40) != 0 ? 7.0F : 3.0F;
    enemy->action_seconds_remaining = 0.0F;
}

} // namespace

void tick_rope(GameState* play, const World* world, Enemy* enemy, const Player* player,
               float dt_seconds) {
    if (enemy->special_counter == 0) {
        enemy->action_seconds_remaining =
            glm::max(0.0F, enemy->action_seconds_remaining - dt_seconds);
    }

    const float speed = enemy->special_counter == 1 ? qspeed_to_speed(0x60) : qspeed_to_speed(0x20);
    const glm::vec2 candidate = enemy->position + facing_vector(enemy->facing) * speed * dt_seconds;
    if (enemy_can_move_to(enemy, world, candidate)) {
        enemy->position = candidate;
    } else {
        enemy->special_counter = 0;
        enemy->action_seconds_remaining = 0.0F;
    }

    if (!near_tile_center(enemy->position)) {
        return;
    }

    snap_to_tile_center(&enemy->position);
    if (enemy->special_counter == 0 && enemy->action_seconds_remaining <= 0.0F) {
        enemy->action_seconds_remaining = frames_to_seconds(random_byte(play) & 0x3F);
        choose_cardinal_direction(play, world, enemy);
    }

    if (player == nullptr || enemy->special_counter != 0) {
        return;
    }

    const glm::vec2 delta = player->position - enemy->position;
    if (std::abs(delta.x) < 0.5F) {
        enemy->facing = delta.y < 0.0F ? Facing::Up : Facing::Down;
        enemy->special_counter = 1;
    } else if (std::abs(delta.y) < 0.5F) {
        enemy->facing = delta.x < 0.0F ? Facing::Left : Facing::Right;
        enemy->special_counter = 1;
    }
}

void tick_stalfos(GameState* play, const World* world, Enemy* enemy, const Player* player,
                  float dt_seconds) {
    tick_rom_common_wanderer(play, world, enemy, player, dt_seconds, qspeed_to_speed(0x20), 0x80);
}

void tick_gibdo(GameState* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds) {
    tick_rom_common_wanderer(play, world, enemy, player, dt_seconds, qspeed_to_speed(0x20), 0x80);
}

void tick_like_like(GameState* play, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds) {
    if (enemy->special_counter > 0) {
        if (player != nullptr) {
            enemy->position = player->position;
        }
        enemy->action_seconds_remaining += dt_seconds;
        return;
    }

    tick_rom_common_wanderer(play, world, enemy, player, dt_seconds, qspeed_to_speed(0x18), 0x80);
}

void tick_bubble(GameState* play, const World* world, Enemy* enemy, const Player* player,
                 float dt_seconds) {
    tick_rom_common_wanderer(play, world, enemy, player, dt_seconds, qspeed_to_speed(0x40), 0x40);
}

void tick_keese(GameState* play, const World* world, Enemy* enemy, float dt_seconds) {
    tick_rom_flyer(play, world, enemy, nullptr, dt_seconds, kKeeseSpeed, 0xA0, false);
}

void tick_pols_voice(GameState* play, const World* world, Enemy* enemy, float dt_seconds) {
    if (enemy->special_counter == 1) {
        enemy->action_seconds_remaining += dt_seconds;
        const float t = glm::clamp(enemy->action_seconds_remaining / kPolsVoiceJumpSeconds, 0.0F, 1.0F);
        const float arc = std::sin(t * 3.14159265359F) * kPolsVoiceJumpHeight;
        enemy->position.y = glm::mix(enemy->origin.y, enemy->state_seconds_remaining, t) - arc;
        if (t >= 1.0F) {
            finish_pols_voice_jump(play, enemy);
        }
        return;
    }

    const glm::vec2 step = facing_vector(enemy->facing) * kPolsVoiceWalkSpeed * dt_seconds;
    const glm::vec2 candidate = enemy->position + step;
    if (enemy->move_seconds_remaining <= 0.0F || !enemy_can_move_to(enemy, world, candidate)) {
        begin_pols_voice_jump(world, enemy);
        return;
    }

    enemy->position = candidate;
    enemy->move_seconds_remaining = glm::max(0.0F, enemy->move_seconds_remaining - glm::length(step));
}

void tick_wallmaster(GameState* play, const World* world, Enemy* enemy, const Player* player,
                     float dt_seconds) {
    if (enemy->hidden) {
        enemy->action_seconds_remaining = glm::max(0.0F, enemy->action_seconds_remaining - dt_seconds);
        if (enemy->action_seconds_remaining > 0.0F) {
            return;
        }
        if (!try_spawn_wallmaster(world, enemy, player)) {
            enemy->action_seconds_remaining = 0.2F;
        }
        return;
    }

    const glm::vec2 step = enemy->velocity * dt_seconds;
    const glm::vec2 candidate = enemy->position + step;
    if (enemy_can_move_to(enemy, world, candidate)) {
        enemy->position = candidate;
        enemy->move_seconds_remaining = glm::max(0.0F, enemy->move_seconds_remaining - glm::length(step));
    } else {
        enemy->move_seconds_remaining = 0.0F;
    }

    if (enemy->move_seconds_remaining > 0.0F) {
        return;
    }

    enemy->hidden = true;
    enemy->velocity = glm::vec2(0.0F);
    enemy->action_seconds_remaining = 0.8F + random_unit(play) * 0.8F;
    enemy->special_counter = 0;
}

} // namespace z1m

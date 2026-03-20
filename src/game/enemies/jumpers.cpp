#include "game/area_state.hpp"
#include "game/enemies/state.hpp"
#include "game/enemies/ticks.hpp"
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

constexpr std::array<int, 6> kBlueLeeverStateFrames = {0x80, 0x20, 0x0F, 0xFF, 0x10, 0x60};
constexpr std::array<int, 6> kRedLeeverStateFrames = {0x00, 0x10, 0x08, 0xFF, 0x08, 0x10};
constexpr std::array<float, 16> kVireJumpOffsets = {
    0.0F,   -0.1875F, -0.125F, -0.0625F, -0.0625F, 0.0F,   -0.0625F, 0.0F,
    0.0F,    0.0625F,  0.0F,    0.0625F,  0.0625F, 0.125F,  0.1875F, 0.0F,
};

void set_leever_state_frames(Enemy* enemy, const std::array<int, 6>& frames, int state) {
    enemy->special_counter = state;
    enemy->hidden = state != 3;
    enemy->invulnerable = state != 3;
    enemy->action_seconds_remaining = frames_to_seconds(frames[static_cast<std::size_t>(state)]);
}

bool try_place_red_leever(const World* world, GameState* play, Enemy* enemy, const Player* player) {
    if (player == nullptr) {
        return false;
    }

    Facing facing = player->facing;
    if (random_byte(play) >= 0xC0) {
        switch (facing) {
        case Facing::Up:
            facing = Facing::Down;
            break;
        case Facing::Down:
            facing = Facing::Up;
            break;
        case Facing::Left:
            facing = Facing::Right;
            break;
        case Facing::Right:
            facing = Facing::Left;
            break;
        }
    }

    glm::vec2 position = player->position;
    if (facing == Facing::Up || facing == Facing::Down) {
        position.x = std::floor(position.x) + 0.5F;
        position.y += facing == Facing::Down ? kLeeverSpawnOffset : -kLeeverSpawnOffset;
        position.y = std::floor(position.y) + 0.5F;
    } else {
        position.y = std::floor(position.y) + 0.5F;
        position.x += facing == Facing::Right ? kLeeverSpawnOffset : -kLeeverSpawnOffset;
        position.x = std::floor(position.x) + 0.5F;
    }

    position.x = glm::clamp(position.x, 0.5F, static_cast<float>(world_width(world)) - 0.5F);
    position.y = glm::clamp(position.y, 2.5F, static_cast<float>(world_height(world)) - 0.5F);
    if (!enemy_can_move_to(enemy, world, position)) {
        return false;
    }

    enemy->position = position;
    enemy->spawn_position = position;
    enemy->origin = position;
    choose_player_axis_direction(*enemy, *player, &enemy->facing);
    return true;
}

float vire_jump_offset(const Enemy& enemy) {
    const float grid = std::abs(enemy.position.x - (std::floor(enemy.position.x) + 0.5F)) * 16.0F;
    const int index = glm::clamp(static_cast<int>(std::round(grid)), 0, 15);
    return kVireJumpOffsets[static_cast<std::size_t>(index)];
}

void begin_tektite_jump(GameState* play, Enemy* enemy, const Player* player) {
    glm::vec2 target = enemy->position + glm::vec2(0.0F, 1.0F);
    if (player != nullptr) {
        target = player->position;
    }

    glm::vec2 direction = eight_way_direction_toward(enemy->position, target);
    if (std::abs(direction.x) < 0.001F) {
        direction.x = target.x < enemy->position.x ? -1.0F : 1.0F;
        direction = glm::normalize(direction);
    }

    if (enemy->special_counter >= 2) {
        direction.x *= -1.0F;
        direction = glm::normalize(direction);
        enemy->special_counter = 0;
    }

    enemy->velocity = direction * kTektiteHopSpeed;
    enemy->move_seconds_remaining = 0.34F + random_unit(play) * 0.08F;
    enemy->state_seconds_remaining = 0.0F;
}

} // namespace

void tick_ghini(GameState* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds) {
    tick_rom_common_wanderer(play, world, enemy, player, dt_seconds, qspeed_to_speed(0x20), 0xFF);
}

void tick_flying_ghini(GameState* play, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds) {
    tick_rom_flyer(play, world, enemy, player, dt_seconds, kFlyingGhiniSpeed, 0xA0, 0x08, false);
}

void tick_vire(GameState* play, const World* world, Enemy* enemy, const Player* player,
               float dt_seconds) {
    const float base_y = enemy->origin.y <= 0.0F ? enemy->position.y : enemy->origin.y;
    tick_rom_common_wanderer(play, world, enemy, player, dt_seconds, qspeed_to_speed(0x20), 0x80);
    enemy->origin.y = base_y;

    if (enemy->facing == Facing::Left || enemy->facing == Facing::Right) {
        enemy->position.y = base_y + vire_jump_offset(*enemy);
    } else {
        enemy->origin.y = enemy->position.y;
    }
}

void tick_tektite(GameState* play, const World* world, Enemy* enemy, const Player* player,
                  float dt_seconds) {
    if (enemy->move_seconds_remaining > 0.0F) {
        enemy->move_seconds_remaining = glm::max(0.0F, enemy->move_seconds_remaining - dt_seconds);
        const glm::vec2 candidate = enemy->position + enemy->velocity * dt_seconds;
        if (!enemy_can_move_to(enemy, world, candidate)) {
            enemy->special_counter += 1;
            enemy->velocity *= -1.0F;
            begin_tektite_jump(play, enemy, player);
            return;
        }

        enemy->position = candidate;
        if (enemy->move_seconds_remaining <= 0.0F) {
            enemy->velocity = glm::vec2(0.0F);
            enemy->action_seconds_remaining = 0.18F + random_unit(play) * 0.50F;
        }
        return;
    }

    enemy->action_seconds_remaining = glm::max(0.0F, enemy->action_seconds_remaining - dt_seconds);
    if (enemy->action_seconds_remaining > 0.0F) {
        return;
    }

    begin_tektite_jump(play, enemy, player);
}

void tick_rom_flyer(GameState* play, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds, float max_speed, int chase_threshold, int wander_threshold,
                    bool vulnerable_only_in_delay) {
    if (glm::length(enemy->velocity) < 0.001F) {
        enemy->velocity = glm::vec2(1.0F, 0.0F);
    }
    if (enemy->state_seconds_remaining <= 0.0F) {
        enemy->state_seconds_remaining = max_speed * 0.25F;
    }

    enemy->invulnerable = vulnerable_only_in_delay && enemy->special_counter != 5;

    if (enemy->special_counter == 5) {
        enemy->action_seconds_remaining =
            glm::max(0.0F, enemy->action_seconds_remaining - dt_seconds);
        if (enemy->action_seconds_remaining <= 0.0F) {
            enemy->special_counter = 0;
        }
    } else if (enemy->special_counter == 0) {
        enemy->state_seconds_remaining =
            glm::min(max_speed, enemy->state_seconds_remaining + max_speed * 1.8F * dt_seconds);
        if (enemy->state_seconds_remaining >= max_speed) {
            enemy->special_counter = 1;
        }
    } else if (enemy->special_counter == 1) {
        const int roll = random_byte(play);
        enemy->special_counter =
            roll >= chase_threshold ? 2 : (roll >= wander_threshold ? 3 : 4);
        enemy->move_seconds_remaining = 6.0F;
        enemy->action_seconds_remaining = 0.0F;
    } else if (enemy->special_counter == 2 || enemy->special_counter == 3) {
        enemy->action_seconds_remaining =
            glm::max(0.0F, enemy->action_seconds_remaining - dt_seconds);
        if (enemy->action_seconds_remaining <= 0.0F) {
            enemy->move_seconds_remaining -= 1.0F;
            if (enemy->move_seconds_remaining <= 0.0F) {
                enemy->special_counter = 1;
            } else {
                enemy->action_seconds_remaining = frames_to_seconds(16);
                const glm::vec2 current_dir = glm::normalize(enemy->velocity);
                if (enemy->special_counter == 2 && player != nullptr) {
                    const glm::vec2 target_dir =
                        flyer_direction_toward(enemy->position, player->position);
                    enemy->velocity = rotate_dir8_once_toward(current_dir, target_dir);
                } else {
                    enemy->velocity = rotate_dir8_random(play, current_dir);
                }
            }
        }
    } else if (enemy->special_counter == 4) {
        enemy->state_seconds_remaining =
            glm::max(0.0F, enemy->state_seconds_remaining - max_speed * 1.8F * dt_seconds);
        if (enemy->state_seconds_remaining <= max_speed * 0.20F) {
            enemy->special_counter = 5;
            enemy->action_seconds_remaining = frames_to_seconds(0x40 + (random_byte(play) & 0x3F));
        }
    }

    const float speed = glm::max(enemy->state_seconds_remaining, 0.0F);
    if (speed > 0.0F) {
        bounce_velocity(enemy, world, &enemy->position, &enemy->velocity, dt_seconds);
        if (glm::length(enemy->velocity) > 0.001F) {
            enemy->velocity = glm::normalize(enemy->velocity) * speed;
        }
    }
}

void tick_leever(GameState* play, const World* world, Enemy* enemy, const Player* player,
                 float dt_seconds) {
    enemy->action_seconds_remaining = glm::max(0.0F, enemy->action_seconds_remaining - dt_seconds);

    if (enemy->subtype == 0 && enemy->special_counter == 0) {
        if (enemy->action_seconds_remaining > 0.0F) {
            return;
        }

        if (!try_place_red_leever(world, play, enemy, player)) {
            enemy->action_seconds_remaining = frames_to_seconds(20);
            return;
        }

        set_leever_state_frames(enemy, kRedLeeverStateFrames, 1);
        enemy->facing = axis_facing_toward(enemy->position, player->position, false);
        return;
    }

    if (enemy->special_counter != 3) {
        if (enemy->action_seconds_remaining > 0.0F) {
            return;
        }

        int next_state = enemy->special_counter + 1;
        if (next_state >= 6) {
            next_state = 0;
        }

        const auto& frames = enemy->subtype == 0 ? kRedLeeverStateFrames : kBlueLeeverStateFrames;
        set_leever_state_frames(enemy, frames, next_state);
        return;
    }

    if (enemy->subtype != 0 && near_tile_center(enemy->position)) {
        snap_to_tile_center(&enemy->position);
        if (player != nullptr) {
            choose_player_axis_direction(*enemy, *player, &enemy->facing);
        }
    }

    const glm::vec2 candidate =
        enemy->position + facing_vector(enemy->facing) * qspeed_to_speed(0x20) * dt_seconds;
    if (enemy_can_move_to(enemy, world, candidate)) {
        enemy->position = candidate;
    } else {
        enemy->action_seconds_remaining = 0.0F;
    }

    if (enemy->action_seconds_remaining <= 0.0F) {
        const auto& frames = enemy->subtype == 0 ? kRedLeeverStateFrames : kBlueLeeverStateFrames;
        set_leever_state_frames(enemy, frames, 4);
    }
}

} // namespace z1m

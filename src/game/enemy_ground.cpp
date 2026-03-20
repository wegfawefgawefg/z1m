#include "game/area_state.hpp"
#include "game/enemy_state.hpp"
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

constexpr std::array<int, 6> kBlueLeeverStateFrames = {0x80, 0x20, 0x0F, 0xFF, 0x10, 0x60};
constexpr std::array<int, 6> kRedLeeverStateFrames = {0x00, 0x10, 0x08, 0xFF, 0x08, 0x10};
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
    } else {
        position.y = std::floor(position.y) + 0.5F;
        position.x += facing == Facing::Right ? kLeeverSpawnOffset : -kLeeverSpawnOffset;
    }

    position.x = glm::clamp(position.x, 0.5F, static_cast<float>(world_width(world)) - 0.5F);
    position.y = glm::clamp(position.y, 0.5F, static_cast<float>(world_height(world)) - 0.5F);
    if (!enemy_can_move_to(enemy, world, position)) {
        return false;
    }

    enemy->position = position;
    enemy->spawn_position = position;
    enemy->origin = position;
    choose_player_axis_direction(*enemy, *player, &enemy->facing);
    return true;
}

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

    const int facing_index = random_int(play, 4);
    enemy->facing = kRedWizzrobeFacings[static_cast<std::size_t>(facing_index)];
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

void tick_ghini(GameState* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds) {
    tick_rom_common_wanderer(play, world, enemy, player, dt_seconds, qspeed_to_speed(0x20), 0xFF);
}

void tick_flying_ghini(GameState* play, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds) {
    tick_rom_flyer(play, world, enemy, player, dt_seconds, kFlyingGhiniSpeed, 0xA0, false);
}

void tick_vire(GameState* play, const World* world, Enemy* enemy, const Player* player,
               float dt_seconds) {
    enemy->move_seconds_remaining -= dt_seconds;
    if (enemy->move_seconds_remaining <= 0.0F) {
        choose_cardinal_direction(play, world, enemy);
    }

    glm::vec2 candidate = enemy->position + facing_vector(enemy->facing) * kVireSpeed * dt_seconds;
    candidate.y += std::sin(enemy->action_seconds_remaining * 14.0F) * 0.04F;
    if (enemy_can_move_to(enemy, world, candidate)) {
        enemy->position = candidate;
    } else {
        enemy->move_seconds_remaining = 0.0F;
    }
    enemy->action_seconds_remaining += dt_seconds;

    const Projectile* food = find_active_food(play, enemy->area_kind, enemy->cave_id);
    if (food != nullptr && random_unit(play) < dt_seconds * 1.2F) {
        const Player bait_player = Player{.position = food->position};
        choose_player_axis_direction(*enemy, bait_player, &enemy->facing);
    } else if (player != nullptr && random_unit(play) < dt_seconds * 0.6F) {
        choose_player_axis_direction(*enemy, *player, &enemy->facing);
    }
}

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
    if (enemy->action_seconds_remaining <= 0.0F) {
        choose_blue_wizzrobe_teleport_target(play, world, enemy);
        return;
    }

    if (enemy->action_seconds_remaining <= frames_to_seconds(1)) {
        choose_blue_wizzrobe_teleport_target(play, world, enemy);
        return;
    }

    if (enemy->action_seconds_remaining <= frames_to_seconds(16)) {
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
        const bool hit_vertical_wall =
            candidate.x < 0.5F || candidate.x > static_cast<float>(world_width(world)) - 0.5F;
        const bool hit_horizontal_wall =
            candidate.y < 0.5F || candidate.y > static_cast<float>(world_height(world)) - 0.5F;
        if (hit_vertical_wall) {
            if (enemy->facing == Facing::Left) {
                enemy->facing = Facing::Right;
            } else if (enemy->facing == Facing::Right) {
                enemy->facing = Facing::Left;
            }
        } else if (hit_horizontal_wall) {
            if (enemy->facing == Facing::Up) {
                enemy->facing = Facing::Down;
            } else if (enemy->facing == Facing::Down) {
                enemy->facing = Facing::Up;
            }
        } else {
            begin_blue_wizzrobe_teleport(enemy, facing_vector(enemy->facing));
            enemy->special_counter ^= 0x40;
            return;
        }
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

void tick_tektite(GameState* play, const World* world, Enemy* enemy, const Player* player,
                  float dt_seconds) {
    if (enemy->move_seconds_remaining > 0.0F) {
        enemy->move_seconds_remaining = glm::max(0.0F, enemy->move_seconds_remaining - dt_seconds);
        const glm::vec2 previous = enemy->position;
        const glm::vec2 candidate = enemy->position + enemy->velocity * dt_seconds;
        if (!enemy_can_move_to(enemy, world, candidate)) {
            enemy->special_counter += 1;
            enemy->velocity *= -1.0F;
            if (enemy->special_counter >= 2) {
                enemy->velocity.x *= -1.0F;
                enemy->special_counter = 0;
            }
        } else {
            enemy->special_counter = 0;
            enemy->position = candidate;
        }

        if (enemy->move_seconds_remaining <= 0.0F) {
            enemy->action_seconds_remaining = 0.18F + random_unit(play) * 0.50F;
            enemy->velocity = glm::vec2(0.0F);
        } else if (glm::length(enemy->position - previous) <= 0.001F) {
            enemy->move_seconds_remaining = 0.0F;
            enemy->action_seconds_remaining = 0.2F;
        }
        return;
    }

    enemy->action_seconds_remaining = glm::max(0.0F, enemy->action_seconds_remaining - dt_seconds);
    if (enemy->action_seconds_remaining > 0.0F) {
        return;
    }

    glm::vec2 target = enemy->position + glm::vec2(0.0F, 1.0F);
    if (player != nullptr) {
        target = player->position;
    } else {
        target += glm::vec2(random_unit(play) * 2.0F - 1.0F, random_unit(play) * 2.0F - 1.0F);
    }
    glm::vec2 jump_direction = eight_way_direction_toward(enemy->position, target);
    if (std::abs(jump_direction.x) < 0.001F) {
        jump_direction.x = target.x < enemy->position.x ? -1.0F : 1.0F;
        jump_direction = glm::normalize(jump_direction);
    }
    enemy->velocity = jump_direction * kTektiteHopSpeed;
    enemy->move_seconds_remaining = 0.34F + random_unit(play) * 0.08F;
}

void tick_rom_flyer(GameState* play, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds, float max_speed, int chase_threshold,
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
        enemy->special_counter = roll >= chase_threshold ? 2 : (roll >= 0x20 ? 3 : 4);
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

        if (enemy->subtype == 0) {
            set_leever_state_frames(enemy, kRedLeeverStateFrames, next_state);
        } else {
            set_leever_state_frames(enemy, kBlueLeeverStateFrames, next_state);
        }
        return;
    }

    if (enemy->subtype == 0) {
        const glm::vec2 candidate =
            enemy->position + facing_vector(enemy->facing) * qspeed_to_speed(0x20) * dt_seconds;
        if (enemy_can_move_to(enemy, world, candidate)) {
            enemy->position = candidate;
        } else {
            enemy->action_seconds_remaining = 0.0F;
        }
    } else {
        if (near_tile_center(enemy->position)) {
            snap_to_tile_center(&enemy->position);
            if (player != nullptr) {
                choose_player_axis_direction(*enemy, *player, &enemy->facing);
            } else {
                choose_cardinal_direction(play, world, enemy);
            }
        }

        const glm::vec2 candidate =
            enemy->position + facing_vector(enemy->facing) * qspeed_to_speed(0x20) * dt_seconds;
        if (enemy_can_move_to(enemy, world, candidate)) {
            enemy->position = candidate;
        } else {
            enemy->action_seconds_remaining = 0.0F;
        }
    }

    if (enemy->action_seconds_remaining <= 0.0F) {
        if (enemy->subtype == 0) {
            set_leever_state_frames(enemy, kRedLeeverStateFrames, 4);
        } else {
            set_leever_state_frames(enemy, kBlueLeeverStateFrames, 4);
        }
    }
}

} // namespace z1m

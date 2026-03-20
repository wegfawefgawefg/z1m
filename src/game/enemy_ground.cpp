#include "game/area_state.hpp"
#include "game/enemy_state.hpp"
#include "game/enemy_ticks.hpp"
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
        if (world_is_walkable_tile(world, candidate)) {
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
        if (world_is_walkable_tile(world, candidate)) {
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

void tick_rope(GameState* play, const World* world, Enemy* enemy, const Player* player,
               float dt_seconds) {
    if (enemy->special_counter == 0) {
        enemy->action_seconds_remaining =
            glm::max(0.0F, enemy->action_seconds_remaining - dt_seconds);
    }

    const float speed = enemy->special_counter == 1 ? qspeed_to_speed(0x60) : qspeed_to_speed(0x20);
    const glm::vec2 candidate = enemy->position + facing_vector(enemy->facing) * speed * dt_seconds;
    if (world_is_walkable_tile(world, candidate)) {
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
    if (std::abs(delta.x) < kRopeRushAlignThreshold) {
        enemy->facing = delta.y < 0.0F ? Facing::Up : Facing::Down;
        enemy->special_counter = 1;
    } else if (std::abs(delta.y) < kRopeRushAlignThreshold) {
        enemy->facing = delta.x < 0.0F ? Facing::Left : Facing::Right;
        enemy->special_counter = 1;
    }
}

void tick_ghini(GameState* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds) {
    tick_rom_common_wanderer(play, world, enemy, player, dt_seconds, qspeed_to_speed(0x20), 0xFF);
}

void tick_flying_ghini(GameState* play, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds) {
    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining <= 0.0F) {
        glm::vec2 direction(random_unit(play) * 2.0F - 1.0F, random_unit(play) * 2.0F - 1.0F);
        if (player != nullptr) {
            direction = player->position - enemy->position;
        }
        if (glm::length(direction) < 0.1F) {
            direction = glm::vec2(0.0F, 1.0F);
        } else {
            direction = glm::normalize(direction);
        }
        enemy->velocity = direction * kFlyingGhiniSpeed;
        enemy->action_seconds_remaining = 0.15F + random_unit(play) * 0.20F;
    }

    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
}

void tick_vire(GameState* play, const World* world, Enemy* enemy, const Player* player,
               float dt_seconds) {
    enemy->move_seconds_remaining -= dt_seconds;
    if (enemy->move_seconds_remaining <= 0.0F) {
        choose_cardinal_direction(play, world, enemy);
    }

    glm::vec2 candidate = enemy->position + facing_vector(enemy->facing) * kVireSpeed * dt_seconds;
    candidate.y += std::sin(enemy->action_seconds_remaining * 14.0F) * 0.04F;
    if (world_is_walkable_tile(world, candidate)) {
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
    if (enemy->hidden) {
        enemy->state_seconds_remaining -= dt_seconds;
        if (enemy->state_seconds_remaining > 0.0F) {
            return;
        }

        enemy->hidden = false;
        enemy->action_seconds_remaining = 1.0F + random_unit(play) * 0.5F;
        enemy->move_seconds_remaining = 0.3F;
        return;
    }

    enemy->action_seconds_remaining -= dt_seconds;
    enemy->move_seconds_remaining -= dt_seconds;
    if (enemy->move_seconds_remaining <= 0.0F) {
        if (player != nullptr) {
            choose_player_axis_direction(*enemy, *player, &enemy->facing);
        }
        enemy->move_seconds_remaining = 0.3F;
    }

    const glm::vec2 candidate =
        enemy->position + facing_vector(enemy->facing) * kWizzrobeSpeed * dt_seconds;
    if (world_is_walkable_tile(world, candidate)) {
        enemy->position = candidate;
    }

    if (enemy->action_seconds_remaining <= 0.0F) {
        if (player != nullptr &&
            choose_cardinal_shot_direction(play, *enemy, *player, &enemy->facing)) {
            make_projectile(play, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                            enemy->position + facing_vector(enemy->facing) * 0.7F,
                            facing_vector(enemy->facing) * kFireSpeed, kProjectileLifetimeSeconds,
                            kFireRadius, 1);
        }

        enemy->hidden = true;
        enemy->state_seconds_remaining = kWizzrobeTeleportSeconds;
        const std::array<glm::vec2, 4> offsets = {
            glm::vec2(-4.0F, 0.0F),
            glm::vec2(4.0F, 0.0F),
            glm::vec2(0.0F, -4.0F),
            glm::vec2(0.0F, 4.0F),
        };
        for (int attempt = 0; attempt < 4; ++attempt) {
            const glm::vec2 target =
                enemy->position +
                offsets[static_cast<std::size_t>((attempt + random_int(play, 4)) % 4)];
            if (!world_is_walkable_tile(world, target)) {
                continue;
            }
            enemy->position = target;
            break;
        }
    }
}

void tick_red_wizzrobe(GameState* play, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds) {
    enemy->state_seconds_remaining -= dt_seconds;
    if (enemy->hidden) {
        if (enemy->state_seconds_remaining > 0.0F || player == nullptr) {
            return;
        }

        enemy->hidden = false;
        enemy->action_seconds_remaining = 1.0F;
        choose_player_axis_direction(*enemy, *player, &enemy->facing);
        const std::array<glm::vec2, 8> offsets = {
            glm::vec2(-4.0F, 0.0F), glm::vec2(4.0F, 0.0F),   glm::vec2(0.0F, -4.0F),
            glm::vec2(0.0F, 4.0F),  glm::vec2(-3.0F, -3.0F), glm::vec2(3.0F, -3.0F),
            glm::vec2(-3.0F, 3.0F), glm::vec2(3.0F, 3.0F),
        };
        for (int attempt = 0; attempt < 8; ++attempt) {
            const glm::vec2 target =
                player->position +
                offsets[static_cast<std::size_t>((attempt + random_int(play, 8)) % 8)];
            if (!world_is_walkable_tile(world, target)) {
                continue;
            }
            enemy->position = target;
            break;
        }
        return;
    }

    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining <= 0.45F && enemy->action_seconds_remaining > 0.35F &&
        player != nullptr) {
        choose_cardinal_shot_direction(play, *enemy, *player, &enemy->facing);
        make_projectile(play, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                        enemy->position + facing_vector(enemy->facing) * 0.8F,
                        facing_vector(enemy->facing) * kFireSpeed, kProjectileLifetimeSeconds,
                        kFireRadius, 1);
    }

    if (enemy->action_seconds_remaining > 0.0F) {
        return;
    }

    enemy->hidden = true;
    enemy->state_seconds_remaining = 1.0F + random_unit(play) * 0.8F;
}

void tick_trap(const World* world, Enemy* enemy, const Player* player, float dt_seconds) {
    if (glm::length(enemy->velocity) > 0.0F) {
        const glm::vec2 candidate = enemy->position + enemy->velocity * dt_seconds;
        if (!world_is_walkable_tile(world, candidate)) {
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
        if (!world_is_walkable_tile(world, candidate)) {
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
        bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
        if (glm::length(enemy->velocity) > 0.001F) {
            enemy->velocity = glm::normalize(enemy->velocity) * speed;
        }
    }
}

void tick_leever(GameState* play, const World* world, Enemy* enemy, const Player* player,
                 float dt_seconds) {
    enemy->state_seconds_remaining -= dt_seconds;
    if (enemy->hidden) {
        if (enemy->state_seconds_remaining <= 0.0F) {
            enemy->hidden = false;
            enemy->state_seconds_remaining = 1.4F + random_unit(play) * 0.8F;
        }
        return;
    }

    if (enemy->state_seconds_remaining <= 0.0F) {
        enemy->hidden = true;
        enemy->state_seconds_remaining = 0.7F + random_unit(play) * 0.5F;
        return;
    }

    glm::vec2 toward_player(random_unit(play) * 2.0F - 1.0F, random_unit(play) * 2.0F - 1.0F);
    if (player != nullptr) {
        toward_player = player->position - enemy->position;
    }
    if (glm::length(toward_player) > 0.001F) {
        toward_player = glm::normalize(toward_player);
    } else {
        toward_player = glm::vec2(0.0F, 1.0F);
    }

    const glm::vec2 candidate = enemy->position + toward_player * kLeeverSpeed * dt_seconds;
    if (world_is_walkable_tile(world, candidate)) {
        enemy->position = candidate;
    }
}

} // namespace z1m

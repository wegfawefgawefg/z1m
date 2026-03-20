#include "content/sandbox_content.hpp"
#include "game/enemies/ticks.hpp"
#include "game/geometry.hpp"
#include "game/items.hpp"
#include "game/rng.hpp"
#include "game/tuning.hpp"

#include <glm/common.hpp>

namespace z1m {

namespace {

Facing perpendicular_facing_toward_player(const Enemy& enemy, const Player& player) {
    const bool facing_vertical = enemy.facing == Facing::Up || enemy.facing == Facing::Down;
    return axis_facing_toward(enemy.position, player.position, !facing_vertical);
}

Facing goriya_primary_facing(const Enemy& enemy, const Player& player) {
    const float vertical_distance = std::abs(player.position.y - enemy.position.y);
    const float horizontal_distance = std::abs(player.position.x - enemy.position.x);
    const bool use_horizontal = horizontal_distance > vertical_distance;
    return axis_facing_toward(enemy.position, player.position, !use_horizontal);
}

Facing goriya_secondary_facing(const Enemy& enemy, const Player& player) {
    const float vertical_distance = std::abs(player.position.y - enemy.position.y);
    const float horizontal_distance = std::abs(player.position.x - enemy.position.x);
    const bool use_horizontal = horizontal_distance > vertical_distance;
    return axis_facing_toward(enemy.position, player.position, use_horizontal);
}

bool try_set_walkable_facing(const World* world, const Enemy& enemy, Facing facing) {
    const glm::vec2 probe = enemy.position + facing_vector(facing) * 0.8F;
    return enemy_can_move_to(&enemy, world, probe);
}

Facing opposite_facing(Facing facing) {
    switch (facing) {
    case Facing::Up:
        return Facing::Down;
    case Facing::Down:
        return Facing::Up;
    case Facing::Left:
        return Facing::Right;
    case Facing::Right:
        return Facing::Left;
    }

    return Facing::Down;
}

bool choose_wanderer_axis_turn_toward_player(const World* world, const Enemy& enemy,
                                             const Player& player, Facing* facing_out) {
    const glm::vec2 delta = player.position - enemy.position;
    if (std::abs(delta.x) < kWandererTargetThreshold) {
        const Facing facing = axis_facing_toward(enemy.position, player.position, true);
        if (try_set_walkable_facing(world, enemy, facing)) {
            *facing_out = facing;
            return true;
        }
    }

    if (std::abs(delta.y) < kWandererTargetThreshold) {
        const Facing facing = axis_facing_toward(enemy.position, player.position, false);
        if (try_set_walkable_facing(world, enemy, facing)) {
            *facing_out = facing;
            return true;
        }
    }

    return false;
}

bool choose_wanderer_perpendicular_turn(const World* world, const Enemy& enemy,
                                        const Player& player, Facing* facing_out) {
    const Facing perpendicular = perpendicular_facing_toward_player(enemy, player);
    if (try_set_walkable_facing(world, enemy, perpendicular)) {
        *facing_out = perpendicular;
        return true;
    }

    const Facing opposite = opposite_facing(perpendicular);
    if (try_set_walkable_facing(world, enemy, opposite)) {
        *facing_out = opposite;
        return true;
    }

    return false;
}

} // namespace

bool enemy_can_move_to(const Enemy* enemy, const World* world, const glm::vec2& position) {
    if (!world_is_walkable_tile(world, position)) {
        return false;
    }

    if (enemy == nullptr || enemy->area_kind != AreaKind::EnemyZoo || enemy->respawn_group < 0) {
        return true;
    }

    glm::vec2 min_position(0.0F);
    glm::vec2 max_position(0.0F);
    if (!get_enemy_zoo_pen_bounds(enemy->respawn_group, &min_position, &max_position)) {
        return true;
    }

    return position.x >= min_position.x && position.x <= max_position.x &&
           position.y >= min_position.y && position.y <= max_position.y;
}

void bounce_velocity(const Enemy* enemy, const World* world, glm::vec2* position,
                     glm::vec2* velocity, float dt) {
    const glm::vec2 candidate_x = *position + glm::vec2(velocity->x * dt, 0.0F);
    if (enemy_can_move_to(enemy, world, candidate_x)) {
        position->x = candidate_x.x;
    } else {
        velocity->x *= -1.0F;
    }

    const glm::vec2 candidate_y = *position + glm::vec2(0.0F, velocity->y * dt);
    if (enemy_can_move_to(enemy, world, candidate_y)) {
        position->y = candidate_y.y;
    } else {
        velocity->y *= -1.0F;
    }
}

float random_turn_timer_seconds(GameState* play) {
    return static_cast<float>(random_int(play, 256)) / 60.0F;
}

int random_byte(GameState* play) {
    return random_int(play, 256);
}

float frames_to_seconds(int frames) {
    return static_cast<float>(frames) / 60.0F;
}

bool try_fire_monster_projectile(GameState* play, Enemy* enemy, ProjectileKind shot_kind) {
    if (shot_kind == ProjectileKind::SwordBeam) {
        create_sword_beam(play, *enemy, facing_vector(enemy->facing));
        return true;
    }

    if (shot_kind == ProjectileKind::Arrow) {
        make_projectile(play, enemy->area_kind, enemy->cave_id, ProjectileKind::Arrow, false,
                        enemy->position + facing_vector(enemy->facing) * 0.9F,
                        facing_vector(enemy->facing) * kArrowSpeed, kProjectileLifetimeSeconds,
                        kArrowRadius, 1);
        return true;
    }

    if (shot_kind == ProjectileKind::Boomerang) {
        make_projectile(play, enemy->area_kind, enemy->cave_id, ProjectileKind::Boomerang, false,
                        enemy->position + facing_vector(enemy->facing) * 0.8F,
                        facing_vector(enemy->facing) * kBoomerangSpeed, 0.85F, kBoomerangRadius, 1);
        return true;
    }

    const glm::vec2 velocity = facing_vector(enemy->facing) * kRockSpeed;
    throw_enemy_rock(play, *enemy, velocity);
    return true;
}

bool tick_monster_shot_windup(GameState* play, Enemy* enemy, ProjectileKind shot_kind,
                              float dt_seconds) {
    if (enemy->action_seconds_remaining > 0.0F) {
        const float previous = enemy->action_seconds_remaining;
        enemy->action_seconds_remaining = glm::max(0.0F, previous - dt_seconds);
        if (previous > frames_to_seconds(16) &&
            enemy->action_seconds_remaining <= frames_to_seconds(16)) {
            try_fire_monster_projectile(play, enemy, shot_kind);
            enemy->action_seconds_remaining = 0.0F;
            enemy->special_counter = 0;
        }
        return true;
    }

    return false;
}

void try_begin_monster_shot_windup(GameState* play, Enemy* enemy, bool blue_walker) {
    if (enemy->special_counter == 0 || enemy->action_seconds_remaining > 0.0F) {
        return;
    }

    if (!blue_walker && random_byte(play) < 0xF8) {
        return;
    }

    enemy->action_seconds_remaining = frames_to_seconds(48);
}

void tick_rom_wanderer_shooter(GameState* play, const World* world, Enemy* enemy,
                               const Player* player, float dt_seconds, float speed, int turn_rate,
                               ProjectileKind shot_kind, bool allow_shoot, bool blue_walker) {
    enemy->state_seconds_remaining = glm::max(0.0F, enemy->state_seconds_remaining - dt_seconds);

    if (tick_monster_shot_windup(play, enemy, shot_kind, dt_seconds)) {
        return;
    }

    enemy->move_seconds_remaining = glm::max(0.0F, enemy->move_seconds_remaining - dt_seconds);

    // Cooldown prevents re-entering tile-center logic until the enemy has moved past
    // the tolerance zone. Matches NES behavior where grid offset != 0 skips decisions.
    const float tile_center_cooldown =
        speed > 0.0F ? (kGridCenterTolerance / speed + dt_seconds) : 0.0F;

    if (near_tile_center(enemy->position) && enemy->state_seconds_remaining <= 0.0F) {
        snap_to_tile_center(&enemy->position);
        enemy->special_counter = 0;
        enemy->state_seconds_remaining = tile_center_cooldown;

        if (player != nullptr) {
            const bool can_turn_toward_player = random_byte(play) <= turn_rate;
            if (can_turn_toward_player) {
                Facing new_facing = enemy->facing;
                if (choose_wanderer_axis_turn_toward_player(world, *enemy, *player, &new_facing)) {
                    enemy->facing = new_facing;
                    enemy->move_seconds_remaining = random_turn_timer_seconds(play);
                    enemy->special_counter = allow_shoot ? 1 : 0;
                } else if (enemy->move_seconds_remaining <= 0.0F) {
                    if (!choose_wanderer_perpendicular_turn(world, *enemy, *player, &new_facing)) {
                        choose_cardinal_direction(play, world, enemy);
                    } else {
                        enemy->facing = new_facing;
                    }
                    enemy->move_seconds_remaining = random_turn_timer_seconds(play);
                }
            } else if (enemy->move_seconds_remaining <= 0.0F) {
                Facing new_facing = enemy->facing;
                if (choose_wanderer_perpendicular_turn(world, *enemy, *player, &new_facing)) {
                    enemy->facing = new_facing;
                } else {
                    choose_cardinal_direction(play, world, enemy);
                }
                enemy->move_seconds_remaining = random_turn_timer_seconds(play);
            }
        } else if (enemy->move_seconds_remaining <= 0.0F) {
            choose_cardinal_direction(play, world, enemy);
        }
    }

    const glm::vec2 candidate = enemy->position + facing_vector(enemy->facing) * speed * dt_seconds;
    if (enemy_can_move_to(enemy, world, candidate)) {
        enemy->position = candidate;
    } else {
        snap_to_tile_center(&enemy->position);
        enemy->move_seconds_remaining = 0.0F;
        enemy->state_seconds_remaining = tile_center_cooldown;
        if (player != nullptr) {
            Facing new_facing = enemy->facing;
            if (choose_wanderer_perpendicular_turn(world, *enemy, *player, &new_facing)) {
                enemy->facing = new_facing;
            } else {
                choose_cardinal_direction(play, world, enemy);
            }
        } else {
            choose_cardinal_direction(play, world, enemy);
        }
    }

    try_begin_monster_shot_windup(play, enemy, blue_walker);
}

void tick_rom_goriya_movement(GameState* play, const World* world, Enemy* enemy,
                              const Player* player, float dt_seconds, float speed) {
    enemy->state_seconds_remaining = glm::max(0.0F, enemy->state_seconds_remaining - dt_seconds);

    const float tile_center_cooldown =
        speed > 0.0F ? (kGridCenterTolerance / speed + dt_seconds) : 0.0F;

    if (near_tile_center(enemy->position) && enemy->state_seconds_remaining <= 0.0F) {
        snap_to_tile_center(&enemy->position);
        enemy->special_counter = 0;
        enemy->state_seconds_remaining = tile_center_cooldown;

        if (player != nullptr) {
            const float vertical_distance = std::abs(player->position.y - enemy->position.y);
            const float horizontal_distance = std::abs(player->position.x - enemy->position.x);
            const bool use_horizontal = horizontal_distance > vertical_distance;
            const float chosen_distance = use_horizontal ? horizontal_distance : vertical_distance;
            if (chosen_distance < kGoriyaTargetThreshold) {
                const Facing primary = goriya_primary_facing(*enemy, *player);
                const Facing secondary = goriya_secondary_facing(*enemy, *player);
                if (try_set_walkable_facing(world, *enemy, primary)) {
                    enemy->facing = primary;
                } else if (try_set_walkable_facing(world, *enemy, secondary)) {
                    enemy->facing = secondary;
                }
                enemy->special_counter = 1;
            }
        } else if (enemy->move_seconds_remaining <= 0.0F) {
            choose_cardinal_direction(play, world, enemy);
        }
    }

    const glm::vec2 candidate = enemy->position + facing_vector(enemy->facing) * speed * dt_seconds;
    if (enemy_can_move_to(enemy, world, candidate)) {
        enemy->position = candidate;
    } else {
        enemy->state_seconds_remaining = tile_center_cooldown;
        if (player != nullptr) {
            const Facing primary = goriya_primary_facing(*enemy, *player);
            const Facing secondary = goriya_secondary_facing(*enemy, *player);
            if (try_set_walkable_facing(world, *enemy, secondary)) {
                enemy->facing = secondary;
            } else if (try_set_walkable_facing(world, *enemy, primary)) {
                enemy->facing = primary;
            }
        } else {
            choose_cardinal_direction(play, world, enemy);
        }
    }
}

void tick_rom_goriya_like(GameState* play, const World* world, Enemy* enemy, const Player* player,
                          float dt_seconds, float speed, ProjectileKind shot_kind) {
    if (tick_monster_shot_windup(play, enemy, shot_kind, dt_seconds)) {
        return;
    }

    tick_rom_goriya_movement(play, world, enemy, player, dt_seconds, speed);
    if (enemy->special_counter == 0) {
        return;
    }

    if (enemy->subtype != 0) {
        const int roll = random_byte(play);
        if (roll != 0x23 && roll != 0x77) {
            return;
        }
    }

    enemy->action_seconds_remaining = frames_to_seconds(48);
}

void tick_rom_lynel(GameState* play, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds) {
    if (tick_monster_shot_windup(play, enemy, ProjectileKind::SwordBeam, dt_seconds)) {
        return;
    }

    tick_rom_goriya_movement(play, world, enemy, player, dt_seconds, kLynelSpeed);
    try_begin_monster_shot_windup(play, enemy, enemy->subtype == 0);
}

void tick_rom_darknut(GameState* play, const World* world, Enemy* enemy, const Player* player,
                      float dt_seconds) {
    const float speed = enemy->subtype == 0 ? qspeed_to_speed(0x20) : qspeed_to_speed(0x28);
    tick_rom_wanderer_shooter(play, world, enemy, player, dt_seconds, speed, 0x80,
                              ProjectileKind::Rock, false, false);
}

void tick_rom_common_wanderer(GameState* play, const World* world, Enemy* enemy,
                              const Player* player, float dt_seconds, float speed, int turn_rate) {
    tick_rom_wanderer_shooter(play, world, enemy, player, dt_seconds, speed, turn_rate,
                              ProjectileKind::Rock, false, false);
}

void tick_octorok_like(GameState* play, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds, float speed, float shoot_period_min,
                       float shoot_period_random, ProjectileKind shot_kind) {
    static_cast<void>(speed);
    static_cast<void>(shoot_period_min);
    static_cast<void>(shoot_period_random);

    int turn_rate = 0x70;
    bool blue_walker = false;
    float actual_speed = qspeed_to_speed(0x20);

    if (enemy->kind == EnemyKind::Moblin) {
        turn_rate = 0xA0;
        blue_walker = true;
        actual_speed = qspeed_to_speed(0x20);
    } else if (enemy->subtype > 0) {
        turn_rate = 0xA0;
        blue_walker = true;
        actual_speed = qspeed_to_speed(0x30);
    }

    tick_rom_wanderer_shooter(play, world, enemy, player, dt_seconds, actual_speed, turn_rate,
                              shot_kind, true, blue_walker);
}

void tick_basic_walker(GameState* play, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds, float speed, bool bias_toward_player) {
    enemy->move_seconds_remaining -= dt_seconds;
    enemy->action_seconds_remaining -= dt_seconds;

    if (bias_toward_player && player != nullptr && enemy->action_seconds_remaining <= 0.0F) {
        const glm::vec2 delta = player->position - enemy->position;
        if (std::abs(delta.x) > std::abs(delta.y)) {
            enemy->facing = delta.x < 0.0F ? Facing::Left : Facing::Right;
        } else {
            enemy->facing = delta.y < 0.0F ? Facing::Up : Facing::Down;
        }
        enemy->action_seconds_remaining = 0.35F + random_unit(play) * 0.40F;
        enemy->move_seconds_remaining = 0.30F + random_unit(play) * 0.50F;
    } else if (enemy->move_seconds_remaining <= 0.0F) {
        choose_cardinal_direction(play, world, enemy);
    }

    const glm::vec2 candidate = enemy->position + facing_vector(enemy->facing) * speed * dt_seconds;
    if (enemy_can_move_to(enemy, world, candidate)) {
        enemy->position = candidate;
    } else {
        enemy->move_seconds_remaining = 0.0F;
    }
}

} // namespace z1m

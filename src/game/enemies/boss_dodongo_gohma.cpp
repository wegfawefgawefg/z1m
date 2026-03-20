#include "game/area_state.hpp"
#include "game/combat.hpp"
#include "game/enemies/state.hpp"
#include "game/enemies/ticks.hpp"
#include "game/geometry.hpp"
#include "game/items.hpp"
#include "game/rng.hpp"
#include "game/tuning.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace z1m {

namespace {

constexpr int kDodongoPhaseMove = 0;
constexpr int kDodongoPhaseBloated0 = 1;
constexpr int kDodongoPhaseBloated1 = 2;
constexpr int kDodongoPhaseBloated2 = 3;
constexpr int kDodongoPhaseRecover = 4;

constexpr int kGohmaEyeClosedA = 0;
constexpr int kGohmaEyeClosedB = 1;
constexpr int kGohmaEyeHalfOpen = 2;
constexpr int kGohmaEyeOpen = 3;

Facing reverse_facing(Facing facing) {
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

int gohma_eye_state(const Enemy& enemy) {
    return enemy.special_counter & 0x03;
}

void set_gohma_eye_state(Enemy* enemy, int eye_state) {
    enemy->special_counter = (enemy->special_counter & ~0x03) | (eye_state & 0x03);
}

bool gohma_needs_fresh_direction(const Enemy& enemy) {
    return (enemy.special_counter & 0x04) == 0;
}

void set_gohma_needs_fresh_direction(Enemy* enemy, bool needs_fresh_direction) {
    if (needs_fresh_direction) {
        enemy->special_counter &= ~0x04;
    } else {
        enemy->special_counter |= 0x04;
    }
}

Facing random_gohma_facing(GameState* play) {
    const int roll = random_byte(play);
    if (roll >= 0xB0) {
        return Facing::Right;
    }
    if (roll >= 0x60) {
        return Facing::Left;
    }
    return Facing::Down;
}

void begin_gohma_sprint(GameState* play, Enemy* enemy) {
    if (gohma_needs_fresh_direction(*enemy)) {
        enemy->facing = random_gohma_facing(play);
        set_gohma_needs_fresh_direction(enemy, false);
    } else {
        enemy->facing = reverse_facing(enemy->facing);
        set_gohma_needs_fresh_direction(enemy, true);
    }

    enemy->velocity = facing_vector(enemy->facing) * kGohmaSpeed;
    enemy->move_seconds_remaining = 32.0F / 16.0F / kGohmaSpeed;
}

void tick_gohma_eye_and_shoot(GameState* play, Enemy* enemy, const Player* player,
                              float dt_seconds) {
    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining <= 0.0F) {
        enemy->state_seconds_remaining = frames_to_seconds(0x80);
        enemy->action_seconds_remaining = frames_to_seconds(0xC0 | random_byte(play));
    }

    const float previous_eye_timer = enemy->state_seconds_remaining;
    if (enemy->state_seconds_remaining > 0.0F) {
        enemy->state_seconds_remaining =
            glm::max(0.0F, enemy->state_seconds_remaining - dt_seconds);
    }

    if (enemy->state_seconds_remaining <= 0.0F) {
        const int next_closed =
            gohma_eye_state(*enemy) == kGohmaEyeClosedA ? kGohmaEyeClosedB : kGohmaEyeClosedA;
        set_gohma_eye_state(enemy, next_closed);
    } else if (previous_eye_timer >= frames_to_seconds(0x70) ||
               enemy->state_seconds_remaining < frames_to_seconds(0x10)) {
        set_gohma_eye_state(enemy, kGohmaEyeHalfOpen);
    } else {
        set_gohma_eye_state(enemy, kGohmaEyeOpen);
    }

    enemy->respawn_seconds_remaining -= dt_seconds;
    if (enemy->respawn_seconds_remaining > 0.0F) {
        return;
    }

    enemy->respawn_seconds_remaining = frames_to_seconds(0x41);
    glm::vec2 direction = facing_vector(enemy->facing);
    if (player != nullptr) {
        direction = player->position - enemy->position;
        if (glm::length(direction) >= 0.001F) {
            direction = glm::normalize(direction);
        } else {
            direction = facing_vector(enemy->facing);
        }
    }

    make_projectile(play, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                    enemy->position + direction * 0.8F, direction * kFireSpeed,
                    kProjectileLifetimeSeconds, kFireRadius, 1);
}

void spawn_digdogger_children(GameState* play, const Enemy& enemy) {
    for (int index = 0; index < 3; ++index) {
        Enemy child;
        child.active = true;
        child.kind = EnemyKind::Digdogger;
        child.area_kind = enemy.area_kind;
        child.cave_id = enemy.cave_id;
        child.position =
            enemy.position + orbit_offset(static_cast<float>(index) * 2.09439510239F, 1.2F);
        child.spawn_position = child.position;
        child.origin = child.position;
        child.max_health = 2;
        child.health = 2;
        child.subtype = 1;
        child.respawn_group = -1;
        child.zoo_respawn = false;
        reset_enemy_state(play, &child);
        play->enemies.push_back(child);
    }
}

} // namespace

void tick_dodongo(GameState* play, const World* world, Enemy* enemy, const Player* player,
                  float dt_seconds) {
    enemy->velocity = glm::vec2(0.0F);

    switch (enemy->subtype) {
    case kDodongoPhaseMove:
        enemy->invulnerable = false;
        tick_rom_common_wanderer(play, world, enemy, player, dt_seconds, kDodongoSpeed, 0x20);
        break;
    case kDodongoPhaseBloated0:
    case kDodongoPhaseBloated1:
    case kDodongoPhaseBloated2:
    case kDodongoPhaseRecover:
        enemy->invulnerable = true;
        enemy->state_seconds_remaining =
            glm::max(0.0F, enemy->state_seconds_remaining - dt_seconds);
        if (enemy->state_seconds_remaining > 0.0F) {
            break;
        }

        if (enemy->subtype == kDodongoPhaseBloated0) {
            enemy->subtype = kDodongoPhaseBloated1;
            enemy->state_seconds_remaining = frames_to_seconds(0x40);
            break;
        }

        if (enemy->subtype == kDodongoPhaseBloated1) {
            if (enemy->special_counter >= 2) {
                enemy->subtype = kDodongoPhaseBloated2;
                enemy->state_seconds_remaining = frames_to_seconds(0x40);
            } else {
                enemy->subtype = kDodongoPhaseRecover;
                enemy->state_seconds_remaining = frames_to_seconds(0x20);
            }
            break;
        }

        if (enemy->subtype == kDodongoPhaseBloated2) {
            damage_enemy(play, enemy, enemy->health);
            break;
        }

        enemy->subtype = kDodongoPhaseMove;
        break;
    default:
        enemy->subtype = kDodongoPhaseMove;
        break;
    }
}

void tick_gohma(GameState* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds) {
    if (enemy->move_seconds_remaining <= 0.0F) {
        begin_gohma_sprint(play, enemy);
    }

    const glm::vec2 candidate = enemy->position + enemy->velocity * dt_seconds;
    if (enemy_can_move_to(enemy, world, candidate)) {
        enemy->position = candidate;
        enemy->move_seconds_remaining = glm::max(0.0F, enemy->move_seconds_remaining - dt_seconds);
    } else {
        enemy->facing = reverse_facing(enemy->facing);
        enemy->velocity = facing_vector(enemy->facing) * kGohmaSpeed;
        enemy->move_seconds_remaining = 32.0F / 16.0F / kGohmaSpeed;
        set_gohma_needs_fresh_direction(enemy, true);
    }

    tick_gohma_eye_and_shoot(play, enemy, player, dt_seconds);
}

void tick_digdogger(GameState* play, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds) {
    enemy->invulnerable = enemy->subtype == 0;

    if (enemy->subtype == 0 && enemy->special_counter == 1) {
        enemy->velocity = glm::vec2(0.0F);
        enemy->state_seconds_remaining =
            glm::max(0.0F, enemy->state_seconds_remaining - dt_seconds);
        if (enemy->state_seconds_remaining <= 0.0F) {
            enemy->active = false;
            spawn_digdogger_children(play, *enemy);
            set_message(play, "digdogger split", 1.0F);
        }
        return;
    }

    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining <= 0.0F) {
        const glm::vec2 current_direction = glm::length(enemy->velocity) > 0.001F
                                                ? glm::normalize(enemy->velocity)
                                                : glm::vec2(1.0F, 0.0F);
        glm::vec2 next_direction = current_direction;
        if (player != nullptr && random_byte(play) >= 0x80) {
            next_direction = rotate_dir8_once_toward(
                current_direction, eight_way_direction_toward(enemy->position, player->position));
        } else {
            next_direction = rotate_dir8_random(play, current_direction);
        }
        enemy->velocity = next_direction;
        enemy->action_seconds_remaining = frames_to_seconds(0x10);
    }

    enemy->move_seconds_remaining -= dt_seconds;
    if (enemy->move_seconds_remaining <= 0.0F) {
        enemy->move_seconds_remaining = frames_to_seconds(0x10);
        enemy->special_counter = enemy->special_counter == 2 ? 0 : 2;
    }

    float speed = enemy->subtype == 0 ? kDigdoggerSpeed : kDigdoggerSpeed * 1.35F;
    if (enemy->special_counter == 2) {
        speed *= 1.25F;
    } else if (enemy->subtype == 0) {
        speed *= 0.80F;
    }

    const glm::vec2 candidate =
        enemy->position + glm::normalize(enemy->velocity) * speed * dt_seconds;
    if (enemy_can_move_to(enemy, world, candidate)) {
        enemy->position = candidate;
        return;
    }

    enemy->velocity = rotate_dir8_random(play, -glm::normalize(enemy->velocity));
    const glm::vec2 bounce_candidate =
        enemy->position + glm::normalize(enemy->velocity) * speed * dt_seconds;
    if (enemy_can_move_to(enemy, world, bounce_candidate)) {
        enemy->position = bounce_candidate;
    }
}

} // namespace z1m

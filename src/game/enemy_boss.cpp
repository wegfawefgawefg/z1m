#include "game/enemy_ticks.hpp"
#include "game/geometry.hpp"
#include "game/items.hpp"
#include "game/rng.hpp"
#include "game/tuning.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace z1m {

void tick_moldorm(GameState* play, const World* world, Enemy* enemy, const Player* player,
                  float dt_seconds) {
    if (enemy->subtype > 0) {
        Enemy* leader = nullptr;
        for (Enemy& other : play->enemies) {
            if (!other.active || other.kind != EnemyKind::Moldorm ||
                other.area_kind != enemy->area_kind || other.cave_id != enemy->cave_id ||
                other.respawn_group != enemy->respawn_group ||
                other.subtype != enemy->subtype - 1) {
                continue;
            }
            leader = &other;
            break;
        }

        if (leader == nullptr) {
            return;
        }

        const glm::vec2 toward_leader = leader->position - enemy->position;
        const float distance = glm::length(toward_leader);
        if (distance > 0.72F) {
            const glm::vec2 direction = toward_leader / distance;
            enemy->position += direction * glm::min(distance - 0.72F, kMoldormSpeed * dt_seconds);
            enemy->facing = direction.x < 0.0F ? Facing::Left : Facing::Right;
        }
        return;
    }

    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining <= 0.0F) {
        glm::vec2 direction(random_unit(play) * 2.0F - 1.0F, random_unit(play) * 2.0F - 1.0F);
        if (player != nullptr && random_unit(play) < 0.5F) {
            direction = player->position - enemy->position;
        }
        if (glm::length(direction) < 0.1F) {
            direction = glm::vec2(1.0F, 0.0F);
        } else {
            direction = glm::normalize(direction);
        }
        enemy->velocity = direction * kMoldormSpeed;
        enemy->action_seconds_remaining = 0.25F + random_unit(play) * 0.35F;
    }

    bounce_velocity(enemy, world, &enemy->position, &enemy->velocity, dt_seconds);
}

void tick_manhandla(GameState* play, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds) {
    enemy->respawn_seconds_remaining += dt_seconds;
    enemy->action_seconds_remaining = glm::max(0.0F, enemy->action_seconds_remaining - dt_seconds);
    enemy->state_seconds_remaining = glm::max(0.0F, enemy->state_seconds_remaining - dt_seconds);

    const int live_petals = glm::max(enemy->special_counter, 1);
    const float speed = kManhandlaSpeed + static_cast<float>(4 - live_petals) * 0.9F;
    if (enemy->action_seconds_remaining <= 0.0F) {
        glm::vec2 direction = enemy->velocity;
        if (glm::length(direction) < 0.001F) {
            direction = glm::vec2(1.0F, 0.0F);
        }

        if (player != nullptr && random_byte(play) >= 0x80) {
            direction = eight_way_direction_toward(enemy->position, player->position);
        } else {
            direction = rotate_dir8_random(play, direction);
        }

        enemy->velocity = glm::normalize(direction) * speed;
        enemy->action_seconds_remaining = frames_to_seconds(0x10);
    } else if (glm::length(enemy->velocity) > 0.001F) {
        enemy->velocity = glm::normalize(enemy->velocity) * speed;
    }

    bounce_velocity(enemy, world, &enemy->position, &enemy->velocity, dt_seconds);

    if (enemy->state_seconds_remaining <= 0.0F) {
        enemy->state_seconds_remaining = 0.55F + random_unit(play) * 0.25F;
        const glm::vec2 shot_origin =
            manhandla_petal_position(*enemy, random_int(play, live_petals));
        if (player != nullptr) {
            const glm::vec2 toward_player = player->position - shot_origin;
            if (glm::length(toward_player) > 0.1F) {
                make_projectile(play, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                                shot_origin, glm::normalize(toward_player) * kFireSpeed,
                                kProjectileLifetimeSeconds, kFireRadius, 1);
            }
        }
    }
}

void tick_gleeok(GameState* play, const World* world, Enemy* enemy, const Player* player,
                 float dt_seconds) {
    static_cast<void>(world);
    enemy->velocity = glm::vec2(0.0F);
    enemy->respawn_seconds_remaining += dt_seconds;
    enemy->action_seconds_remaining = glm::max(0.0F, enemy->action_seconds_remaining - dt_seconds);
    if (enemy->action_seconds_remaining > 0.0F) {
        return;
    }

    const int head_count = glm::max(enemy->special_counter, 1);
    const int head_index = random_int(play, head_count);
    const glm::vec2 head_position = gleeok_head_position(*enemy, head_index);
    glm::vec2 direction = glm::vec2(0.0F, 1.0F);
    if (player != nullptr) {
        direction = player->position - head_position;
    }
    if (glm::length(direction) >= 0.1F) {
        direction = glm::normalize(direction);
    }

    make_projectile(play, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                    head_position, direction * kFireSpeed, kProjectileLifetimeSeconds, kFireRadius,
                    1);
    enemy->action_seconds_remaining = 0.7F + random_unit(play) * 0.4F;
}

void tick_patra(GameState* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds) {
    enemy->respawn_seconds_remaining += dt_seconds;
    tick_rom_flyer(play, world, enemy, player, dt_seconds, kPatraSpeed, 0x40, 0x00, false);
    enemy->state_seconds_remaining = glm::max(0.0F, enemy->state_seconds_remaining - dt_seconds);
    if (enemy->state_seconds_remaining <= 0.0F) {
        enemy->subtype = enemy->subtype == 0 ? 1 : 0;
        enemy->state_seconds_remaining = enemy->subtype == 0 ? 4.0F : 2.2F;
    }

    enemy->action_seconds_remaining = glm::max(0.0F, enemy->action_seconds_remaining - dt_seconds);
    if (enemy->action_seconds_remaining <= 0.0F && player != nullptr) {
        glm::vec2 direction = player->position - enemy->position;
        if (glm::length(direction) > 0.1F) {
            direction = glm::normalize(direction);
            make_projectile(play, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                            enemy->position + direction * 0.8F, direction * kFireSpeed,
                            kProjectileLifetimeSeconds, kFireRadius, 1);
        }
        enemy->action_seconds_remaining = 0.9F;
    }
}

void tick_ganon(GameState* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds) {
    if (enemy->special_counter > 0) {
        enemy->hidden = false;
        enemy->state_seconds_remaining = glm::max(0.0F, enemy->state_seconds_remaining - dt_seconds);
        if (enemy->state_seconds_remaining <= 0.0F) {
            enemy->special_counter = 0;
            enemy->hidden = true;
            enemy->position.y = static_cast<float>(world_height(world)) - 2.5F;
            enemy->position.x =
                (random_byte(play) & 0x01) == 0 ? 2.5F : static_cast<float>(world_width(world)) - 2.5F;
            enemy->facing = enemy->position.x < static_cast<float>(world_width(world)) * 0.5F
                                ? Facing::Right
                                : Facing::Left;
            enemy->action_seconds_remaining = 0.45F;
        }
        return;
    }

    enemy->hidden = true;
    enemy->respawn_seconds_remaining = glm::max(0.0F, enemy->respawn_seconds_remaining - dt_seconds);
    if (enemy->respawn_seconds_remaining <= 0.0F) {
        enemy->respawn_seconds_remaining = frames_to_seconds(0x40);
        glm::vec2 direction = facing_vector(enemy->facing);
        if (player != nullptr) {
            direction = player->position - enemy->position;
            if (glm::length(direction) >= 0.1F) {
                direction = glm::normalize(direction);
            }
        }

        make_projectile(play, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                        enemy->position, direction * kFireSpeed, kProjectileLifetimeSeconds,
                        kFireRadius, 1);
    }

    enemy->action_seconds_remaining = glm::max(0.0F, enemy->action_seconds_remaining - dt_seconds);
    if (enemy->action_seconds_remaining <= 0.0F) {
        enemy->position.y = static_cast<float>(world_height(world)) - 2.5F;
        enemy->position.x =
            (random_byte(play) & 0x01) == 0 ? 2.5F : static_cast<float>(world_width(world)) - 2.5F;
        enemy->facing = player == nullptr ? Facing::Left
                                          : axis_facing_toward(enemy->position, player->position, false);
        enemy->action_seconds_remaining = 0.45F;
    }

    const glm::vec2 candidate = enemy->position + facing_vector(enemy->facing) * kGanonSpeed * dt_seconds;
    if (enemy_can_move_to(enemy, world, candidate)) {
        enemy->position = candidate;
    } else {
        enemy->action_seconds_remaining = 0.0F;
    }
}

void tick_zora(GameState* play, const World* world, Enemy* enemy, const Player* player,
               float dt_seconds) {
    enemy->state_seconds_remaining -= dt_seconds;
    enemy->action_seconds_remaining -= dt_seconds;

    if (enemy->hidden) {
        if (enemy->state_seconds_remaining > 0.0F) {
            return;
        }

        enemy->hidden = false;
        enemy->state_seconds_remaining = 1.6F + random_unit(play) * 0.7F;
        enemy->action_seconds_remaining = 0.45F;
        return;
    }

    if (enemy->action_seconds_remaining <= 0.0F && player != nullptr) {
        glm::vec2 toward_player = player->position - enemy->position;
        if (glm::length(toward_player) > 0.001F) {
            toward_player = glm::normalize(toward_player);
            make_projectile(play, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                            enemy->position + toward_player * 0.8F, toward_player * kFireSpeed,
                            kProjectileLifetimeSeconds, kFireRadius, 1);
        }
        enemy->action_seconds_remaining = 99.0F;
    }

    if (enemy->state_seconds_remaining > 0.0F) {
        return;
    }

    enemy->hidden = true;
    enemy->state_seconds_remaining = 1.2F + random_unit(play) * 0.8F;

    const glm::ivec2 base(static_cast<int>(enemy->position.x), static_cast<int>(enemy->position.y));
    for (int radius = 0; radius <= 6; ++radius) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                const int tile_x = base.x + dx;
                const int tile_y = base.y + dy;
                if (world_tile_at(world, tile_x, tile_y) != TileKind::Water) {
                    continue;
                }

                enemy->position =
                    glm::vec2(static_cast<float>(tile_x) + 0.5F, static_cast<float>(tile_y) + 0.5F);
                return;
            }
        }
    }
}

void tick_peahat(GameState* play, const World* world, Enemy* enemy, float dt_seconds) {
    tick_rom_flyer(play, world, enemy, nullptr, dt_seconds, kPeahatSpeed, 0xB0, 0x20, true);
}

void tick_aquamentus(GameState* play, const World* world, Enemy* enemy, const Player* player,
                     float dt_seconds) {
    bounce_velocity(enemy, world, &enemy->position, &enemy->velocity, dt_seconds);
    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining > 0.0F) {
        return;
    }

    glm::vec2 toward_player = glm::vec2(0.0F, 1.0F);
    if (player != nullptr) {
        toward_player = player->position - enemy->position;
    }
    if (glm::length(toward_player) < 0.001F) {
        toward_player = glm::vec2(0.0F, 1.0F);
    }

    throw_spread_rocks(play, *enemy, toward_player);
    enemy->action_seconds_remaining = 1.1F + random_unit(play) * 0.4F;
}

} // namespace z1m

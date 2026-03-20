#include "game/play_core.hpp"

namespace z1m {

void tick_keese(Play* play, const World* world, Enemy* enemy, float dt_seconds) {
    tick_rom_flyer(play, world, enemy, nullptr, dt_seconds, kKeeseSpeed, 0xA0, false);
}

void tick_pols_voice(Play* play, const World* world, Enemy* enemy, float dt_seconds) {
    enemy->action_seconds_remaining -= dt_seconds;
    enemy->move_seconds_remaining -= dt_seconds;

    if (enemy->move_seconds_remaining <= 0.0F && enemy->action_seconds_remaining <= 0.0F) {
        const glm::vec2 direction = glm::normalize(
            glm::vec2(random_unit(play) * 2.0F - 1.0F, random_unit(play) * 2.0F - 1.0F));
        enemy->velocity = direction * kPolsVoiceSpeed;
        enemy->move_seconds_remaining = 0.20F + random_unit(play) * 0.10F;
        enemy->action_seconds_remaining = 0.38F + random_unit(play) * 0.25F;
    }

    if (enemy->move_seconds_remaining > 0.0F) {
        bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
    }
}

void tick_wallmaster(Play* play, const World* world, Enemy* enemy, const Player* player,
                     float dt_seconds) {
    if (player == nullptr) {
        return;
    }

    if (enemy->hidden) {
        enemy->action_seconds_remaining -= dt_seconds;
        if (enemy->action_seconds_remaining > 0.0F) {
            return;
        }

        const float left = player->position.x;
        const float right = static_cast<float>(world_width(world)) - player->position.x;
        const float top = player->position.y;
        const float bottom = static_cast<float>(world_height(world)) - player->position.y;

        if (left <= right && left <= top && left <= bottom) {
            enemy->position = glm::vec2(1.5F, player->position.y);
            enemy->velocity = glm::vec2(kWallmasterSpeed, 0.0F);
        } else if (right <= top && right <= bottom) {
            enemy->position =
                glm::vec2(static_cast<float>(world_width(world)) - 1.5F, player->position.y);
            enemy->velocity = glm::vec2(-kWallmasterSpeed, 0.0F);
        } else if (top <= bottom) {
            enemy->position = glm::vec2(player->position.x, 1.5F);
            enemy->velocity = glm::vec2(0.0F, kWallmasterSpeed);
        } else {
            enemy->position =
                glm::vec2(player->position.x, static_cast<float>(world_height(world)) - 1.5F);
            enemy->velocity = glm::vec2(0.0F, -kWallmasterSpeed);
        }

        enemy->origin = enemy->position;
        enemy->hidden = false;
        enemy->move_seconds_remaining = 1.8F;
        return;
    }

    enemy->move_seconds_remaining -= dt_seconds;
    const glm::vec2 candidate = enemy->position + enemy->velocity * dt_seconds;
    if (world_is_walkable_tile(world, candidate)) {
        enemy->position = candidate;
    }

    if (enemy->move_seconds_remaining > 0.0F) {
        return;
    }

    enemy->hidden = true;
    enemy->velocity = glm::vec2(0.0F);
    enemy->action_seconds_remaining = 0.8F + random_unit(play) * 0.8F;
}

void tick_dodongo(Play* play, const World* world, Enemy* enemy, const Player* player,
                  float dt_seconds) {
    enemy->state_seconds_remaining -= dt_seconds;
    if (enemy->state_seconds_remaining > 0.0F) {
        return;
    }

    if (enemy->special_counter == 1) {
        enemy->special_counter = 0;
    }

    enemy->move_seconds_remaining -= dt_seconds;
    if (enemy->move_seconds_remaining <= 0.0F) {
        if (player != nullptr && random_unit(play) < 0.55F) {
            choose_player_axis_direction(*enemy, *player, &enemy->facing);
        } else {
            choose_cardinal_direction(play, world, enemy);
        }
        enemy->move_seconds_remaining = 0.8F + random_unit(play) * 0.7F;
    }

    const glm::vec2 candidate =
        enemy->position + facing_vector(enemy->facing) * kDodongoSpeed * dt_seconds;
    if (world_is_walkable_tile(world, candidate)) {
        enemy->position = candidate;
    } else {
        enemy->move_seconds_remaining = 0.0F;
    }
}

void tick_gohma(Play* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds) {
    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);

    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining <= 0.0F) {
        enemy->velocity.x *= -1.0F;
        enemy->action_seconds_remaining = 0.7F + random_unit(play) * 0.6F;
    }

    enemy->state_seconds_remaining -= dt_seconds;
    if (enemy->state_seconds_remaining > 0.0F) {
        return;
    }

    if (enemy->special_counter == 0) {
        enemy->special_counter = 1;
        enemy->state_seconds_remaining = kGohmaEyeOpenSeconds;
        if (player != nullptr) {
            Facing shot_facing = enemy->facing;
            choose_player_axis_direction(*enemy, *player, &shot_facing);
            make_projectile(play, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                            enemy->position + facing_vector(shot_facing) * 0.8F,
                            facing_vector(shot_facing) * kFireSpeed, kProjectileLifetimeSeconds,
                            kFireRadius, 1);
        }
        return;
    }

    enemy->special_counter = 0;
    enemy->state_seconds_remaining = kGohmaEyeClosedSeconds + random_unit(play) * 0.6F;
}

void tick_moldorm(Play* play, const World* world, Enemy* enemy, const Player* player,
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

    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
}

void tick_digdogger(Play* play, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds) {
    const float speed = enemy->special_counter == 0 ? kDigdoggerSpeed : kDigdoggerSpeed * 1.35F;
    if (enemy->special_counter == 0) {
        enemy->invulnerable = true;
    }

    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining <= 0.0F) {
        glm::vec2 direction(random_unit(play) * 2.0F - 1.0F, random_unit(play) * 2.0F - 1.0F);
        if (player != nullptr && random_unit(play) < 0.6F) {
            direction = player->position - enemy->position;
        }
        if (glm::length(direction) < 0.1F) {
            direction = glm::vec2(1.0F, 0.0F);
        } else {
            direction = glm::normalize(direction);
        }
        enemy->velocity = direction * speed;
        enemy->action_seconds_remaining = 0.35F + random_unit(play) * 0.25F;
    }

    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
}

void tick_manhandla(Play* play, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds) {
    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining <= 0.0F) {
        if (player != nullptr) {
            const glm::vec2 toward_player = player->position - enemy->position;
            if (glm::length(toward_player) > 0.1F) {
                enemy->velocity = glm::normalize(toward_player) *
                                  (kManhandlaSpeed + static_cast<float>(6 - enemy->health) * 0.4F);
            }
        } else {
            enemy->velocity.x *= -1.0F;
        }

        const std::array<glm::vec2, 4> shots = {glm::vec2(1.0F, 0.0F), glm::vec2(-1.0F, 0.0F),
                                                glm::vec2(0.0F, 1.0F), glm::vec2(0.0F, -1.0F)};
        for (const glm::vec2& shot : shots) {
            make_projectile(play, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                            enemy->position + shot * 0.9F, shot * kFireSpeed,
                            kProjectileLifetimeSeconds, kFireRadius, 1);
        }
        enemy->action_seconds_remaining = 1.6F;
    }
}

void tick_gleeok(Play* play, const World* world, Enemy* enemy, const Player* player,
                 float dt_seconds) {
    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining > 0.0F) {
        return;
    }

    const int head_count = glm::max(enemy->special_counter, 1);
    for (int head = 0; head < head_count; ++head) {
        const float head_offset =
            (static_cast<float>(head) - static_cast<float>(head_count / 2)) * 0.8F;
        glm::vec2 direction = glm::vec2(0.0F, 1.0F);
        if (player != nullptr) {
            direction = player->position - (enemy->position + glm::vec2(head_offset, -0.6F));
        }
        if (glm::length(direction) < 0.1F) {
            direction = glm::vec2(0.0F, 1.0F);
        } else {
            direction = glm::normalize(direction);
        }

        make_projectile(play, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                        enemy->position + glm::vec2(head_offset, -0.6F), direction * kFireSpeed,
                        kProjectileLifetimeSeconds, kFireRadius, 1);
    }
    enemy->action_seconds_remaining = 1.2F;
}

void tick_patra(Play* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds) {
    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
    enemy->state_seconds_remaining -= dt_seconds;
    if (enemy->state_seconds_remaining <= 0.0F) {
        enemy->state_seconds_remaining = 4.0F;
        enemy->velocity *= -1.0F;
    }

    enemy->action_seconds_remaining -= dt_seconds;
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

void tick_ganon(Play* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds) {
    if (enemy->special_counter > 0) {
        enemy->state_seconds_remaining -= dt_seconds;
        if (enemy->state_seconds_remaining <= 0.0F) {
            enemy->special_counter = 0;
            enemy->hidden = true;
        } else {
            enemy->hidden = false;
        }
        return;
    }

    tick_blue_wizzrobe(play, world, enemy, player, dt_seconds);
    enemy->hidden = true;
}

void tick_zora(Play* play, const World* world, Enemy* enemy, const Player* player,
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

void tick_peahat(Play* play, const World* world, Enemy* enemy, float dt_seconds) {
    tick_rom_flyer(play, world, enemy, nullptr, dt_seconds, kPeahatSpeed, 0xB0, true);
}

void tick_aquamentus(Play* play, const World* world, Enemy* enemy, const Player* player,
                     float dt_seconds) {
    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
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

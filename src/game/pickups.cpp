#include "game/pickups.hpp"

#include "game/area_state.hpp"
#include "game/combat.hpp"
#include "game/enemies/ticks.hpp"
#include "game/geometry.hpp"
#include "game/items.hpp"
#include "game/tuning.hpp"

#include <algorithm>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <string>

namespace z1m {

void apply_player_pickup(Player* player, GameState* play, Pickup* pickup) {
    if (pickup->shop_item && player->rupees < pickup->price_rupees) {
        set_message(play, "need " + std::to_string(pickup->price_rupees) + " rupees", 1.4F);
        return;
    }

    if (pickup->shop_item) {
        player->rupees -= pickup->price_rupees;
    }

    switch (pickup->kind) {
    case PickupKind::None:
        break;
    case PickupKind::Sword:
        player->has_sword = true;
        play->sword_cave_reward_taken = true;
        set_message(play, "got wooden sword", 1.8F);
        break;
    case PickupKind::Heart:
        player->health = glm::min(player->max_health, player->health + 1);
        set_message(play, "heart recovered", 0.9F);
        break;
    case PickupKind::Rupee:
        player->rupees += pickup->shop_item ? 0 : 5;
        set_message(play, "rupees +5", 0.9F);
        break;
    case PickupKind::Bombs:
        player->bombs = glm::min(player->max_bombs, player->bombs + 4);
        player->max_bombs = glm::max(player->max_bombs, 8);
        select_if_unset(player, UseItemKind::Bombs);
        set_message(play, pickup->shop_item ? "bought bombs" : "bombs +4", 1.0F);
        break;
    case PickupKind::Boomerang:
        player->has_boomerang = true;
        select_if_unset(player, UseItemKind::Boomerang);
        set_message(play, pickup->shop_item ? "bought boomerang" : "got boomerang", 1.2F);
        break;
    case PickupKind::Bow:
        player->has_bow = true;
        select_if_unset(player, UseItemKind::Bow);
        set_message(play, pickup->shop_item ? "bought bow" : "got bow", 1.2F);
        break;
    case PickupKind::Candle:
        player->has_candle = true;
        select_if_unset(player, UseItemKind::Candle);
        set_message(play, pickup->shop_item ? "bought candle" : "got candle", 1.2F);
        break;
    case PickupKind::BluePotion:
        if (pickup->area_kind == AreaKind::ItemZoo && !player->has_letter) {
            set_message(play, "need letter for potion", 1.2F);
            return;
        }
        player->has_potion = true;
        select_if_unset(player, UseItemKind::Potion);
        set_message(play, pickup->shop_item ? "bought potion" : "got potion", 1.2F);
        break;
    case PickupKind::HeartContainer:
        player->max_health += 1;
        player->health = player->max_health;
        set_message(play, "heart container", 1.4F);
        break;
    case PickupKind::Key:
        player->keys += 1;
        set_message(play, "key +1", 1.0F);
        break;
    case PickupKind::Recorder:
        player->has_recorder = true;
        select_if_unset(player, UseItemKind::Recorder);
        set_message(play, pickup->shop_item ? "bought recorder" : "got recorder", 1.2F);
        break;
    case PickupKind::Ladder:
        player->has_ladder = true;
        set_message(play, pickup->shop_item ? "bought ladder" : "got ladder", 1.2F);
        break;
    case PickupKind::Raft:
        player->has_raft = true;
        set_message(play, pickup->shop_item ? "bought raft" : "got raft", 1.2F);
        break;
    case PickupKind::Food:
        player->has_food = true;
        select_if_unset(player, UseItemKind::Food);
        set_message(play, pickup->shop_item ? "bought bait" : "got bait", 1.2F);
        break;
    case PickupKind::Letter:
        player->has_letter = true;
        set_message(play, "got letter", 1.2F);
        break;
    case PickupKind::MagicShield:
        player->has_magic_shield = true;
        set_message(play, pickup->shop_item ? "bought magic shield" : "got magic shield", 1.3F);
        break;
    case PickupKind::SilverArrows:
        player->has_silver_arrows = true;
        set_message(play, pickup->shop_item ? "bought silver arrows" : "got silver arrows", 1.3F);
        break;
    }

    pickup->collected = true;
    pickup->active = false;
}

void tick_pickups(GameState* play, const World* overworld_world, Player* player, float dt_seconds) {
    for (Pickup& pickup : play->pickups) {
        if (!pickup.active) {
            continue;
        }

        if (!pickup.persistent) {
            pickup.seconds_remaining -= dt_seconds;
            pickup.velocity.y += kPickupGravityTilesPerSecond * dt_seconds;
            pickup.position += pickup.velocity * dt_seconds;
            if (pickup.seconds_remaining <= 0.0F) {
                pickup.active = false;
                continue;
            }
        }

        const World* world =
            get_world_for_area(play, overworld_world, pickup.area_kind, pickup.cave_id);
        if (!world_is_walkable_tile(world, pickup.position + glm::vec2(0.0F, 0.4F))) {
            pickup.velocity = glm::vec2(0.0F);
        }

        if (!pickup_in_current_area(play, pickup)) {
            continue;
        }

        const float radius = pickup.kind == PickupKind::Sword ? kSwordPickupRadius : kPickupRadius;
        if (!overlaps_circle(pickup.position, player->position, radius)) {
            continue;
        }

        apply_player_pickup(player, play, &pickup);
    }
}

void trigger_explosion(GameState* play, const Projectile& bomb) {
    make_projectile(play, bomb.area_kind, bomb.cave_id, ProjectileKind::Explosion, true,
                    bomb.position, glm::vec2(0.0F), kExplosionSeconds, kExplosionRadius, 2);
}

void tick_projectiles(GameState* play, const World* overworld_world, Player* player,
                      float dt_seconds) {
    for (Projectile& projectile : play->projectiles) {
        if (!projectile.active) {
            continue;
        }

        const World* world =
            get_world_for_area(play, overworld_world, projectile.area_kind, projectile.cave_id);
        const bool active_area = in_area(play, projectile.area_kind, projectile.cave_id);
        projectile.seconds_remaining -= dt_seconds;
        if (projectile.kind == ProjectileKind::Bomb && projectile.seconds_remaining <= 0.0F) {
            projectile.active = false;
            trigger_explosion(play, projectile);
            continue;
        }

        if (projectile.seconds_remaining <= 0.0F) {
            projectile.active = false;
            continue;
        }

        if (projectile.kind == ProjectileKind::Boomerang &&
            projectile.seconds_remaining <= kBoomerangFlightSeconds * 0.5F) {
            projectile.returning = true;
            const glm::vec2 toward_player = player->position - projectile.position;
            if (glm::length(toward_player) > 0.001F) {
                projectile.velocity = glm::normalize(toward_player) * kBoomerangSpeed;
            }
        }

        if (projectile.kind != ProjectileKind::Bomb && projectile.kind != ProjectileKind::Food &&
            projectile.kind != ProjectileKind::Explosion) {
            const glm::vec2 candidate = projectile.position + projectile.velocity * dt_seconds;
            if (!world_is_walkable_tile(world, candidate)) {
                if (projectile.kind == ProjectileKind::Boomerang) {
                    projectile.returning = true;
                    projectile.velocity *= -1.0F;
                } else {
                    projectile.active = false;
                    continue;
                }
            } else {
                projectile.position = candidate;
            }
        }

        if (projectile.from_player) {
            if (projectile.kind == ProjectileKind::Food) {
                continue;
            }

            for (Enemy& enemy : play->enemies) {
                if (!enemy.active || enemy.area_kind != projectile.area_kind ||
                    enemy.cave_id != projectile.cave_id ||
                    (enemy.hidden && enemy.kind != EnemyKind::Ganon)) {
                    continue;
                }

                const float radius = projectile.kind == ProjectileKind::Explosion
                                         ? kBombExplosionRadius
                                         : projectile.radius + 0.35F;
                if (!overlaps_circle(projectile.position, enemy.position, radius)) {
                    continue;
                }

                if (enemy.kind == EnemyKind::Bubble || enemy.kind == EnemyKind::Trap) {
                    continue;
                }

                if (enemy.kind == EnemyKind::PolsVoice) {
                    continue;
                }

                if ((enemy.kind == EnemyKind::BlueWizzrobe ||
                     enemy.kind == EnemyKind::RedWizzrobe) &&
                    projectile.kind != ProjectileKind::SwordBeam &&
                    projectile.kind != ProjectileKind::Fire &&
                    projectile.kind != ProjectileKind::Explosion) {
                    continue;
                }

                if (enemy.kind == EnemyKind::Dodongo) {
                    if (enemy.subtype != 0) {
                        continue;
                    }

                    if (!projectile_hits_dodongo_mouth(enemy, projectile)) {
                        continue;
                    }

                    enemy.special_counter += 1;
                    enemy.subtype = 1;
                    enemy.state_seconds_remaining = frames_to_seconds(0x20);
                    enemy.move_seconds_remaining = 0.0F;
                    enemy.action_seconds_remaining = 0.0F;
                    enemy.velocity = glm::vec2(0.0F);
                    set_message(play,
                                enemy.special_counter >= 2 ? "dodongo doomed" : "dodongo ate bomb",
                                0.8F);

                    projectile.active = false;
                    continue;
                }

                if (enemy.kind == EnemyKind::Gohma) {
                    if (projectile.kind != ProjectileKind::Arrow ||
                        (enemy.special_counter & 0x03) != 0x03 || projectile.velocity.y >= 0.0F) {
                        continue;
                    }
                }

                if (enemy.kind == EnemyKind::Patra && enemy.special_counter > 0) {
                    bool hit_orbiter = false;
                    for (int orbiter = 0; orbiter < enemy.special_counter; ++orbiter) {
                        if (!overlaps_circle(projectile.position,
                                             patra_orbiter_position(enemy, orbiter), radius)) {
                            continue;
                        }
                        enemy.special_counter -= 1;
                        hit_orbiter = true;
                        set_message(play, "patra orbiter down", 0.6F);
                        break;
                    }
                    projectile.active = false;
                    if (hit_orbiter) {
                        continue;
                    }
                    continue;
                }

                if (enemy.kind == EnemyKind::Manhandla && enemy.special_counter > 0) {
                    bool hit_petal = false;
                    for (int petal = 0; petal < enemy.special_counter; ++petal) {
                        if (!overlaps_circle(projectile.position,
                                             manhandla_petal_position(enemy, petal), radius)) {
                            continue;
                        }
                        enemy.special_counter -= 1;
                        enemy.health = glm::max(1, enemy.special_counter);
                        projectile.active = false;
                        if (enemy.special_counter <= 0) {
                            damage_enemy(play, &enemy, enemy.health);
                        } else {
                            set_message(play, "manhandla petal down", 0.6F);
                        }
                        hit_petal = true;
                        break;
                    }
                    if (hit_petal) {
                        continue;
                    }
                    continue;
                }

                if (enemy.kind == EnemyKind::Gleeok) {
                    bool hit_head = false;
                    for (int head = 0; head < glm::max(enemy.special_counter, 1); ++head) {
                        if (!overlaps_circle(projectile.position, gleeok_head_position(enemy, head),
                                             radius)) {
                            continue;
                        }
                        damage_enemy(play, &enemy, projectile.damage);
                        projectile.active = false;
                        hit_head = true;
                        break;
                    }
                    if (hit_head) {
                        continue;
                    }
                    continue;
                }

                if (enemy.kind == EnemyKind::Ganon) {
                    if (projectile.kind != ProjectileKind::Arrow || enemy.special_counter == 0 ||
                        !player->has_silver_arrows || enemy.health > 1) {
                        continue;
                    }
                    damage_enemy(play, &enemy, enemy.health);
                    projectile.active = false;
                    set_message(play, "ganon down", 1.0F);
                    continue;
                }

                damage_enemy(play, &enemy, projectile.damage);
                if (projectile.kind != ProjectileKind::Boomerang &&
                    projectile.kind != ProjectileKind::Explosion) {
                    projectile.active = false;
                }
            }

            if (projectile.kind == ProjectileKind::Boomerang && active_area &&
                overlaps_circle(projectile.position, player->position, 0.65F) &&
                projectile.returning) {
                projectile.active = false;
            }
            continue;
        }

        if (!active_area || !overlaps_circle(projectile.position, player->position,
                                             projectile.radius + kProjectileHitPadding)) {
            continue;
        }

        projectile.active = false;
        damage_player_from(play, world, player, projectile.damage, projectile.position);
    }
}

void compact_vectors(GameState* play) {
    play->projectiles.erase(
        std::remove_if(play->projectiles.begin(), play->projectiles.end(),
                       [](const Projectile& projectile) { return !projectile.active; }),
        play->projectiles.end());

    play->pickups.erase(std::remove_if(play->pickups.begin(), play->pickups.end(),
                                       [](const Pickup& pickup) {
                                           return !pickup.active && !pickup.persistent &&
                                                  !pickup.shop_item;
                                       }),
                        play->pickups.end());
}

} // namespace z1m

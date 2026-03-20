#include "game/play_core.hpp"

namespace z1m {

void ensure_sword_cave_pickup(Play* play) {
    if (play->sword_cave_reward_taken) {
        return;
    }

    for (const Pickup& pickup : play->pickups) {
        if (!pickup.active || pickup.kind != PickupKind::Sword || pickup.cave_id != kSwordCaveId) {
            continue;
        }

        return;
    }

    const CaveDef* cave = get_cave_def(kSwordCaveId);
    if (cave == nullptr) {
        return;
    }

    Pickup pickup;
    pickup.active = true;
    pickup.persistent = true;
    pickup.area_kind = AreaKind::Cave;
    pickup.cave_id = kSwordCaveId;
    pickup.kind = PickupKind::Sword;
    pickup.position = cave->reward_position;
    pickup.seconds_remaining = 9999.0F;
    play->pickups.push_back(pickup);
}

void init_opening_overworld_enemies(Play* play) {
    for (int room_id = 0; room_id < kScreenCount; ++room_id) {
        int spawn_count = 0;
        const EnemySpawnDef* spawns = get_room_enemy_spawns(room_id, &spawn_count);
        if (spawns == nullptr || spawn_count <= 0) {
            continue;
        }

        for (int index = 0; index < spawn_count; ++index) {
            Enemy enemy;
            enemy.active = true;
            enemy.kind = EnemyKind::Octorok;
            enemy.area_kind = AreaKind::Overworld;
            enemy.room_id = room_id;
            enemy.position = make_world_position(room_id, spawns[index].local_position);
            reset_enemy_state(play, &enemy);
            play->enemies.push_back(enemy);
        }
    }
}

void update_current_room(Play* play, const Player* player) {
    if (play->area_kind != AreaKind::Overworld) {
        play->current_room_id = -1;
        play->previous_room_id = -1;
        return;
    }

    play->current_room_id = get_room_from_position(player->position);
    if (play->current_room_id != play->previous_room_id) {
        play->previous_room_id = play->current_room_id;
    }
}

void spawn_pickup_drop(Play* play, const Enemy& enemy) {
    if (enemy.kind == EnemyKind::Moldorm && enemy.subtype > 0) {
        return;
    }

    Pickup pickup;
    pickup.active = true;
    pickup.area_kind = enemy.area_kind;
    pickup.cave_id = enemy.cave_id;
    pickup.room_id = enemy.room_id;
    pickup.position = enemy.position;
    pickup.velocity = glm::vec2(0.0F, -1.4F);
    pickup.seconds_remaining = kPickupLifetimeSeconds;

    if (enemy.kind == EnemyKind::Aquamentus) {
        pickup.kind = PickupKind::HeartContainer;
        pickup.persistent = true;
        pickup.seconds_remaining = 9999.0F;
        play->pickups.push_back(pickup);
        return;
    }

    const int roll = random_int(play, 7);
    if (roll <= 1) {
        pickup.kind = PickupKind::Heart;
    } else if (roll <= 4) {
        pickup.kind = PickupKind::Rupee;
    } else {
        pickup.kind = PickupKind::Bombs;
    }

    play->pickups.push_back(pickup);
}

void damage_player_from(Play* play, const World* world, Player* player, int damage,
                        const glm::vec2& source_position) {
    if (play->area_kind == AreaKind::EnemyZoo || play->area_kind == AreaKind::ItemZoo) {
        player->health = player->max_health;
        return;
    }

    if (player->invincibility_seconds > 0.0F || player->health <= 0) {
        return;
    }

    player->health = glm::max(0, player->health - damage);
    player->invincibility_seconds = kPlayerDamageInvincibilitySeconds;

    glm::vec2 push = player->position - source_position;
    if (glm::length(push) < 0.001F) {
        push = glm::vec2(0.0F, 1.0F);
    } else {
        push = glm::normalize(push);
    }

    const glm::vec2 candidate = player->position + push * 0.7F;
    if (world_is_walkable_tile(world, candidate)) {
        player->position = candidate;
    }

    if (player->health > 0) {
        return;
    }

    player->health = player->max_health;
    player->position = get_opening_start_position();
    player->facing = Facing::Down;
    play->area_kind = AreaKind::Overworld;
    play->current_cave_id = -1;
    play->cave_return_room_id = -1;
    play->cave_return_position = glm::vec2(0.0F);
    play->warp_cooldown_seconds = kAreaTransitionCooldownSeconds;
    update_current_room(play, player);
}

void damage_enemy(Play* play, Enemy* enemy, int damage) {
    if (!enemy->active || enemy->hidden || enemy->invulnerable ||
        enemy->hurt_seconds_remaining > 0.0F) {
        return;
    }

    const int previous_special_counter = enemy->special_counter;
    enemy->hurt_seconds_remaining = 0.20F;
    enemy->health -= damage;
    if (enemy->health > 0) {
        if (enemy->kind == EnemyKind::Gleeok && previous_special_counter > 1 &&
            enemy->special_counter > 0) {
            enemy->special_counter = previous_special_counter - 1;
            Enemy head;
            head.active = true;
            head.kind = EnemyKind::FlyingGhini;
            head.area_kind = enemy->area_kind;
            head.cave_id = enemy->cave_id;
            head.position = gleeok_head_position(*enemy, previous_special_counter - 1);
            head.spawn_position = head.position;
            head.origin = head.position;
            head.max_health = 2;
            head.health = 2;
            reset_enemy_state(play, &head);
            play->enemies.push_back(head);
        }
        return;
    }

    if (enemy->kind == EnemyKind::Zol) {
        for (int index = 0; index < 2; ++index) {
            Enemy child;
            child.active = true;
            child.kind = EnemyKind::Gel;
            child.area_kind = enemy->area_kind;
            child.cave_id = enemy->cave_id;
            child.position = enemy->position + glm::vec2(index == 0 ? -0.5F : 0.5F, 0.0F);
            child.spawn_position = child.position;
            child.origin = child.position;
            child.max_health = 1;
            child.health = 1;
            reset_enemy_state(play, &child);
            child.facing = index == 0 ? Facing::Left : Facing::Right;
            child.subtype = 1;
            child.action_seconds_remaining = 5.0F / 60.0F;
            play->enemies.push_back(child);
        }
    }

    if (enemy->kind == EnemyKind::Vire) {
        for (int index = 0; index < 2; ++index) {
            Enemy child;
            child.active = true;
            child.kind = EnemyKind::Keese;
            child.area_kind = enemy->area_kind;
            child.cave_id = enemy->cave_id;
            child.position = enemy->position + glm::vec2(index == 0 ? -0.6F : 0.6F, 0.0F);
            child.spawn_position = child.position;
            child.origin = child.position;
            child.max_health = 1;
            child.health = 1;
            reset_enemy_state(play, &child);
            play->enemies.push_back(child);
        }
    }

    if (enemy->zoo_respawn && enemy->area_kind == AreaKind::EnemyZoo) {
        enemy->active = false;
        enemy->respawn_seconds_remaining = 2.0F;
        return;
    }

    enemy->active = false;
    spawn_pickup_drop(play, *enemy);
}

void process_player_attacks(Play* play, Player* player) {
    if (!player->has_sword || !is_sword_active(player)) {
        return;
    }

    const glm::vec2 sword_pos = sword_world_position(player);
    for (Enemy& enemy : play->enemies) {
        if (!enemy_in_current_area(play, enemy)) {
            continue;
        }

        if (enemy.hidden && enemy.kind != EnemyKind::Ganon) {
            continue;
        }

        if (!overlaps_circle(enemy.position, sword_pos, kSwordHitRadius)) {
            continue;
        }

        if (enemy.kind == EnemyKind::Bubble || enemy.kind == EnemyKind::Trap ||
            enemy.kind == EnemyKind::Dodongo || enemy.kind == EnemyKind::Gohma) {
            continue;
        }

        if (enemy.kind == EnemyKind::Manhandla && enemy.special_counter > 0) {
            bool hit_petal = false;
            for (int petal = 0; petal < enemy.special_counter; ++petal) {
                if (!overlaps_circle(sword_pos, manhandla_petal_position(enemy, petal), 0.60F)) {
                    continue;
                }
                enemy.special_counter -= 1;
                enemy.health = glm::max(1, enemy.special_counter);
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
                if (!overlaps_circle(sword_pos, gleeok_head_position(enemy, head), 0.65F)) {
                    continue;
                }
                damage_enemy(play, &enemy, 1);
                hit_head = true;
                break;
            }
            if (hit_head) {
                continue;
            }
            continue;
        }

        if (enemy.kind == EnemyKind::Patra && enemy.special_counter > 0) {
            bool hit_orbiter = false;
            for (int orbiter = 0; orbiter < enemy.special_counter; ++orbiter) {
                if (!overlaps_circle(sword_pos, patra_orbiter_position(enemy, orbiter), 0.55F)) {
                    continue;
                }
                enemy.special_counter -= 1;
                set_message(play, "patra orbiter down", 0.6F);
                hit_orbiter = true;
                break;
            }
            if (hit_orbiter) {
                continue;
            }
            continue;
        }

        if (enemy.kind == EnemyKind::Ganon) {
            enemy.hidden = false;
            enemy.special_counter = 1;
            enemy.state_seconds_remaining = 1.0F;
            if (enemy.health > 1) {
                damage_enemy(play, &enemy, 1);
                if (enemy.active) {
                    enemy.hidden = false;
                    enemy.special_counter = 1;
                    enemy.state_seconds_remaining = 1.0F;
                }
            }
            set_message(play, enemy.health <= 1 ? "ganon is weak" : "ganon revealed", 0.6F);
            continue;
        }

        if (enemy.kind == EnemyKind::Darknut) {
            const glm::vec2 delta = sword_pos - enemy.position;
            bool blocked = false;
            switch (enemy.facing) {
            case Facing::Up:
                blocked = delta.y < 0.0F && std::abs(delta.y) >= std::abs(delta.x);
                break;
            case Facing::Down:
                blocked = delta.y > 0.0F && std::abs(delta.y) >= std::abs(delta.x);
                break;
            case Facing::Left:
                blocked = delta.x < 0.0F && std::abs(delta.x) >= std::abs(delta.y);
                break;
            case Facing::Right:
                blocked = delta.x > 0.0F && std::abs(delta.x) >= std::abs(delta.y);
                break;
            }
            if (blocked) {
                continue;
            }
        }

        damage_enemy(play, &enemy, 1);
    }
}

} // namespace z1m

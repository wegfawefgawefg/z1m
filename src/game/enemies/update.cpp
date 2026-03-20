#include "content/opening_content.hpp"
#include "game/area_state.hpp"
#include "game/combat.hpp"
#include "game/enemies/state.hpp"
#include "game/enemies/ticks.hpp"
#include "game/geometry.hpp"
#include "game/tuning.hpp"

#include <glm/common.hpp>

namespace z1m {

void tick_enemies(GameState* play, const World* overworld_world, Player* player, float dt_seconds) {
    for (Enemy& enemy : play->enemies) {
        if (!enemy.active) {
            continue;
        }

        const World* world =
            get_world_for_area(play, overworld_world, enemy.area_kind, enemy.cave_id);
        const Player* target_player =
            in_area(play, enemy.area_kind, enemy.cave_id) ? player : nullptr;
        enemy.room_id =
            enemy.area_kind == AreaKind::Overworld ? get_room_from_position(enemy.position) : -1;
        enemy.hurt_seconds_remaining = glm::max(0.0F, enemy.hurt_seconds_remaining - dt_seconds);
        switch (enemy.kind) {
        case EnemyKind::Octorok:
            tick_octorok_like(play, world, &enemy, target_player, dt_seconds, kOctorokSpeed, 1.1F,
                              0.9F, ProjectileKind::Rock);
            break;
        case EnemyKind::Moblin:
            tick_octorok_like(play, world, &enemy, target_player, dt_seconds, kMoblinSpeed, 0.8F,
                              0.7F, ProjectileKind::Arrow);
            break;
        case EnemyKind::Lynel:
            tick_rom_lynel(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Goriya:
            tick_goriya(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Darknut:
            tick_rom_darknut(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Tektite:
            tick_tektite(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Leever:
            tick_leever(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Keese:
            tick_keese(play, world, &enemy, dt_seconds);
            break;
        case EnemyKind::Zol:
            tick_rom_zol_or_gel(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Gel:
            tick_rom_zol_or_gel(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Rope:
            tick_rope(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Vire:
            tick_vire(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Stalfos:
            tick_stalfos(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Gibdo:
            tick_gibdo(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::LikeLike:
            tick_like_like(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::PolsVoice:
            tick_pols_voice(play, world, &enemy, dt_seconds);
            break;
        case EnemyKind::BlueWizzrobe:
            tick_blue_wizzrobe(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::RedWizzrobe:
            tick_red_wizzrobe(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Wallmaster:
            tick_wallmaster(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Ghini:
            tick_ghini(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Bubble:
            tick_bubble(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::FlyingGhini:
            tick_flying_ghini(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Trap:
            tick_trap(world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Armos:
            tick_armos(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Zora:
            tick_zora(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Peahat:
            tick_peahat(play, world, &enemy, dt_seconds);
            break;
        case EnemyKind::Dodongo:
            tick_dodongo(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Digdogger:
            tick_digdogger(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Manhandla:
            tick_manhandla(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Gohma:
            tick_gohma(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Moldorm:
            tick_moldorm(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Aquamentus:
            tick_aquamentus(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Gleeok:
            tick_gleeok(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Patra:
            tick_patra(play, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Ganon:
            tick_ganon(play, world, &enemy, target_player, dt_seconds);
            break;
        }

        if (target_player == nullptr || enemy.hidden) {
            continue;
        }

        if (enemy.kind == EnemyKind::Leever && enemy.special_counter != 3) {
            continue;
        }

        if (overlaps_circle(enemy.position, player->position, kEnemyTouchRadius)) {
            if (enemy.kind == EnemyKind::Wallmaster) {
                player->position = play->area_kind == AreaKind::Overworld
                                       ? get_opening_start_position()
                                       : enemy.origin;
                set_message(play, "wallmaster drag", 1.2F);
                continue;
            }

            if (enemy.kind == EnemyKind::Bubble) {
                if (enemy.subtype == 0) {
                    player->sword_disabled_seconds =
                        glm::max(player->sword_disabled_seconds, kBubbleCurseSeconds);
                    set_message(play, "bubble curse", 0.8F);
                } else if (enemy.subtype == 1) {
                    player->sword_cursed = true;
                    player->sword_disabled_seconds = 9999.0F;
                    set_message(play, "bubble block", 0.8F);
                } else {
                    player->sword_cursed = false;
                    player->sword_disabled_seconds = 0.0F;
                    set_message(play, "bubble restore", 0.8F);
                }
                continue;
            }

            if (enemy.kind == EnemyKind::LikeLike) {
                if (enemy.special_counter == 0) {
                    enemy.special_counter = 1;
                    enemy.action_seconds_remaining = 0.0F;
                }
                player->stunned_seconds = glm::max(player->stunned_seconds, 0.2F);
                player->position = enemy.position;
                if (enemy.special_counter == 1 &&
                    enemy.action_seconds_remaining >= frames_to_seconds(0x60) &&
                    player->has_magic_shield) {
                    player->has_magic_shield = false;
                    enemy.special_counter = 2;
                    set_message(play, "like-like ate shield", 1.0F);
                } else {
                    set_message(play, "like-like grab", 0.2F);
                }
                continue;
            }

            damage_player_from(play, world, player, 1, enemy.position);
        }
    }
}

void tick_enemy_respawns(GameState* play, float dt_seconds) {
    for (Enemy& enemy : play->enemies) {
        if (enemy.active || !enemy.zoo_respawn) {
            continue;
        }

        bool group_active = false;
        for (const Enemy& other : play->enemies) {
            if (!other.active || !other.zoo_respawn || other.respawn_group != enemy.respawn_group) {
                continue;
            }
            group_active = true;
            break;
        }

        if (group_active) {
            enemy.respawn_seconds_remaining = 2.0F;
            continue;
        }

        enemy.respawn_seconds_remaining =
            glm::max(0.0F, enemy.respawn_seconds_remaining - dt_seconds);
        if (enemy.respawn_seconds_remaining > 0.0F) {
            continue;
        }

        enemy.active = true;
        enemy.position = enemy.spawn_position;
        enemy.origin = enemy.spawn_position;
        reset_enemy_state(play, &enemy);
    }
}

void respawn_enemy_group_internal(GameState* play, int respawn_group) {
    for (Enemy& enemy : play->enemies) {
        if (enemy.respawn_group != respawn_group) {
            continue;
        }

        enemy.active = true;
        enemy.position = enemy.spawn_position;
        enemy.origin = enemy.spawn_position;
        enemy.respawn_seconds_remaining = 0.0F;
        reset_enemy_state(play, &enemy);
    }
}

} // namespace z1m

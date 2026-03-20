#pragma once

#include "game/play.hpp"

namespace z1m {

void bounce_velocity(const World* world, glm::vec2* position, glm::vec2* velocity, float dt);
float random_turn_timer_seconds(Play* play);
int random_byte(Play* play);
float frames_to_seconds(int frames);
bool try_fire_monster_projectile(Play* play, Enemy* enemy, ProjectileKind shot_kind);
bool tick_monster_shot_windup(Play* play, Enemy* enemy, ProjectileKind shot_kind, float dt_seconds);
void try_begin_monster_shot_windup(Play* play, Enemy* enemy, bool blue_walker);
void tick_rom_wanderer_shooter(Play* play, const World* world, Enemy* enemy, const Player* player,
                               float dt_seconds, float speed, int turn_rate_roll,
                               ProjectileKind shot_kind, bool blue_walker, bool fast_turns);
void tick_rom_goriya_movement(Play* play, const World* world, Enemy* enemy, const Player* player,
                              float dt_seconds, float speed);
void tick_rom_goriya_like(Play* play, const World* world, Enemy* enemy, const Player* player,
                          float dt_seconds, float speed, ProjectileKind shot_kind);
void tick_rom_lynel(Play* play, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds);
void tick_rom_darknut(Play* play, const World* world, Enemy* enemy, const Player* player,
                      float dt_seconds);
void tick_rom_common_wanderer(Play* play, const World* world, Enemy* enemy, const Player* player,
                              float dt_seconds, float speed, int turn_rate_roll);
void tick_octorok_like(Play* play, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds, float speed, float move_bias, float shoot_bias,
                       ProjectileKind shot_kind);
void tick_basic_walker(Play* play, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds, float speed, bool chase_player);
void tick_goriya(Play* play, const World* world, Enemy* enemy, const Player* player,
                 float dt_seconds);
float zol_or_gel_edge_delay_seconds(Play* play, EnemyKind kind);
void tick_rom_zol_or_gel(Play* play, const World* world, Enemy* enemy, const Player* player,
                         float dt_seconds);
void tick_rope(Play* play, const World* world, Enemy* enemy, const Player* player,
               float dt_seconds);
void tick_ghini(Play* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds);
void tick_flying_ghini(Play* play, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds);
void tick_vire(Play* play, const World* world, Enemy* enemy, const Player* player,
               float dt_seconds);
void tick_blue_wizzrobe(Play* play, const World* world, Enemy* enemy, const Player* player,
                        float dt_seconds);
void tick_red_wizzrobe(Play* play, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds);
void tick_armos(Play* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds);
void tick_trap(const World* world, Enemy* enemy, const Player* player, float dt_seconds);
void tick_tektite(Play* play, const World* world, Enemy* enemy, const Player* player,
                  float dt_seconds);
void tick_rom_flyer(Play* play, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds, float speed, int random_turn_roll,
                    bool invulnerable_when_moving);
void tick_leever(Play* play, const World* world, Enemy* enemy, const Player* player,
                 float dt_seconds);
void tick_keese(Play* play, const World* world, Enemy* enemy, float dt_seconds);
void tick_pols_voice(Play* play, const World* world, Enemy* enemy, float dt_seconds);
void tick_wallmaster(Play* play, const World* world, Enemy* enemy, const Player* player,
                     float dt_seconds);
void tick_dodongo(Play* play, const World* world, Enemy* enemy, const Player* player,
                  float dt_seconds);
void tick_gohma(Play* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds);
void tick_moldorm(Play* play, const World* world, Enemy* enemy, const Player* player,
                  float dt_seconds);
void tick_digdogger(Play* play, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds);
void tick_manhandla(Play* play, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds);
void tick_gleeok(Play* play, const World* world, Enemy* enemy, const Player* player,
                 float dt_seconds);
void tick_patra(Play* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds);
void tick_ganon(Play* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds);
void tick_zora(Play* play, const World* world, Enemy* enemy, const Player* player,
               float dt_seconds);
void tick_peahat(Play* play, const World* world, Enemy* enemy, float dt_seconds);
void tick_aquamentus(Play* play, const World* world, Enemy* enemy, const Player* player,
                     float dt_seconds);
void tick_enemies(Play* play, const World* overworld_world, Player* player, float dt_seconds);
void tick_enemy_respawns(Play* play, float dt_seconds);
void respawn_enemy_group_internal(Play* play, int respawn_group);

} // namespace z1m

#pragma once

#include "game/game_state.hpp"

namespace z1m {

bool enemy_can_move_to(const Enemy* enemy, const World* world, const glm::vec2& position);
void bounce_velocity(const Enemy* enemy, const World* world, glm::vec2* position,
                     glm::vec2* velocity, float dt);
float random_turn_timer_seconds(GameState* play);
int random_byte(GameState* play);
float frames_to_seconds(int frames);
bool try_fire_monster_projectile(GameState* play, Enemy* enemy, ProjectileKind shot_kind);
bool tick_monster_shot_windup(GameState* play, Enemy* enemy, ProjectileKind shot_kind,
                              float dt_seconds);
void try_begin_monster_shot_windup(GameState* play, Enemy* enemy, bool blue_walker);
void tick_rom_wanderer_shooter(GameState* play, const World* world, Enemy* enemy,
                               const Player* player, float dt_seconds, float speed,
                               int turn_rate_roll, ProjectileKind shot_kind, bool blue_walker,
                               bool fast_turns);
void tick_rom_goriya_movement(GameState* play, const World* world, Enemy* enemy,
                              const Player* player, float dt_seconds, float speed);
void tick_rom_goriya_like(GameState* play, const World* world, Enemy* enemy, const Player* player,
                          float dt_seconds, float speed, ProjectileKind shot_kind);
void tick_rom_lynel(GameState* play, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds);
void tick_rom_darknut(GameState* play, const World* world, Enemy* enemy, const Player* player,
                      float dt_seconds);
void tick_rom_common_wanderer(GameState* play, const World* world, Enemy* enemy,
                              const Player* player, float dt_seconds, float speed,
                              int turn_rate_roll);
void tick_octorok_like(GameState* play, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds, float speed, float move_bias, float shoot_bias,
                       ProjectileKind shot_kind);
void tick_basic_walker(GameState* play, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds, float speed, bool chase_player);
void tick_goriya(GameState* play, const World* world, Enemy* enemy, const Player* player,
                 float dt_seconds);
float zol_or_gel_edge_delay_seconds(GameState* play, EnemyKind kind);
void tick_rom_zol_or_gel(GameState* play, const World* world, Enemy* enemy, const Player* player,
                         float dt_seconds);
void tick_rope(GameState* play, const World* world, Enemy* enemy, const Player* player,
               float dt_seconds);
void tick_ghini(GameState* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds);
void tick_flying_ghini(GameState* play, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds);
void tick_vire(GameState* play, const World* world, Enemy* enemy, const Player* player,
               float dt_seconds);
void tick_blue_wizzrobe(GameState* play, const World* world, Enemy* enemy, const Player* player,
                        float dt_seconds);
void tick_red_wizzrobe(GameState* play, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds);
void tick_armos(GameState* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds);
void tick_trap(const World* world, Enemy* enemy, const Player* player, float dt_seconds);
void tick_tektite(GameState* play, const World* world, Enemy* enemy, const Player* player,
                  float dt_seconds);
void tick_rom_flyer(GameState* play, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds, float speed, int random_turn_roll,
                    bool invulnerable_when_moving);
void tick_leever(GameState* play, const World* world, Enemy* enemy, const Player* player,
                 float dt_seconds);
void tick_keese(GameState* play, const World* world, Enemy* enemy, float dt_seconds);
void tick_pols_voice(GameState* play, const World* world, Enemy* enemy, float dt_seconds);
void tick_wallmaster(GameState* play, const World* world, Enemy* enemy, const Player* player,
                     float dt_seconds);
void tick_dodongo(GameState* play, const World* world, Enemy* enemy, const Player* player,
                  float dt_seconds);
void tick_gohma(GameState* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds);
void tick_moldorm(GameState* play, const World* world, Enemy* enemy, const Player* player,
                  float dt_seconds);
void tick_digdogger(GameState* play, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds);
void tick_manhandla(GameState* play, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds);
void tick_gleeok(GameState* play, const World* world, Enemy* enemy, const Player* player,
                 float dt_seconds);
void tick_patra(GameState* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds);
void tick_ganon(GameState* play, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds);
void tick_zora(GameState* play, const World* world, Enemy* enemy, const Player* player,
               float dt_seconds);
void tick_peahat(GameState* play, const World* world, Enemy* enemy, float dt_seconds);
void tick_aquamentus(GameState* play, const World* world, Enemy* enemy, const Player* player,
                     float dt_seconds);
void tick_enemies(GameState* play, const World* overworld_world, Player* player, float dt_seconds);
void tick_enemy_respawns(GameState* play, float dt_seconds);
void respawn_enemy_group_internal(GameState* play, int respawn_group);

} // namespace z1m

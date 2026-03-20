#pragma once

#include "content/opening_content.hpp"
#include "content/overworld_warps.hpp"
#include "content/sandbox_content.hpp"
#include "game/play.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <string>

namespace z1m {

inline constexpr float kEnemyTouchRadius = 0.60F;
inline constexpr float kProjectileHitPadding = 0.35F;
inline constexpr float kPickupRadius = 0.65F;
inline constexpr float kSwordPickupRadius = 0.80F;
inline constexpr float kSwordHitRadius = 0.95F;
inline constexpr float kPlayerDamageInvincibilitySeconds = 1.0F;
inline constexpr float kAreaTransitionCooldownSeconds = 0.25F;
inline constexpr float kBombFuseSeconds = 1.0F;
inline constexpr float kExplosionSeconds = 0.22F;
inline constexpr float kBoomerangFlightSeconds = 0.65F;
inline constexpr float kFireSeconds = 0.55F;
inline constexpr float kFoodSeconds = 5.0F;
inline constexpr float kProjectileLifetimeSeconds = 2.5F;
inline constexpr float kPickupLifetimeSeconds = 8.0F;
inline constexpr float kPickupGravityTilesPerSecond = 10.0F;
inline constexpr float kBombExplosionRadius = 1.55F;
inline constexpr float kBoomerangRadius = 0.42F;
inline constexpr float kArrowRadius = 0.24F;
inline constexpr float kFireRadius = 0.44F;
inline constexpr float kRockRadius = 0.30F;
inline constexpr float kBombRadius = 0.42F;
inline constexpr float kExplosionRadius = 1.10F;
inline constexpr float kShootAlignThreshold = 1.0F;
inline constexpr float kWandererTargetThreshold = 1.125F;
inline constexpr float kGridCenterTolerance = 0.12F;
inline constexpr float kShootMaxDistance = 14.0F;
inline constexpr float kOctorokSpeed = 4.5F;
inline constexpr float kMoblinSpeed = 5.0F;
inline constexpr float kGoriyaSpeed = 4.8F;
inline constexpr float kDarknutSpeed = 4.2F;
inline constexpr float kGelSpeed = 2.8F;
inline constexpr float kZolSpeed = 3.6F;
inline constexpr float kRopeSpeed = 4.2F;
inline constexpr float kVireSpeed = 5.8F;
inline constexpr float kWizzrobeSpeed = 4.0F;
inline constexpr float kRopeChargeSpeed = 10.0F;
inline constexpr float kWalkerSpeed = 3.8F;
inline constexpr float kLikeLikeSpeed = 2.6F;
inline constexpr float kPolsVoiceSpeed = 6.8F;
inline constexpr float kWallmasterSpeed = 6.2F;
inline constexpr float kGhiniSpeed = 4.4F;
inline constexpr float kFlyingGhiniSpeed = 6.0F;
inline constexpr float kTrapSpeed = 13.0F;
inline constexpr float kArmosSpeed = 4.0F;
inline constexpr float kDodongoSpeed = 3.2F;
inline constexpr float kDigdoggerSpeed = 4.6F;
inline constexpr float kManhandlaSpeed = 4.8F;
inline constexpr float kGohmaSpeed = 5.2F;
inline constexpr float kMoldormSpeed = 6.0F;
inline constexpr float kKeeseSpeed = 5.7F;
inline constexpr float kLeeverSpeed = 4.2F;
inline constexpr float kTektiteHopSpeed = 7.2F;
inline constexpr float kAquamentusSpeed = 3.0F;
inline constexpr float kGleeokSpeed = 2.4F;
inline constexpr float kPatraSpeed = 5.4F;
inline constexpr float kGanonSpeed = 4.2F;
inline constexpr float kLynelSpeed = 5.7F;
inline constexpr float kPeahatSpeed = 6.6F;
inline constexpr float kRockSpeed = 8.0F;
inline constexpr float kArrowSpeed = 12.5F;
inline constexpr float kSwordBeamSpeed = 11.5F;
inline constexpr float kBoomerangSpeed = 9.2F;
inline constexpr float kFireSpeed = 7.5F;
inline constexpr float kLikeLikeGrabSeconds = 0.25F;
inline constexpr float kBubbleCurseSeconds = 3.0F;
inline constexpr float kDodongoBloatedSeconds = 1.2F;
inline constexpr float kDodongoStunnedSeconds = 0.8F;
inline constexpr float kGohmaEyeClosedSeconds = 1.1F;
inline constexpr float kGohmaEyeOpenSeconds = 0.8F;
inline constexpr float kWizzrobeTeleportSeconds = 0.55F;
inline constexpr float kPatraOrbitRadiusWide = 2.3F;
inline constexpr float kPatraOrbitRadiusTight = 1.25F;
inline constexpr int kSwordCaveId = 0x10;
inline constexpr float kDiag8 = 0.70710678F;
inline constexpr std::array<glm::vec2, 8> kDir8Vectors = {
    glm::vec2(1.0F, 0.0F),      glm::vec2(kDiag8, kDiag8),  glm::vec2(0.0F, 1.0F),
    glm::vec2(-kDiag8, kDiag8), glm::vec2(-1.0F, 0.0F),     glm::vec2(-kDiag8, -kDiag8),
    glm::vec2(0.0F, -1.0F),     glm::vec2(kDiag8, -kDiag8),
};

void update_current_room(Play* play, const Player* player);

std::uint32_t next_random(Play* play);
float random_unit(Play* play);
int random_int(Play* play, int max_value);
glm::vec2 facing_vector(Facing facing);
float qspeed_to_speed(int qspeed);
bool near_tile_center(const glm::vec2& position);
void snap_to_tile_center(glm::vec2* position);
Facing axis_facing_toward(const glm::vec2& from, const glm::vec2& to, bool vertical);
glm::vec2 eight_way_direction_toward(const glm::vec2& from, const glm::vec2& to);
glm::vec2 flyer_direction_toward(const glm::vec2& from, const glm::vec2& to);
int dir8_index_from_vector(const glm::vec2& direction);
glm::vec2 rotate_dir8_once_toward(const glm::vec2& current, const glm::vec2& target);
glm::vec2 rotate_dir8_random(Play* play, const glm::vec2& current);
bool choose_cardinal_shot_direction(const Play* play, const Enemy& enemy, const Player& player,
                                    Facing* facing_out);
bool overlaps_circle(const glm::vec2& a, const glm::vec2& b, float radius);
void set_message(Play* play, const std::string& text, float seconds);
Projectile* find_active_food(Play* play, AreaKind area_kind, int cave_id);
const World* get_world_for_area(const Play* play, const World* overworld_world, AreaKind area_kind,
                                int cave_id);
void clamp_enemy_to_zoo_pen(Enemy* enemy);
bool in_area(const Play* play, AreaKind area_kind, int cave_id);
bool enemy_in_current_area(const Play* play, const Enemy& enemy);
bool pickup_in_current_area(const Play* play, const Pickup& pickup);
int get_room_from_position(const glm::vec2& position);
void reset_enemy_state(Play* play, Enemy* enemy);

void choose_cardinal_direction(Play* play, const World* world, Enemy* enemy);
void choose_player_axis_direction(const Enemy& enemy, const Player& player, Facing* facing_out);
glm::vec2 dodongo_mouth_position(const Enemy& enemy);
bool projectile_hits_dodongo_mouth(const Enemy& enemy, const Projectile& projectile);
glm::vec2 manhandla_petal_position(const Enemy& enemy, int petal_index);
glm::vec2 gleeok_head_position(const Enemy& enemy, int head_index);
glm::vec2 orbit_offset(float angle_radians, float radius);
glm::vec2 patra_orbiter_position(const Enemy& enemy, int orbiter_index);
Projectile& spawn_projectile(Play* play);
void make_projectile(Play* play, AreaKind area_kind, int cave_id, ProjectileKind kind,
                     bool from_player, const glm::vec2& position, const glm::vec2& velocity,
                     float seconds_remaining, float radius, int damage);
void throw_enemy_rock(Play* play, const Enemy& enemy, const glm::vec2& velocity);
void throw_spread_rocks(Play* play, const Enemy& enemy, const glm::vec2& toward_player);
bool has_available_item(const Player* player, UseItemKind item);
void select_next_item(Player* player, int direction);
void select_if_unset(Player* player, UseItemKind item);
void create_bomb(Play* play, const Player* player);
void create_boomerang(Play* play, const Player* player);
void create_arrow(Play* play, const Player* player);
void create_sword_beam(Play* play, const Enemy& enemy, const glm::vec2& direction);
void create_player_sword_beam(Play* play, Player* player);
bool try_trigger_digdogger_split(Play* play);
void use_recorder(Play* play, const World* overworld_world, Player* player);
void create_fire(Play* play, const Player* player);
void create_food(Play* play, const Player* player);
void use_selected_item(Play* play, const World* overworld_world, Player* player);

void ensure_sword_cave_pickup(Play* play);
void init_opening_overworld_enemies(Play* play);
void spawn_pickup_drop(Play* play, const Enemy& enemy);
void damage_player_from(Play* play, const World* world, Player* player, int damage,
                        const glm::vec2& source_position);
void damage_enemy(Play* play, Enemy* enemy, int damage);
void process_player_attacks(Play* play, Player* player);

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

void apply_player_pickup(Player* player, Play* play, Pickup* pickup);
void tick_pickups(Play* play, const World* overworld_world, Player* player, float dt_seconds);
void trigger_explosion(Play* play, const Projectile& bomb);
void tick_projectiles(Play* play, const World* overworld_world, Player* player, float dt_seconds);
void compact_vectors(Play* play);

} // namespace z1m

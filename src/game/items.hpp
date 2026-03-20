#pragma once

#include "game/play.hpp"

namespace z1m {

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

} // namespace z1m

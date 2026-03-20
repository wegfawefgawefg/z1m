#pragma once

#include "game/area_data.hpp"
#include "game/entities.hpp"
#include "game/world.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace z1m {

struct Player;
struct PlayerCommand;

struct GameState {
    AreaKind area_kind = AreaKind::Overworld;
    int current_room_id = 0;
    int previous_room_id = -1;
    int current_cave_id = -1;
    int recorder_destination_index = -1;
    int cave_return_room_id = -1;
    glm::vec2 cave_return_position = glm::vec2(0.0F, 0.0F);
    float warp_cooldown_seconds = 0.0F;
    bool sword_cave_reward_taken = false;
    std::uint32_t rng_state = 0x13572468U;
    float message_seconds_remaining = 0.0F;
    std::string message_text = {};
    World cave_world = {};
    World enemy_zoo_world = {};
    World item_zoo_world = {};
    std::array<RoomRuntime, kScreenCount> room_runtime = {};
    std::vector<Enemy> enemies = {};
    std::vector<Projectile> projectiles = {};
    std::vector<Pickup> pickups = {};
    std::vector<Npc> npcs = {};
};

GameState make_game_state();
void init_game_state(GameState* game_state, Player* player);
void tick_game_state(GameState* game_state, const World* overworld_world, Player* player,
                     const PlayerCommand* command, float dt_seconds);
void respawn_enemy_group(GameState* game_state, int respawn_group);

} // namespace z1m

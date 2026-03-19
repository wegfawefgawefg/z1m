#pragma once

#include "game/player.hpp"
#include "game/world.hpp"

#include <array>
#include <cstdint>

namespace z1m {

enum class AreaKind {
    Overworld,
    Cave,
};

enum class EnemyKind {
    Octorok,
};

enum class PickupKind {
    None,
    Sword,
    Heart,
    Rupee,
    Bombs,
};

struct Enemy {
    bool active = false;
    EnemyKind kind = EnemyKind::Octorok;
    int room_id = -1;
    glm::vec2 position = glm::vec2(0.0F, 0.0F);
    Facing facing = Facing::Down;
    int health = 1;
    float move_seconds_remaining = 0.0F;
    float action_seconds_remaining = 0.0F;
    float hurt_seconds_remaining = 0.0F;
};

struct Projectile {
    bool active = false;
    int room_id = -1;
    glm::vec2 position = glm::vec2(0.0F, 0.0F);
    glm::vec2 velocity = glm::vec2(0.0F, 0.0F);
    float seconds_remaining = 0.0F;
};

struct Pickup {
    bool active = false;
    bool persistent = false;
    int room_id = -1;
    int cave_id = -1;
    PickupKind kind = PickupKind::None;
    glm::vec2 position = glm::vec2(0.0F, 0.0F);
    glm::vec2 velocity = glm::vec2(0.0F, 0.0F);
    float seconds_remaining = 0.0F;
};

struct RoomRuntime {
    bool cleared = false;
};

constexpr int kMaxEnemies = 16;
constexpr int kMaxProjectiles = 24;
constexpr int kMaxPickups = 24;

struct GameSession {
    AreaKind area_kind = AreaKind::Overworld;
    int current_room_id = 0;
    int previous_room_id = -1;
    int current_cave_id = -1;
    int cave_return_room_id = -1;
    glm::vec2 cave_return_position = glm::vec2(0.0F, 0.0F);
    float warp_cooldown_seconds = 0.0F;
    bool sword_cave_reward_taken = false;
    std::uint32_t rng_state = 0x13572468U;
    World cave_world = {};
    std::array<RoomRuntime, kScreenCount> room_runtime = {};
    std::array<Enemy, kMaxEnemies> enemies = {};
    std::array<Projectile, kMaxProjectiles> projectiles = {};
    std::array<Pickup, kMaxPickups> pickups = {};
};

GameSession make_game_session();
void init_game_session(GameSession* session, Player* player);
const World* get_active_world(const GameSession* session, const World* overworld_world);
void tick_game_session(GameSession* session, const World* overworld_world, Player* player,
                       const PlayerCommand* command, float dt_seconds);
const char* area_name(const GameSession* session);
const char* pickup_name(PickupKind kind);

} // namespace z1m

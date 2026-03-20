#pragma once

#include "game/player.hpp"
#include "game/world.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace z1m {

enum class AreaKind {
    Overworld,
    Cave,
    EnemyZoo,
    ItemZoo,
};

enum class EnemyKind {
    Octorok,
    Moblin,
    Lynel,
    Goriya,
    Darknut,
    Tektite,
    Leever,
    Keese,
    Zol,
    Gel,
    Rope,
    Vire,
    Stalfos,
    Gibdo,
    LikeLike,
    PolsVoice,
    BlueWizzrobe,
    RedWizzrobe,
    Wallmaster,
    Ghini,
    FlyingGhini,
    Bubble,
    Trap,
    Armos,
    Zora,
    Peahat,
    Dodongo,
    Digdogger,
    Manhandla,
    Gohma,
    Moldorm,
    Aquamentus,
    Gleeok,
    Patra,
    Ganon,
};

enum class PickupKind {
    None,
    Sword,
    Heart,
    Rupee,
    Bombs,
    Boomerang,
    Bow,
    Candle,
    BluePotion,
    HeartContainer,
    Key,
    Recorder,
    Ladder,
    Raft,
};

enum class ProjectileKind {
    Rock,
    Arrow,
    SwordBeam,
    Boomerang,
    Fire,
    Bomb,
    Explosion,
};

enum class NpcKind {
    OldMan,
    ShopKeeper,
};

struct Enemy {
    bool active = false;
    EnemyKind kind = EnemyKind::Octorok;
    AreaKind area_kind = AreaKind::Overworld;
    int cave_id = -1;
    int room_id = -1;
    glm::vec2 position = glm::vec2(0.0F, 0.0F);
    glm::vec2 spawn_position = glm::vec2(0.0F, 0.0F);
    glm::vec2 origin = glm::vec2(0.0F, 0.0F);
    glm::vec2 velocity = glm::vec2(0.0F, 0.0F);
    Facing facing = Facing::Down;
    int health = 1;
    int max_health = 1;
    int respawn_group = -1;
    int special_counter = 0;
    float move_seconds_remaining = 0.0F;
    float action_seconds_remaining = 0.0F;
    float hurt_seconds_remaining = 0.0F;
    float state_seconds_remaining = 0.0F;
    float respawn_seconds_remaining = 0.0F;
    bool hidden = false;
    bool invulnerable = false;
    bool zoo_respawn = false;
};

struct Projectile {
    bool active = false;
    ProjectileKind kind = ProjectileKind::Rock;
    AreaKind area_kind = AreaKind::Overworld;
    int cave_id = -1;
    int room_id = -1;
    bool from_player = false;
    glm::vec2 position = glm::vec2(0.0F, 0.0F);
    glm::vec2 velocity = glm::vec2(0.0F, 0.0F);
    glm::vec2 origin = glm::vec2(0.0F, 0.0F);
    float seconds_remaining = 0.0F;
    float radius = 0.30F;
    int damage = 1;
    bool returning = false;
};

struct Pickup {
    bool active = false;
    bool persistent = false;
    bool shop_item = false;
    bool collected = false;
    AreaKind area_kind = AreaKind::Overworld;
    int room_id = -1;
    int cave_id = -1;
    PickupKind kind = PickupKind::None;
    glm::vec2 position = glm::vec2(0.0F, 0.0F);
    glm::vec2 velocity = glm::vec2(0.0F, 0.0F);
    float seconds_remaining = 0.0F;
    int price_rupees = 0;
};

struct Npc {
    bool active = false;
    AreaKind area_kind = AreaKind::Overworld;
    int cave_id = -1;
    NpcKind kind = NpcKind::OldMan;
    glm::vec2 position = glm::vec2(0.0F, 0.0F);
    int room_id = -1;
    int shop_item_index = -1;
    const char* label = "";
};

struct AreaPortal {
    AreaKind source_area_kind = AreaKind::Overworld;
    int source_cave_id = -1;
    glm::vec2 center = glm::vec2(0.0F, 0.0F);
    glm::vec2 half_size = glm::vec2(0.0F, 0.0F);
    AreaKind target_area_kind = AreaKind::Overworld;
    int target_cave_id = -1;
    glm::vec2 target_position = glm::vec2(0.0F, 0.0F);
    bool requires_raft = false;
    const char* label = "";
};

struct RoomRuntime {
    bool cleared = false;
};

constexpr int kMaxAreaPortals = 8;

struct GameSession {
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

GameSession make_game_session();
void init_game_session(GameSession* session, Player* player);
const World* get_active_world(const GameSession* session, const World* overworld_world);
void set_area_kind(GameSession* session, Player* player, AreaKind area_kind, int cave_id,
                   const glm::vec2& position);
int gather_area_portals(const GameSession* session,
                        std::array<AreaPortal, kMaxAreaPortals>* portals);
void tick_game_session(GameSession* session, const World* overworld_world, Player* player,
                       const PlayerCommand* command, float dt_seconds);
void respawn_enemy_group(GameSession* session, int respawn_group);
const char* area_name(const GameSession* session);
const char* pickup_name(PickupKind kind);
const char* enemy_name(EnemyKind kind);
const char* projectile_name(ProjectileKind kind);
const char* npc_name(NpcKind kind);

} // namespace z1m

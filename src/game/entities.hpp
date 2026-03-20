#pragma once

#include "game/area_data.hpp"
#include "game/player.hpp"

namespace z1m {

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
    Food,
    Letter,
    MagicShield,
    SilverArrows,
};

enum class ProjectileKind {
    Rock,
    Arrow,
    SwordBeam,
    Boomerang,
    Fire,
    Food,
    Bomb,
    Explosion,
};

enum class NpcKind {
    OldMan,
    ShopKeeper,
    HungryGoriya,
    OldWoman,
    Fairy,
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
    int subtype = 0;
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
    glm::vec2 origin = glm::vec2(0.0F, 0.0F);
    int room_id = -1;
    int subtype = 0;
    int shop_item_index = -1;
    float action_seconds_remaining = 0.0F;
    float state_seconds_remaining = 0.0F;
    bool solved = false;
    const char* label = "";
};

} // namespace z1m

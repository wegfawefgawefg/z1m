#include "game/game_session.hpp"

#include "content/opening_content.hpp"
#include "content/overworld_warps.hpp"
#include "content/sandbox_content.hpp"

#include <algorithm>
#include <cmath>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace z1m {

namespace {

constexpr float kEnemyTouchRadius = 0.60F;
constexpr float kProjectileHitPadding = 0.35F;
constexpr float kPickupRadius = 0.65F;
constexpr float kSwordPickupRadius = 0.80F;
constexpr float kSwordHitRadius = 0.95F;
constexpr float kPlayerDamageInvincibilitySeconds = 1.0F;
constexpr float kAreaTransitionCooldownSeconds = 0.25F;
constexpr float kBombFuseSeconds = 1.0F;
constexpr float kExplosionSeconds = 0.22F;
constexpr float kBoomerangFlightSeconds = 0.65F;
constexpr float kFireSeconds = 0.55F;
constexpr float kFoodSeconds = 5.0F;
constexpr float kProjectileLifetimeSeconds = 2.5F;
constexpr float kPickupLifetimeSeconds = 8.0F;
constexpr float kPickupGravityTilesPerSecond = 10.0F;
constexpr float kBombExplosionRadius = 1.55F;
constexpr float kBoomerangRadius = 0.42F;
constexpr float kArrowRadius = 0.24F;
constexpr float kFireRadius = 0.44F;
constexpr float kRockRadius = 0.30F;
constexpr float kBombRadius = 0.42F;
constexpr float kExplosionRadius = 1.10F;
constexpr float kShootAlignThreshold = 1.0F;
constexpr float kShootMaxDistance = 14.0F;
constexpr float kOctorokSpeed = 4.5F;
constexpr float kMoblinSpeed = 5.0F;
constexpr float kGoriyaSpeed = 4.8F;
constexpr float kDarknutSpeed = 4.2F;
constexpr float kGelSpeed = 2.8F;
constexpr float kZolSpeed = 3.6F;
constexpr float kRopeSpeed = 4.2F;
constexpr float kVireSpeed = 5.8F;
constexpr float kWizzrobeSpeed = 4.0F;
constexpr float kRopeChargeSpeed = 10.0F;
constexpr float kWalkerSpeed = 3.8F;
constexpr float kLikeLikeSpeed = 2.6F;
constexpr float kPolsVoiceSpeed = 6.8F;
constexpr float kWallmasterSpeed = 6.2F;
constexpr float kGhiniSpeed = 4.4F;
constexpr float kFlyingGhiniSpeed = 6.0F;
constexpr float kTrapSpeed = 13.0F;
constexpr float kArmosSpeed = 4.0F;
constexpr float kDodongoSpeed = 3.2F;
constexpr float kDigdoggerSpeed = 4.6F;
constexpr float kManhandlaSpeed = 4.8F;
constexpr float kGohmaSpeed = 5.2F;
constexpr float kMoldormSpeed = 6.0F;
constexpr float kKeeseSpeed = 5.7F;
constexpr float kLeeverSpeed = 4.2F;
constexpr float kTektiteHopSpeed = 7.2F;
constexpr float kAquamentusSpeed = 3.0F;
constexpr float kGleeokSpeed = 2.4F;
constexpr float kPatraSpeed = 5.4F;
constexpr float kGanonSpeed = 4.2F;
constexpr float kLynelSpeed = 5.7F;
constexpr float kPeahatSpeed = 6.6F;
constexpr float kRockSpeed = 8.0F;
constexpr float kArrowSpeed = 12.5F;
constexpr float kSwordBeamSpeed = 11.5F;
constexpr float kBoomerangSpeed = 9.2F;
constexpr float kFireSpeed = 7.5F;
constexpr float kLikeLikeGrabSeconds = 0.25F;
constexpr float kBubbleCurseSeconds = 3.0F;
constexpr float kDodongoBloatedSeconds = 1.2F;
constexpr float kDodongoStunnedSeconds = 0.8F;
constexpr float kGohmaEyeClosedSeconds = 1.1F;
constexpr float kGohmaEyeOpenSeconds = 0.8F;
constexpr float kWizzrobeTeleportSeconds = 0.55F;
constexpr float kPatraOrbitRadiusWide = 2.3F;
constexpr float kPatraOrbitRadiusTight = 1.25F;
constexpr int kSwordCaveId = 0x10;

void update_current_room(GameSession* session, const Player* player);

std::uint32_t next_random(GameSession* session) {
    session->rng_state = session->rng_state * 1664525U + 1013904223U;
    return session->rng_state;
}

float random_unit(GameSession* session) {
    const std::uint32_t value = next_random(session) >> 8;
    return static_cast<float>(value & 0x00FFFFFFU) / static_cast<float>(0x01000000U);
}

int random_int(GameSession* session, int max_value) {
    if (max_value <= 0) {
        return 0;
    }

    return static_cast<int>(next_random(session) % static_cast<std::uint32_t>(max_value));
}

glm::vec2 facing_vector(Facing facing) {
    switch (facing) {
    case Facing::Up:
        return glm::vec2(0.0F, -1.0F);
    case Facing::Down:
        return glm::vec2(0.0F, 1.0F);
    case Facing::Left:
        return glm::vec2(-1.0F, 0.0F);
    case Facing::Right:
        return glm::vec2(1.0F, 0.0F);
    }

    return glm::vec2(0.0F, 1.0F);
}

bool choose_cardinal_shot_direction(const GameSession* session, const Enemy& enemy,
                                    const Player& player, Facing* facing_out) {
    if (enemy.area_kind == AreaKind::Overworld && enemy.room_id != session->current_room_id) {
        return false;
    }

    const glm::vec2 delta = player.position - enemy.position;
    if (glm::length(delta) > kShootMaxDistance) {
        return false;
    }

    if (std::abs(delta.x) <= kShootAlignThreshold) {
        *facing_out = delta.y < 0.0F ? Facing::Up : Facing::Down;
        return true;
    }

    if (std::abs(delta.y) <= kShootAlignThreshold) {
        *facing_out = delta.x < 0.0F ? Facing::Left : Facing::Right;
        return true;
    }

    return false;
}

bool overlaps_circle(const glm::vec2& a, const glm::vec2& b, float radius) {
    return glm::length(a - b) <= radius;
}

void set_message(GameSession* session, const std::string& text, float seconds) {
    session->message_text = text;
    session->message_seconds_remaining = seconds;
}

Projectile* find_active_food(GameSession* session, AreaKind area_kind, int cave_id) {
    for (Projectile& projectile : session->projectiles) {
        if (!projectile.active || projectile.kind != ProjectileKind::Food ||
            projectile.area_kind != area_kind || projectile.cave_id != cave_id) {
            continue;
        }
        return &projectile;
    }
    return nullptr;
}

const World* get_world_for_area(const GameSession* session, const World* overworld_world,
                                AreaKind area_kind, int cave_id) {
    switch (area_kind) {
    case AreaKind::Overworld:
        return overworld_world;
    case AreaKind::Cave:
        if (session->current_cave_id == cave_id || cave_id >= 0) {
            return &session->cave_world;
        }
        return &session->cave_world;
    case AreaKind::EnemyZoo:
        return &session->enemy_zoo_world;
    case AreaKind::ItemZoo:
        return &session->item_zoo_world;
    }

    return overworld_world;
}

bool in_area(const GameSession* session, AreaKind area_kind, int cave_id) {
    if (session->area_kind != area_kind) {
        return false;
    }

    if (area_kind == AreaKind::Cave) {
        return session->current_cave_id == cave_id;
    }

    return true;
}

bool enemy_in_current_area(const GameSession* session, const Enemy& enemy) {
    return enemy.active && in_area(session, enemy.area_kind, enemy.cave_id);
}

bool pickup_in_current_area(const GameSession* session, const Pickup& pickup) {
    return pickup.active && in_area(session, pickup.area_kind, pickup.cave_id);
}

int get_room_from_position(const glm::vec2& position) {
    return get_room_id_at_world_tile(static_cast<int>(position.x), static_cast<int>(position.y));
}

void reset_enemy_state(GameSession* session, Enemy* enemy) {
    enemy->health = glm::max(enemy->max_health, 1);
    enemy->hurt_seconds_remaining = 0.0F;
    enemy->hidden = false;
    enemy->invulnerable = false;
    enemy->velocity = glm::vec2(0.0F, 0.0F);
    enemy->special_counter = 0;

    switch (enemy->kind) {
    case EnemyKind::Octorok:
        enemy->move_seconds_remaining = 0.25F + random_unit(session) * 0.40F;
        enemy->action_seconds_remaining = 0.70F + random_unit(session) * 0.90F;
        break;
    case EnemyKind::Moblin:
        enemy->move_seconds_remaining = 0.25F + random_unit(session) * 0.45F;
        enemy->action_seconds_remaining = 0.55F + random_unit(session) * 0.80F;
        break;
    case EnemyKind::Lynel:
    case EnemyKind::Goriya:
    case EnemyKind::Darknut:
        enemy->move_seconds_remaining = 0.25F + random_unit(session) * 0.35F;
        enemy->action_seconds_remaining = 0.45F + random_unit(session) * 0.65F;
        break;
    case EnemyKind::Tektite:
        enemy->move_seconds_remaining = 0.0F;
        enemy->action_seconds_remaining = 0.25F + random_unit(session) * 0.35F;
        break;
    case EnemyKind::Leever:
        enemy->hidden = true;
        enemy->state_seconds_remaining = 0.4F + random_unit(session) * 0.4F;
        enemy->action_seconds_remaining = 0.0F;
        enemy->move_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Keese:
    case EnemyKind::Ghini:
        enemy->action_seconds_remaining = 0.25F + random_unit(session) * 0.30F;
        enemy->move_seconds_remaining = 1.0F + random_unit(session) * 1.2F;
        break;
    case EnemyKind::Bubble:
        enemy->invulnerable = true;
        enemy->action_seconds_remaining = 0.18F + random_unit(session) * 0.25F;
        enemy->move_seconds_remaining = 1.0F + random_unit(session) * 1.0F;
        break;
    case EnemyKind::FlyingGhini:
        enemy->action_seconds_remaining = 0.15F + random_unit(session) * 0.20F;
        enemy->move_seconds_remaining = 1.0F + random_unit(session) * 1.2F;
        break;
    case EnemyKind::Zol:
    case EnemyKind::Gel:
    case EnemyKind::Vire:
    case EnemyKind::Stalfos:
    case EnemyKind::Gibdo:
    case EnemyKind::LikeLike:
    case EnemyKind::PolsVoice:
    case EnemyKind::Rope:
    case EnemyKind::Armos:
        enemy->move_seconds_remaining = 0.25F + random_unit(session) * 0.45F;
        enemy->action_seconds_remaining = 0.35F + random_unit(session) * 0.55F;
        break;
    case EnemyKind::Wallmaster:
        enemy->hidden = true;
        enemy->action_seconds_remaining = 0.8F + random_unit(session) * 0.8F;
        enemy->move_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Trap:
        enemy->invulnerable = true;
        enemy->velocity = glm::vec2(0.0F);
        enemy->state_seconds_remaining = 0.0F;
        enemy->action_seconds_remaining = 0.0F;
        enemy->move_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Zora:
        enemy->hidden = true;
        enemy->state_seconds_remaining = 1.0F + random_unit(session) * 0.8F;
        enemy->action_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Peahat:
        enemy->invulnerable = true;
        enemy->state_seconds_remaining = 1.8F + random_unit(session) * 0.8F;
        enemy->action_seconds_remaining = 0.15F;
        enemy->move_seconds_remaining = 0.55F;
        break;
    case EnemyKind::BlueWizzrobe:
        enemy->action_seconds_remaining = 1.1F + random_unit(session) * 0.6F;
        enemy->move_seconds_remaining = 0.35F;
        break;
    case EnemyKind::RedWizzrobe:
        enemy->hidden = true;
        enemy->state_seconds_remaining = 0.9F + random_unit(session) * 0.7F;
        enemy->action_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Dodongo:
        enemy->facing = Facing::Right;
        enemy->move_seconds_remaining = 0.9F + random_unit(session) * 0.6F;
        enemy->action_seconds_remaining = 0.0F;
        enemy->state_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Digdogger:
        enemy->velocity = glm::vec2(kDigdoggerSpeed, 0.0F);
        enemy->action_seconds_remaining = 0.45F;
        enemy->move_seconds_remaining = 9999.0F;
        break;
    case EnemyKind::Manhandla:
        enemy->special_counter = 4;
        enemy->velocity = glm::vec2(kManhandlaSpeed, kManhandlaSpeed * 0.65F);
        enemy->action_seconds_remaining = 1.0F;
        enemy->move_seconds_remaining = 9999.0F;
        break;
    case EnemyKind::Gohma:
        enemy->facing = Facing::Right;
        enemy->velocity = glm::vec2(enemy->subtype == 0 ? kGohmaSpeed : kGohmaSpeed * 1.15F, 0.0F);
        enemy->action_seconds_remaining = enemy->subtype == 0 ? 1.0F : 0.7F;
        enemy->state_seconds_remaining =
            (enemy->subtype == 0 ? kGohmaEyeClosedSeconds : kGohmaEyeClosedSeconds * 0.7F);
        break;
    case EnemyKind::Moldorm:
        enemy->facing = Facing::Right;
        enemy->action_seconds_remaining = 0.3F + random_unit(session) * 0.4F;
        enemy->move_seconds_remaining = 9999.0F;
        enemy->velocity = enemy->subtype == 0 ? glm::vec2(kMoldormSpeed, 0.0F) : glm::vec2(0.0F);
        break;
    case EnemyKind::Aquamentus:
        enemy->velocity = glm::vec2(kAquamentusSpeed, 0.0F);
        enemy->action_seconds_remaining = 0.9F;
        enemy->move_seconds_remaining = 9999.0F;
        break;
    case EnemyKind::Gleeok:
        enemy->velocity = glm::vec2(kGleeokSpeed, 0.0F);
        enemy->action_seconds_remaining = 0.8F;
        enemy->move_seconds_remaining = 9999.0F;
        enemy->special_counter = enemy->subtype > 0 ? enemy->subtype : 3;
        break;
    case EnemyKind::Patra:
        enemy->velocity = glm::vec2(kPatraSpeed, 0.0F);
        enemy->action_seconds_remaining = 0.0F;
        enemy->state_seconds_remaining = 4.0F;
        enemy->special_counter = 8;
        break;
    case EnemyKind::Ganon:
        enemy->hidden = true;
        enemy->action_seconds_remaining = 0.6F;
        enemy->state_seconds_remaining = kWizzrobeTeleportSeconds;
        break;
    }
}

void choose_cardinal_direction(GameSession* session, const World* world, Enemy* enemy) {
    const std::array<Facing, 4> facings = {Facing::Up, Facing::Down, Facing::Left, Facing::Right};

    for (int attempt = 0; attempt < 8; ++attempt) {
        const Facing facing = facings[static_cast<std::size_t>(random_int(session, 4))];
        const glm::vec2 probe = enemy->position + facing_vector(facing) * 0.8F;
        if (!world_is_walkable_tile(world, probe)) {
            continue;
        }

        enemy->facing = facing;
        enemy->move_seconds_remaining = 0.30F + random_unit(session) * 0.80F;
        return;
    }

    enemy->move_seconds_remaining = 0.15F;
}

void choose_player_axis_direction(const Enemy& enemy, const Player& player, Facing* facing_out) {
    const glm::vec2 delta = player.position - enemy.position;
    if (std::abs(delta.x) > std::abs(delta.y)) {
        *facing_out = delta.x < 0.0F ? Facing::Left : Facing::Right;
        return;
    }

    *facing_out = delta.y < 0.0F ? Facing::Up : Facing::Down;
}

glm::vec2 dodongo_mouth_position(const Enemy& enemy) {
    return enemy.position + facing_vector(enemy.facing) * 0.95F;
}

bool projectile_hits_dodongo_mouth(const Enemy& enemy, const Projectile& projectile) {
    if (projectile.kind != ProjectileKind::Bomb && projectile.kind != ProjectileKind::Explosion) {
        return false;
    }

    return overlaps_circle(projectile.position, dodongo_mouth_position(enemy), 1.0F);
}

glm::vec2 manhandla_petal_position(const Enemy& enemy, int petal_index) {
    static constexpr std::array<glm::vec2, 4> offsets = {
        glm::vec2(0.9F, 0.0F),
        glm::vec2(-0.9F, 0.0F),
        glm::vec2(0.0F, 0.9F),
        glm::vec2(0.0F, -0.9F),
    };
    const int index = glm::clamp(petal_index, 0, 3);
    return enemy.position + offsets[static_cast<std::size_t>(index)];
}

glm::vec2 gleeok_head_position(const Enemy& enemy, int head_index) {
    const int head_count = glm::max(enemy.special_counter, 1);
    const float head_offset =
        (static_cast<float>(head_index) - static_cast<float>(head_count - 1) * 0.5F) * 0.8F;
    return enemy.position + glm::vec2(head_offset, -0.9F);
}

glm::vec2 orbit_offset(float angle_radians, float radius) {
    return glm::vec2(std::cos(angle_radians), std::sin(angle_radians)) * radius;
}

glm::vec2 patra_orbiter_position(const Enemy& enemy, int orbiter_index) {
    const float radius =
        enemy.state_seconds_remaining > 2.0F ? kPatraOrbitRadiusWide : kPatraOrbitRadiusTight;
    const float phase = enemy.action_seconds_remaining * 3.2F;
    const float angle = phase + static_cast<float>(orbiter_index) * (6.28318530718F / 8.0F);
    return enemy.position + orbit_offset(angle, radius);
}

Projectile& spawn_projectile(GameSession* session) {
    session->projectiles.push_back(Projectile{});
    Projectile& projectile = session->projectiles.back();
    projectile.active = true;
    return projectile;
}

void make_projectile(GameSession* session, AreaKind area_kind, int cave_id, ProjectileKind kind,
                     bool from_player, const glm::vec2& position, const glm::vec2& velocity,
                     float seconds_remaining, float radius, int damage) {
    Projectile& projectile = spawn_projectile(session);
    projectile.kind = kind;
    projectile.area_kind = area_kind;
    projectile.cave_id = cave_id;
    projectile.from_player = from_player;
    projectile.position = position;
    projectile.origin = position;
    projectile.velocity = velocity;
    projectile.seconds_remaining = seconds_remaining;
    projectile.radius = radius;
    projectile.damage = damage;
}

void throw_enemy_rock(GameSession* session, const Enemy& enemy, const glm::vec2& velocity) {
    make_projectile(session, enemy.area_kind, enemy.cave_id, ProjectileKind::Rock, false,
                    enemy.position + glm::normalize(velocity) * 0.75F, velocity,
                    kProjectileLifetimeSeconds, kRockRadius, 1);
}

void throw_spread_rocks(GameSession* session, const Enemy& enemy, const glm::vec2& toward_player) {
    const glm::vec2 base = glm::normalize(toward_player);
    const glm::vec2 left = glm::normalize(base + glm::vec2(-0.35F, -0.15F));
    const glm::vec2 right = glm::normalize(base + glm::vec2(0.35F, -0.15F));
    throw_enemy_rock(session, enemy, left * kRockSpeed);
    throw_enemy_rock(session, enemy, base * kRockSpeed);
    throw_enemy_rock(session, enemy, right * kRockSpeed);
}

bool has_available_item(const Player* player, UseItemKind item) {
    switch (item) {
    case UseItemKind::None:
        return false;
    case UseItemKind::Bombs:
        return player->max_bombs > 0;
    case UseItemKind::Boomerang:
        return player->has_boomerang;
    case UseItemKind::Bow:
        return player->has_bow;
    case UseItemKind::Candle:
        return player->has_candle;
    case UseItemKind::Recorder:
        return player->has_recorder;
    case UseItemKind::Food:
        return player->has_food;
    case UseItemKind::Potion:
        return player->has_potion;
    }

    return false;
}

void select_next_item(Player* player, int direction) {
    constexpr std::array<UseItemKind, 7> order = {
        UseItemKind::Bombs,    UseItemKind::Boomerang, UseItemKind::Bow,    UseItemKind::Candle,
        UseItemKind::Recorder, UseItemKind::Food,      UseItemKind::Potion,
    };

    int selected_index = -1;
    for (int index = 0; index < static_cast<int>(order.size()); ++index) {
        if (order[static_cast<std::size_t>(index)] == player->selected_item) {
            selected_index = index;
            break;
        }
    }

    for (int step = 1; step <= static_cast<int>(order.size()); ++step) {
        const int index = (selected_index + direction * step + static_cast<int>(order.size()) * 8) %
                          static_cast<int>(order.size());
        const UseItemKind candidate = order[static_cast<std::size_t>(index)];
        if (!has_available_item(player, candidate)) {
            continue;
        }

        player->selected_item = candidate;
        return;
    }

    player->selected_item = UseItemKind::None;
}

void select_if_unset(Player* player, UseItemKind item) {
    if (player->selected_item == UseItemKind::None && has_available_item(player, item)) {
        player->selected_item = item;
    }
}

void create_bomb(GameSession* session, const Player* player) {
    make_projectile(session, session->area_kind, session->current_cave_id, ProjectileKind::Bomb,
                    true, player->position + facing_vector(player->facing) * 0.9F, glm::vec2(0.0F),
                    kBombFuseSeconds, kBombRadius, 2);
}

void create_boomerang(GameSession* session, const Player* player) {
    for (const Projectile& projectile : session->projectiles) {
        if (projectile.active && projectile.from_player &&
            projectile.kind == ProjectileKind::Boomerang &&
            in_area(session, projectile.area_kind, projectile.cave_id)) {
            return;
        }
    }

    Projectile& projectile = spawn_projectile(session);
    projectile.kind = ProjectileKind::Boomerang;
    projectile.area_kind = session->area_kind;
    projectile.cave_id = session->current_cave_id;
    projectile.from_player = true;
    projectile.position = player->position + facing_vector(player->facing) * 0.7F;
    projectile.origin = player->position;
    projectile.velocity = facing_vector(player->facing) * kBoomerangSpeed;
    projectile.seconds_remaining = kBoomerangFlightSeconds;
    projectile.radius = kBoomerangRadius;
    projectile.damage = 1;
}

void create_arrow(GameSession* session, const Player* player) {
    make_projectile(session, session->area_kind, session->current_cave_id, ProjectileKind::Arrow,
                    true, player->position + facing_vector(player->facing) * 0.9F,
                    facing_vector(player->facing) * kArrowSpeed, kProjectileLifetimeSeconds,
                    kArrowRadius, 1);
}

void create_sword_beam(GameSession* session, const Enemy& enemy, const glm::vec2& direction) {
    if (glm::length(direction) < 0.001F) {
        return;
    }

    make_projectile(session, enemy.area_kind, enemy.cave_id, ProjectileKind::SwordBeam, false,
                    enemy.position + glm::normalize(direction) * 0.9F,
                    glm::normalize(direction) * kSwordBeamSpeed, kProjectileLifetimeSeconds,
                    kArrowRadius, 1);
}

void create_player_sword_beam(GameSession* session, Player* player) {
    for (const Projectile& projectile : session->projectiles) {
        if (!projectile.active || !projectile.from_player ||
            projectile.kind != ProjectileKind::SwordBeam ||
            !in_area(session, projectile.area_kind, projectile.cave_id)) {
            continue;
        }

        return;
    }

    const glm::vec2 direction = facing_vector(player->facing);
    make_projectile(session, session->area_kind, session->current_cave_id,
                    ProjectileKind::SwordBeam, true, player->position + direction * 0.9F,
                    direction * kSwordBeamSpeed, kProjectileLifetimeSeconds, kArrowRadius, 1);
}

int gather_recorder_dungeons(const World* overworld_world,
                             std::array<OverworldWarp, kScreenCount>* recorder_warps) {
    int count = 0;
    for (int room_id = 0; room_id < kScreenCount; ++room_id) {
        std::array<OverworldWarp, kMaxRoomWarps> room_warps = {};
        const int warp_count =
            gather_overworld_warps(&overworld_world->overworld, room_id, &room_warps);
        for (int index = 0; index < warp_count; ++index) {
            const OverworldWarp& warp = room_warps[static_cast<std::size_t>(index)];
            if (!warp.visible || warp.type != OverworldWarpType::Dungeon) {
                continue;
            }

            bool duplicate = false;
            for (int existing = 0; existing < count; ++existing) {
                if ((*recorder_warps)[static_cast<std::size_t>(existing)].cave_id == warp.cave_id) {
                    duplicate = true;
                    break;
                }
            }

            if (duplicate) {
                continue;
            }

            (*recorder_warps)[static_cast<std::size_t>(count)] = warp;
            ++count;
        }
    }

    std::sort(recorder_warps->begin(), recorder_warps->begin() + count,
              [](const OverworldWarp& a, const OverworldWarp& b) { return a.cave_id < b.cave_id; });
    return count;
}

bool try_trigger_digdogger_split(GameSession* session) {
    for (Enemy& enemy : session->enemies) {
        if (!enemy.active || !in_area(session, enemy.area_kind, enemy.cave_id) ||
            enemy.kind != EnemyKind::Digdogger || enemy.special_counter != 0) {
            continue;
        }

        enemy.active = false;
        for (int index = 0; index < 3; ++index) {
            Enemy child;
            child.active = true;
            child.kind = EnemyKind::Digdogger;
            child.area_kind = enemy.area_kind;
            child.cave_id = enemy.cave_id;
            child.position =
                enemy.position + orbit_offset(static_cast<float>(index) * 2.09439510239F, 1.2F);
            child.spawn_position = child.position;
            child.origin = child.position;
            child.max_health = 2;
            child.health = 2;
            child.special_counter = 1;
            child.respawn_group = -1;
            child.zoo_respawn = false;
            reset_enemy_state(session, &child);
            child.special_counter = 1;
            session->enemies.push_back(child);
        }

        set_message(session, "digdogger split", 1.0F);
        return true;
    }

    return false;
}

void use_recorder(GameSession* session, const World* overworld_world, Player* player) {
    if (try_trigger_digdogger_split(session)) {
        return;
    }

    if (session->area_kind != AreaKind::Overworld) {
        set_message(session, "recorder only works outside", 1.2F);
        return;
    }

    std::array<OverworldWarp, kScreenCount> recorder_warps = {};
    const int warp_count = gather_recorder_dungeons(overworld_world, &recorder_warps);
    if (warp_count <= 0) {
        set_message(session, "no recorder destination", 1.2F);
        return;
    }

    session->recorder_destination_index =
        (session->recorder_destination_index + 1 + warp_count) % warp_count;
    const OverworldWarp& warp =
        recorder_warps[static_cast<std::size_t>(session->recorder_destination_index)];
    player->position = warp.return_position;
    player->move_direction = MoveDirection::None;
    player->facing = Facing::Down;
    session->warp_cooldown_seconds = kAreaTransitionCooldownSeconds;
    update_current_room(session, player);
    set_message(session, "recorder -> dungeon " + std::to_string(warp.cave_id), 1.4F);
}

void create_fire(GameSession* session, const Player* player) {
    make_projectile(session, session->area_kind, session->current_cave_id, ProjectileKind::Fire,
                    true, player->position + facing_vector(player->facing) * 0.8F,
                    facing_vector(player->facing) * kFireSpeed, kFireSeconds, kFireRadius, 1);
}

void create_food(GameSession* session, const Player* player) {
    if (find_active_food(session, session->area_kind, session->current_cave_id) != nullptr) {
        return;
    }

    make_projectile(session, session->area_kind, session->current_cave_id, ProjectileKind::Food,
                    true, player->position + facing_vector(player->facing) * 0.8F, glm::vec2(0.0F),
                    kFoodSeconds, 0.45F, 0);
}

void use_selected_item(GameSession* session, const World* overworld_world, Player* player) {
    switch (player->selected_item) {
    case UseItemKind::Bombs:
        if (player->bombs <= 0) {
            set_message(session, "out of bombs", 1.2F);
            return;
        }
        player->bombs -= 1;
        create_bomb(session, player);
        set_message(session, "bomb placed", 0.8F);
        break;
    case UseItemKind::Boomerang:
        if (!player->has_boomerang) {
            set_message(session, "need boomerang", 1.0F);
            return;
        }
        create_boomerang(session, player);
        set_message(session, "boomerang thrown", 0.8F);
        break;
    case UseItemKind::Bow:
        if (!player->has_bow || player->rupees <= 0) {
            set_message(session, player->has_bow ? "need rupees for arrows" : "need bow", 1.2F);
            return;
        }
        player->rupees -= 1;
        create_arrow(session, player);
        set_message(session, "arrow fired", 0.8F);
        break;
    case UseItemKind::Candle:
        if (!player->has_candle) {
            set_message(session, "need candle", 1.0F);
            return;
        }
        create_fire(session, player);
        set_message(session, "fire lit", 0.8F);
        break;
    case UseItemKind::Recorder:
        if (!player->has_recorder) {
            set_message(session, "need recorder", 1.0F);
            return;
        }
        use_recorder(session, overworld_world, player);
        break;
    case UseItemKind::Food:
        if (!player->has_food) {
            set_message(session, "need bait", 1.0F);
            return;
        }
        player->has_food = false;
        create_food(session, player);
        if (player->selected_item == UseItemKind::Food) {
            select_next_item(player, 1);
        }
        set_message(session, "bait placed", 1.0F);
        break;
    case UseItemKind::Potion:
        if (!player->has_potion) {
            set_message(session, "need potion", 1.0F);
            return;
        }
        player->has_potion = false;
        player->health = player->max_health;
        if (player->selected_item == UseItemKind::Potion) {
            select_next_item(player, 1);
        }
        set_message(session, "potion used", 1.0F);
        break;
    case UseItemKind::None:
        break;
    }
}

void ensure_sword_cave_pickup(GameSession* session) {
    if (session->sword_cave_reward_taken) {
        return;
    }

    for (const Pickup& pickup : session->pickups) {
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
    session->pickups.push_back(pickup);
}

void init_opening_overworld_enemies(GameSession* session) {
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
            reset_enemy_state(session, &enemy);
            session->enemies.push_back(enemy);
        }
    }
}

void update_current_room(GameSession* session, const Player* player) {
    if (session->area_kind != AreaKind::Overworld) {
        session->current_room_id = -1;
        session->previous_room_id = -1;
        return;
    }

    session->current_room_id = get_room_from_position(player->position);
    if (session->current_room_id != session->previous_room_id) {
        session->previous_room_id = session->current_room_id;
    }
}

void spawn_pickup_drop(GameSession* session, const Enemy& enemy) {
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
        session->pickups.push_back(pickup);
        return;
    }

    const int roll = random_int(session, 7);
    if (roll <= 1) {
        pickup.kind = PickupKind::Heart;
    } else if (roll <= 4) {
        pickup.kind = PickupKind::Rupee;
    } else {
        pickup.kind = PickupKind::Bombs;
    }

    session->pickups.push_back(pickup);
}

void damage_player_from(GameSession* session, const World* world, Player* player, int damage,
                        const glm::vec2& source_position) {
    if (session->area_kind == AreaKind::EnemyZoo || session->area_kind == AreaKind::ItemZoo) {
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
    session->area_kind = AreaKind::Overworld;
    session->current_cave_id = -1;
    session->cave_return_room_id = -1;
    session->cave_return_position = glm::vec2(0.0F);
    session->warp_cooldown_seconds = kAreaTransitionCooldownSeconds;
    update_current_room(session, player);
}

void damage_enemy(GameSession* session, Enemy* enemy, int damage) {
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
            reset_enemy_state(session, &head);
            session->enemies.push_back(head);
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
            reset_enemy_state(session, &child);
            session->enemies.push_back(child);
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
            reset_enemy_state(session, &child);
            session->enemies.push_back(child);
        }
    }

    if (enemy->zoo_respawn && enemy->area_kind == AreaKind::EnemyZoo) {
        enemy->active = false;
        enemy->respawn_seconds_remaining = 2.0F;
        return;
    }

    enemy->active = false;
    spawn_pickup_drop(session, *enemy);
}

void process_player_attacks(GameSession* session, Player* player) {
    if (!player->has_sword || !is_sword_active(player)) {
        return;
    }

    const glm::vec2 sword_pos = sword_world_position(player);
    for (Enemy& enemy : session->enemies) {
        if (!enemy_in_current_area(session, enemy)) {
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
                    damage_enemy(session, &enemy, enemy.health);
                } else {
                    set_message(session, "manhandla petal down", 0.6F);
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
                damage_enemy(session, &enemy, 1);
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
                set_message(session, "patra orbiter down", 0.6F);
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
                damage_enemy(session, &enemy, 1);
                if (enemy.active) {
                    enemy.hidden = false;
                    enemy.special_counter = 1;
                    enemy.state_seconds_remaining = 1.0F;
                }
            }
            set_message(session, enemy.health <= 1 ? "ganon is weak" : "ganon revealed", 0.6F);
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

        damage_enemy(session, &enemy, 1);
    }
}

void bounce_velocity(const World* world, glm::vec2* position, glm::vec2* velocity, float dt) {
    const glm::vec2 candidate_x = *position + glm::vec2(velocity->x * dt, 0.0F);
    if (world_is_walkable_tile(world, candidate_x)) {
        position->x = candidate_x.x;
    } else {
        velocity->x *= -1.0F;
    }

    const glm::vec2 candidate_y = *position + glm::vec2(0.0F, velocity->y * dt);
    if (world_is_walkable_tile(world, candidate_y)) {
        position->y = candidate_y.y;
    } else {
        velocity->y *= -1.0F;
    }
}

void tick_octorok_like(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds, float speed, float shoot_period_min,
                       float shoot_period_random, ProjectileKind shot_kind) {
    enemy->move_seconds_remaining -= dt_seconds;
    enemy->action_seconds_remaining -= dt_seconds;

    if (enemy->move_seconds_remaining <= 0.0F) {
        choose_cardinal_direction(session, world, enemy);
        const Projectile* food = find_active_food(session, enemy->area_kind, enemy->cave_id);
        if (food != nullptr) {
            const Player bait_player = Player{.position = food->position};
            choose_player_axis_direction(*enemy, bait_player, &enemy->facing);
        }
    }

    const glm::vec2 candidate = enemy->position + facing_vector(enemy->facing) * speed * dt_seconds;
    if (world_is_walkable_tile(world, candidate)) {
        enemy->position = candidate;
    } else {
        enemy->move_seconds_remaining = 0.0F;
    }

    if (enemy->action_seconds_remaining > 0.0F) {
        return;
    }

    if (player != nullptr) {
        Facing shot_facing = enemy->facing;
        if (choose_cardinal_shot_direction(session, *enemy, *player, &shot_facing)) {
            enemy->facing = shot_facing;
            if (shot_kind == ProjectileKind::SwordBeam) {
                create_sword_beam(session, *enemy, facing_vector(enemy->facing));
            } else if (shot_kind == ProjectileKind::Arrow) {
                make_projectile(session, enemy->area_kind, enemy->cave_id, ProjectileKind::Arrow,
                                false, enemy->position + facing_vector(enemy->facing) * 0.9F,
                                facing_vector(enemy->facing) * kArrowSpeed,
                                kProjectileLifetimeSeconds, kArrowRadius, 1);
            } else {
                const glm::vec2 velocity = facing_vector(enemy->facing) * kRockSpeed;
                throw_enemy_rock(session, *enemy, velocity);
            }
        }
    } else {
        enemy->action_seconds_remaining =
            shoot_period_min + random_unit(session) * shoot_period_random;
        return;
    }

    enemy->action_seconds_remaining = shoot_period_min + random_unit(session) * shoot_period_random;
}

void tick_basic_walker(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds, float speed, bool bias_toward_player) {
    enemy->move_seconds_remaining -= dt_seconds;
    enemy->action_seconds_remaining -= dt_seconds;

    if (bias_toward_player && player != nullptr && enemy->action_seconds_remaining <= 0.0F) {
        const glm::vec2 delta = player->position - enemy->position;
        if (std::abs(delta.x) > std::abs(delta.y)) {
            enemy->facing = delta.x < 0.0F ? Facing::Left : Facing::Right;
        } else {
            enemy->facing = delta.y < 0.0F ? Facing::Up : Facing::Down;
        }
        enemy->action_seconds_remaining = 0.35F + random_unit(session) * 0.40F;
        enemy->move_seconds_remaining = 0.30F + random_unit(session) * 0.50F;
    } else if (enemy->move_seconds_remaining <= 0.0F) {
        choose_cardinal_direction(session, world, enemy);
    }

    const glm::vec2 candidate = enemy->position + facing_vector(enemy->facing) * speed * dt_seconds;
    if (world_is_walkable_tile(world, candidate)) {
        enemy->position = candidate;
    } else {
        enemy->move_seconds_remaining = 0.0F;
    }
}

void tick_goriya(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                 float dt_seconds) {
    const Projectile* food = find_active_food(session, enemy->area_kind, enemy->cave_id);
    const Player* move_target = player;
    Player bait_player = {};
    if (food != nullptr) {
        bait_player.position = food->position;
        move_target = &bait_player;
    }

    tick_basic_walker(session, world, enemy, move_target, dt_seconds, kGoriyaSpeed, true);
    if (enemy->action_seconds_remaining > 0.0F || player == nullptr) {
        return;
    }

    Facing shot_facing = enemy->facing;
    if (!choose_cardinal_shot_direction(session, *enemy, *player, &shot_facing)) {
        enemy->action_seconds_remaining = 0.5F;
        return;
    }

    enemy->facing = shot_facing;
    make_projectile(session, enemy->area_kind, enemy->cave_id, ProjectileKind::Boomerang, false,
                    enemy->position + facing_vector(enemy->facing) * 0.8F,
                    facing_vector(enemy->facing) * kBoomerangSpeed, 0.85F, kBoomerangRadius, 1);
    enemy->action_seconds_remaining = 1.1F + random_unit(session) * 0.5F;
}

void tick_rope(GameSession* session, const World* world, Enemy* enemy, const Player* player,
               float dt_seconds) {
    enemy->state_seconds_remaining -= dt_seconds;
    if (enemy->state_seconds_remaining > 0.0F) {
        bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
        return;
    }

    if (player != nullptr) {
        const glm::vec2 delta = player->position - enemy->position;
        if (std::abs(delta.x) <= kShootAlignThreshold ||
            std::abs(delta.y) <= kShootAlignThreshold) {
            enemy->velocity = glm::normalize(glm::vec2(delta.x, delta.y)) * kRopeChargeSpeed;
            if (std::abs(delta.x) > std::abs(delta.y)) {
                enemy->velocity.y = 0.0F;
            } else {
                enemy->velocity.x = 0.0F;
            }
            enemy->state_seconds_remaining = 0.55F;
            return;
        }
    }

    tick_basic_walker(session, world, enemy, player, dt_seconds, kRopeSpeed, false);
}

void tick_ghini(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds) {
    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining <= 0.0F) {
        glm::vec2 direction(random_unit(session) * 2.0F - 1.0F, random_unit(session) * 2.0F - 1.0F);
        if (player != nullptr) {
            direction = player->position - enemy->position;
        }
        if (glm::length(direction) < 0.1F) {
            direction = glm::vec2(0.0F, 1.0F);
        } else {
            direction = glm::normalize(direction);
        }
        enemy->velocity = direction * kGhiniSpeed;
        enemy->action_seconds_remaining = 0.25F + random_unit(session) * 0.35F;
    }

    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
}

void tick_flying_ghini(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds) {
    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining <= 0.0F) {
        glm::vec2 direction(random_unit(session) * 2.0F - 1.0F, random_unit(session) * 2.0F - 1.0F);
        if (player != nullptr) {
            direction = player->position - enemy->position;
        }
        if (glm::length(direction) < 0.1F) {
            direction = glm::vec2(0.0F, 1.0F);
        } else {
            direction = glm::normalize(direction);
        }
        enemy->velocity = direction * kFlyingGhiniSpeed;
        enemy->action_seconds_remaining = 0.15F + random_unit(session) * 0.20F;
    }

    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
}

void tick_vire(GameSession* session, const World* world, Enemy* enemy, const Player* player,
               float dt_seconds) {
    enemy->move_seconds_remaining -= dt_seconds;
    if (enemy->move_seconds_remaining <= 0.0F) {
        choose_cardinal_direction(session, world, enemy);
    }

    glm::vec2 candidate = enemy->position + facing_vector(enemy->facing) * kVireSpeed * dt_seconds;
    candidate.y += std::sin(enemy->action_seconds_remaining * 14.0F) * 0.04F;
    if (world_is_walkable_tile(world, candidate)) {
        enemy->position = candidate;
    } else {
        enemy->move_seconds_remaining = 0.0F;
    }
    enemy->action_seconds_remaining += dt_seconds;

    const Projectile* food = find_active_food(session, enemy->area_kind, enemy->cave_id);
    if (food != nullptr && random_unit(session) < dt_seconds * 1.2F) {
        const Player bait_player = Player{.position = food->position};
        choose_player_axis_direction(*enemy, bait_player, &enemy->facing);
    } else if (player != nullptr && random_unit(session) < dt_seconds * 0.6F) {
        choose_player_axis_direction(*enemy, *player, &enemy->facing);
    }
}

void tick_blue_wizzrobe(GameSession* session, const World* world, Enemy* enemy,
                        const Player* player, float dt_seconds) {
    if (enemy->hidden) {
        enemy->state_seconds_remaining -= dt_seconds;
        if (enemy->state_seconds_remaining > 0.0F) {
            return;
        }

        enemy->hidden = false;
        enemy->action_seconds_remaining = 1.0F + random_unit(session) * 0.5F;
        enemy->move_seconds_remaining = 0.3F;
        return;
    }

    enemy->action_seconds_remaining -= dt_seconds;
    enemy->move_seconds_remaining -= dt_seconds;
    if (enemy->move_seconds_remaining <= 0.0F) {
        if (player != nullptr) {
            choose_player_axis_direction(*enemy, *player, &enemy->facing);
        }
        enemy->move_seconds_remaining = 0.3F;
    }

    const glm::vec2 candidate =
        enemy->position + facing_vector(enemy->facing) * kWizzrobeSpeed * dt_seconds;
    if (world_is_walkable_tile(world, candidate)) {
        enemy->position = candidate;
    }

    if (enemy->action_seconds_remaining <= 0.0F) {
        if (player != nullptr &&
            choose_cardinal_shot_direction(session, *enemy, *player, &enemy->facing)) {
            make_projectile(session, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                            enemy->position + facing_vector(enemy->facing) * 0.7F,
                            facing_vector(enemy->facing) * kFireSpeed, kProjectileLifetimeSeconds,
                            kFireRadius, 1);
        }

        enemy->hidden = true;
        enemy->state_seconds_remaining = kWizzrobeTeleportSeconds;
        const std::array<glm::vec2, 4> offsets = {
            glm::vec2(-4.0F, 0.0F),
            glm::vec2(4.0F, 0.0F),
            glm::vec2(0.0F, -4.0F),
            glm::vec2(0.0F, 4.0F),
        };
        for (int attempt = 0; attempt < 4; ++attempt) {
            const glm::vec2 target =
                enemy->position +
                offsets[static_cast<std::size_t>((attempt + random_int(session, 4)) % 4)];
            if (!world_is_walkable_tile(world, target)) {
                continue;
            }
            enemy->position = target;
            break;
        }
    }
}

void tick_red_wizzrobe(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                       float dt_seconds) {
    enemy->state_seconds_remaining -= dt_seconds;
    if (enemy->hidden) {
        if (enemy->state_seconds_remaining > 0.0F || player == nullptr) {
            return;
        }

        enemy->hidden = false;
        enemy->action_seconds_remaining = 1.0F;
        choose_player_axis_direction(*enemy, *player, &enemy->facing);
        const std::array<glm::vec2, 8> offsets = {
            glm::vec2(-4.0F, 0.0F), glm::vec2(4.0F, 0.0F),   glm::vec2(0.0F, -4.0F),
            glm::vec2(0.0F, 4.0F),  glm::vec2(-3.0F, -3.0F), glm::vec2(3.0F, -3.0F),
            glm::vec2(-3.0F, 3.0F), glm::vec2(3.0F, 3.0F),
        };
        for (int attempt = 0; attempt < 8; ++attempt) {
            const glm::vec2 target =
                player->position +
                offsets[static_cast<std::size_t>((attempt + random_int(session, 8)) % 8)];
            if (!world_is_walkable_tile(world, target)) {
                continue;
            }
            enemy->position = target;
            break;
        }
        return;
    }

    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining <= 0.45F && enemy->action_seconds_remaining > 0.35F &&
        player != nullptr) {
        choose_cardinal_shot_direction(session, *enemy, *player, &enemy->facing);
        make_projectile(session, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                        enemy->position + facing_vector(enemy->facing) * 0.8F,
                        facing_vector(enemy->facing) * kFireSpeed, kProjectileLifetimeSeconds,
                        kFireRadius, 1);
    }

    if (enemy->action_seconds_remaining > 0.0F) {
        return;
    }

    enemy->hidden = true;
    enemy->state_seconds_remaining = 1.0F + random_unit(session) * 0.8F;
}

void tick_trap(const World* world, Enemy* enemy, const Player* player, float dt_seconds) {
    if (glm::length(enemy->velocity) > 0.0F) {
        const glm::vec2 candidate = enemy->position + enemy->velocity * dt_seconds;
        if (!world_is_walkable_tile(world, candidate)) {
            enemy->velocity *= -1.0F;
        } else {
            enemy->position = candidate;
        }

        if (glm::length(enemy->position - enemy->origin) < 0.4F &&
            glm::dot(enemy->velocity, enemy->origin - enemy->position) <= 0.0F) {
            enemy->position = enemy->origin;
            enemy->velocity = glm::vec2(0.0F);
        }
        return;
    }

    if (player == nullptr) {
        return;
    }

    const glm::vec2 delta = player->position - enemy->position;
    if (std::abs(delta.x) <= kShootAlignThreshold) {
        enemy->velocity = glm::vec2(0.0F, delta.y < 0.0F ? -kTrapSpeed : kTrapSpeed);
    } else if (std::abs(delta.y) <= kShootAlignThreshold) {
        enemy->velocity = glm::vec2(delta.x < 0.0F ? -kTrapSpeed : kTrapSpeed, 0.0F);
    }
}

void tick_armos(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds) {
    if (enemy->special_counter == 0) {
        if (player != nullptr && glm::length(player->position - enemy->position) <= 2.5F) {
            enemy->special_counter = 1;
            enemy->move_seconds_remaining = 0.0F;
        } else {
            return;
        }
    }

    tick_basic_walker(session, world, enemy, player, dt_seconds, kArmosSpeed, true);
}

void tick_tektite(GameSession* session, const World* world, Enemy* enemy, float dt_seconds) {
    enemy->action_seconds_remaining -= dt_seconds;
    enemy->move_seconds_remaining -= dt_seconds;

    if (enemy->move_seconds_remaining <= 0.0F && enemy->action_seconds_remaining <= 0.0F) {
        const glm::vec2 direction = glm::normalize(
            glm::vec2(random_unit(session) * 2.0F - 1.0F, random_unit(session) * 2.0F - 1.0F));
        enemy->velocity = direction * kTektiteHopSpeed;
        enemy->move_seconds_remaining = 0.28F + random_unit(session) * 0.10F;
        enemy->action_seconds_remaining = 0.45F + random_unit(session) * 0.35F;
    }

    if (enemy->move_seconds_remaining > 0.0F) {
        bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
    }
}

void tick_leever(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                 float dt_seconds) {
    enemy->state_seconds_remaining -= dt_seconds;
    if (enemy->hidden) {
        if (enemy->state_seconds_remaining <= 0.0F) {
            enemy->hidden = false;
            enemy->state_seconds_remaining = 1.4F + random_unit(session) * 0.8F;
        }
        return;
    }

    if (enemy->state_seconds_remaining <= 0.0F) {
        enemy->hidden = true;
        enemy->state_seconds_remaining = 0.7F + random_unit(session) * 0.5F;
        return;
    }

    glm::vec2 toward_player(random_unit(session) * 2.0F - 1.0F, random_unit(session) * 2.0F - 1.0F);
    if (player != nullptr) {
        toward_player = player->position - enemy->position;
    }
    if (glm::length(toward_player) > 0.001F) {
        toward_player = glm::normalize(toward_player);
    } else {
        toward_player = glm::vec2(0.0F, 1.0F);
    }

    const glm::vec2 candidate = enemy->position + toward_player * kLeeverSpeed * dt_seconds;
    if (world_is_walkable_tile(world, candidate)) {
        enemy->position = candidate;
    }
}

void tick_keese(GameSession* session, const World* world, Enemy* enemy, float dt_seconds) {
    enemy->action_seconds_remaining -= dt_seconds;
    enemy->move_seconds_remaining -= dt_seconds;

    if (enemy->action_seconds_remaining <= 0.0F) {
        glm::vec2 direction(random_unit(session) * 2.0F - 1.0F, random_unit(session) * 2.0F - 1.0F);
        const Projectile* food = find_active_food(session, enemy->area_kind, enemy->cave_id);
        if (food != nullptr) {
            direction = food->position - enemy->position;
        }
        if (glm::length(direction) < 0.1F) {
            direction = glm::vec2(0.0F, 1.0F);
        } else {
            direction = glm::normalize(direction);
        }

        enemy->velocity = direction * kKeeseSpeed;
        enemy->action_seconds_remaining = 0.35F + random_unit(session) * 0.35F;
        enemy->move_seconds_remaining = 0.8F + random_unit(session) * 1.0F;
    }

    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
}

void tick_pols_voice(GameSession* session, const World* world, Enemy* enemy, float dt_seconds) {
    enemy->action_seconds_remaining -= dt_seconds;
    enemy->move_seconds_remaining -= dt_seconds;

    if (enemy->move_seconds_remaining <= 0.0F && enemy->action_seconds_remaining <= 0.0F) {
        const glm::vec2 direction = glm::normalize(
            glm::vec2(random_unit(session) * 2.0F - 1.0F, random_unit(session) * 2.0F - 1.0F));
        enemy->velocity = direction * kPolsVoiceSpeed;
        enemy->move_seconds_remaining = 0.20F + random_unit(session) * 0.10F;
        enemy->action_seconds_remaining = 0.38F + random_unit(session) * 0.25F;
    }

    if (enemy->move_seconds_remaining > 0.0F) {
        bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
    }
}

void tick_wallmaster(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                     float dt_seconds) {
    if (player == nullptr) {
        return;
    }

    if (enemy->hidden) {
        enemy->action_seconds_remaining -= dt_seconds;
        if (enemy->action_seconds_remaining > 0.0F) {
            return;
        }

        const float left = player->position.x;
        const float right = static_cast<float>(world_width(world)) - player->position.x;
        const float top = player->position.y;
        const float bottom = static_cast<float>(world_height(world)) - player->position.y;

        if (left <= right && left <= top && left <= bottom) {
            enemy->position = glm::vec2(1.5F, player->position.y);
            enemy->velocity = glm::vec2(kWallmasterSpeed, 0.0F);
        } else if (right <= top && right <= bottom) {
            enemy->position =
                glm::vec2(static_cast<float>(world_width(world)) - 1.5F, player->position.y);
            enemy->velocity = glm::vec2(-kWallmasterSpeed, 0.0F);
        } else if (top <= bottom) {
            enemy->position = glm::vec2(player->position.x, 1.5F);
            enemy->velocity = glm::vec2(0.0F, kWallmasterSpeed);
        } else {
            enemy->position =
                glm::vec2(player->position.x, static_cast<float>(world_height(world)) - 1.5F);
            enemy->velocity = glm::vec2(0.0F, -kWallmasterSpeed);
        }

        enemy->origin = enemy->position;
        enemy->hidden = false;
        enemy->move_seconds_remaining = 1.8F;
        return;
    }

    enemy->move_seconds_remaining -= dt_seconds;
    const glm::vec2 candidate = enemy->position + enemy->velocity * dt_seconds;
    if (world_is_walkable_tile(world, candidate)) {
        enemy->position = candidate;
    }

    if (enemy->move_seconds_remaining > 0.0F) {
        return;
    }

    enemy->hidden = true;
    enemy->velocity = glm::vec2(0.0F);
    enemy->action_seconds_remaining = 0.8F + random_unit(session) * 0.8F;
}

void tick_dodongo(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                  float dt_seconds) {
    enemy->state_seconds_remaining -= dt_seconds;
    if (enemy->state_seconds_remaining > 0.0F) {
        return;
    }

    if (enemy->special_counter == 1) {
        enemy->special_counter = 0;
    }

    enemy->move_seconds_remaining -= dt_seconds;
    if (enemy->move_seconds_remaining <= 0.0F) {
        if (player != nullptr && random_unit(session) < 0.55F) {
            choose_player_axis_direction(*enemy, *player, &enemy->facing);
        } else {
            choose_cardinal_direction(session, world, enemy);
        }
        enemy->move_seconds_remaining = 0.8F + random_unit(session) * 0.7F;
    }

    const glm::vec2 candidate =
        enemy->position + facing_vector(enemy->facing) * kDodongoSpeed * dt_seconds;
    if (world_is_walkable_tile(world, candidate)) {
        enemy->position = candidate;
    } else {
        enemy->move_seconds_remaining = 0.0F;
    }
}

void tick_gohma(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds) {
    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);

    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining <= 0.0F) {
        enemy->velocity.x *= -1.0F;
        enemy->action_seconds_remaining = 0.7F + random_unit(session) * 0.6F;
    }

    enemy->state_seconds_remaining -= dt_seconds;
    if (enemy->state_seconds_remaining > 0.0F) {
        return;
    }

    if (enemy->special_counter == 0) {
        enemy->special_counter = 1;
        enemy->state_seconds_remaining = kGohmaEyeOpenSeconds;
        if (player != nullptr) {
            Facing shot_facing = enemy->facing;
            choose_player_axis_direction(*enemy, *player, &shot_facing);
            make_projectile(session, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                            enemy->position + facing_vector(shot_facing) * 0.8F,
                            facing_vector(shot_facing) * kFireSpeed, kProjectileLifetimeSeconds,
                            kFireRadius, 1);
        }
        return;
    }

    enemy->special_counter = 0;
    enemy->state_seconds_remaining = kGohmaEyeClosedSeconds + random_unit(session) * 0.6F;
}

void tick_moldorm(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                  float dt_seconds) {
    if (enemy->subtype > 0) {
        Enemy* leader = nullptr;
        for (Enemy& other : session->enemies) {
            if (!other.active || other.kind != EnemyKind::Moldorm ||
                other.area_kind != enemy->area_kind || other.cave_id != enemy->cave_id ||
                other.respawn_group != enemy->respawn_group ||
                other.subtype != enemy->subtype - 1) {
                continue;
            }
            leader = &other;
            break;
        }

        if (leader == nullptr) {
            return;
        }

        const glm::vec2 toward_leader = leader->position - enemy->position;
        const float distance = glm::length(toward_leader);
        if (distance > 0.72F) {
            const glm::vec2 direction = toward_leader / distance;
            enemy->position += direction * glm::min(distance - 0.72F, kMoldormSpeed * dt_seconds);
            enemy->facing = direction.x < 0.0F ? Facing::Left : Facing::Right;
        }
        return;
    }

    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining <= 0.0F) {
        glm::vec2 direction(random_unit(session) * 2.0F - 1.0F, random_unit(session) * 2.0F - 1.0F);
        if (player != nullptr && random_unit(session) < 0.5F) {
            direction = player->position - enemy->position;
        }
        if (glm::length(direction) < 0.1F) {
            direction = glm::vec2(1.0F, 0.0F);
        } else {
            direction = glm::normalize(direction);
        }
        enemy->velocity = direction * kMoldormSpeed;
        enemy->action_seconds_remaining = 0.25F + random_unit(session) * 0.35F;
    }

    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
}

void tick_digdogger(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds) {
    const float speed = enemy->special_counter == 0 ? kDigdoggerSpeed : kDigdoggerSpeed * 1.35F;
    if (enemy->special_counter == 0) {
        enemy->invulnerable = true;
    }

    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining <= 0.0F) {
        glm::vec2 direction(random_unit(session) * 2.0F - 1.0F, random_unit(session) * 2.0F - 1.0F);
        if (player != nullptr && random_unit(session) < 0.6F) {
            direction = player->position - enemy->position;
        }
        if (glm::length(direction) < 0.1F) {
            direction = glm::vec2(1.0F, 0.0F);
        } else {
            direction = glm::normalize(direction);
        }
        enemy->velocity = direction * speed;
        enemy->action_seconds_remaining = 0.35F + random_unit(session) * 0.25F;
    }

    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
}

void tick_manhandla(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                    float dt_seconds) {
    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining <= 0.0F) {
        if (player != nullptr) {
            const glm::vec2 toward_player = player->position - enemy->position;
            if (glm::length(toward_player) > 0.1F) {
                enemy->velocity = glm::normalize(toward_player) *
                                  (kManhandlaSpeed + static_cast<float>(6 - enemy->health) * 0.4F);
            }
        } else {
            enemy->velocity.x *= -1.0F;
        }

        const std::array<glm::vec2, 4> shots = {glm::vec2(1.0F, 0.0F), glm::vec2(-1.0F, 0.0F),
                                                glm::vec2(0.0F, 1.0F), glm::vec2(0.0F, -1.0F)};
        for (const glm::vec2& shot : shots) {
            make_projectile(session, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                            enemy->position + shot * 0.9F, shot * kFireSpeed,
                            kProjectileLifetimeSeconds, kFireRadius, 1);
        }
        enemy->action_seconds_remaining = 1.6F;
    }
}

void tick_gleeok(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                 float dt_seconds) {
    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining > 0.0F) {
        return;
    }

    const int head_count = glm::max(enemy->special_counter, 1);
    for (int head = 0; head < head_count; ++head) {
        const float head_offset =
            (static_cast<float>(head) - static_cast<float>(head_count / 2)) * 0.8F;
        glm::vec2 direction = glm::vec2(0.0F, 1.0F);
        if (player != nullptr) {
            direction = player->position - (enemy->position + glm::vec2(head_offset, -0.6F));
        }
        if (glm::length(direction) < 0.1F) {
            direction = glm::vec2(0.0F, 1.0F);
        } else {
            direction = glm::normalize(direction);
        }

        make_projectile(session, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                        enemy->position + glm::vec2(head_offset, -0.6F), direction * kFireSpeed,
                        kProjectileLifetimeSeconds, kFireRadius, 1);
    }
    enemy->action_seconds_remaining = 1.2F;
}

void tick_patra(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds) {
    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
    enemy->state_seconds_remaining -= dt_seconds;
    if (enemy->state_seconds_remaining <= 0.0F) {
        enemy->state_seconds_remaining = 4.0F;
        enemy->velocity *= -1.0F;
    }

    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining <= 0.0F && player != nullptr) {
        glm::vec2 direction = player->position - enemy->position;
        if (glm::length(direction) > 0.1F) {
            direction = glm::normalize(direction);
            make_projectile(session, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                            enemy->position + direction * 0.8F, direction * kFireSpeed,
                            kProjectileLifetimeSeconds, kFireRadius, 1);
        }
        enemy->action_seconds_remaining = 0.9F;
    }
}

void tick_ganon(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                float dt_seconds) {
    if (enemy->special_counter > 0) {
        enemy->state_seconds_remaining -= dt_seconds;
        if (enemy->state_seconds_remaining <= 0.0F) {
            enemy->special_counter = 0;
            enemy->hidden = true;
        } else {
            enemy->hidden = false;
        }
        return;
    }

    tick_blue_wizzrobe(session, world, enemy, player, dt_seconds);
    enemy->hidden = true;
}

void tick_zora(GameSession* session, const World* world, Enemy* enemy, const Player* player,
               float dt_seconds) {
    enemy->state_seconds_remaining -= dt_seconds;
    enemy->action_seconds_remaining -= dt_seconds;

    if (enemy->hidden) {
        if (enemy->state_seconds_remaining > 0.0F) {
            return;
        }

        enemy->hidden = false;
        enemy->state_seconds_remaining = 1.6F + random_unit(session) * 0.7F;
        enemy->action_seconds_remaining = 0.45F;
        return;
    }

    if (enemy->action_seconds_remaining <= 0.0F && player != nullptr) {
        glm::vec2 toward_player = player->position - enemy->position;
        if (glm::length(toward_player) > 0.001F) {
            toward_player = glm::normalize(toward_player);
            make_projectile(session, enemy->area_kind, enemy->cave_id, ProjectileKind::Fire, false,
                            enemy->position + toward_player * 0.8F, toward_player * kFireSpeed,
                            kProjectileLifetimeSeconds, kFireRadius, 1);
        }
        enemy->action_seconds_remaining = 99.0F;
    }

    if (enemy->state_seconds_remaining > 0.0F) {
        return;
    }

    enemy->hidden = true;
    enemy->state_seconds_remaining = 1.2F + random_unit(session) * 0.8F;

    const glm::ivec2 base(static_cast<int>(enemy->position.x), static_cast<int>(enemy->position.y));
    for (int radius = 0; radius <= 6; ++radius) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                const int tile_x = base.x + dx;
                const int tile_y = base.y + dy;
                if (world_tile_at(world, tile_x, tile_y) != TileKind::Water) {
                    continue;
                }

                enemy->position =
                    glm::vec2(static_cast<float>(tile_x) + 0.5F, static_cast<float>(tile_y) + 0.5F);
                return;
            }
        }
    }
}

void tick_peahat(GameSession* session, const World* world, Enemy* enemy, float dt_seconds) {
    enemy->state_seconds_remaining -= dt_seconds;
    enemy->action_seconds_remaining -= dt_seconds;
    enemy->move_seconds_remaining -= dt_seconds;

    if (enemy->invulnerable) {
        if (enemy->action_seconds_remaining <= 0.0F) {
            glm::vec2 direction(random_unit(session) * 2.0F - 1.0F,
                                random_unit(session) * 2.0F - 1.0F);
            if (glm::length(direction) < 0.1F) {
                direction = glm::vec2(0.0F, -1.0F);
            } else {
                direction = glm::normalize(direction);
            }

            enemy->velocity = direction * kPeahatSpeed;
            enemy->action_seconds_remaining = 0.22F + random_unit(session) * 0.28F;
        }

        bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
        if (enemy->state_seconds_remaining > 0.0F) {
            return;
        }

        enemy->invulnerable = false;
        enemy->velocity = glm::vec2(0.0F);
        enemy->state_seconds_remaining = 0.9F + random_unit(session) * 0.5F;
        return;
    }

    if (enemy->state_seconds_remaining > 0.0F) {
        return;
    }

    enemy->invulnerable = true;
    enemy->state_seconds_remaining = 1.7F + random_unit(session) * 0.8F;
    enemy->action_seconds_remaining = 0.0F;
}

void tick_aquamentus(GameSession* session, const World* world, Enemy* enemy, const Player* player,
                     float dt_seconds) {
    bounce_velocity(world, &enemy->position, &enemy->velocity, dt_seconds);
    enemy->action_seconds_remaining -= dt_seconds;
    if (enemy->action_seconds_remaining > 0.0F) {
        return;
    }

    glm::vec2 toward_player = glm::vec2(0.0F, 1.0F);
    if (player != nullptr) {
        toward_player = player->position - enemy->position;
    }
    if (glm::length(toward_player) < 0.001F) {
        toward_player = glm::vec2(0.0F, 1.0F);
    }

    throw_spread_rocks(session, *enemy, toward_player);
    enemy->action_seconds_remaining = 1.1F + random_unit(session) * 0.4F;
}

void tick_enemies(GameSession* session, const World* overworld_world, Player* player,
                  float dt_seconds) {
    for (Enemy& enemy : session->enemies) {
        if (!enemy.active) {
            continue;
        }

        const World* world =
            get_world_for_area(session, overworld_world, enemy.area_kind, enemy.cave_id);
        const Player* target_player =
            in_area(session, enemy.area_kind, enemy.cave_id) ? player : nullptr;
        enemy.room_id =
            enemy.area_kind == AreaKind::Overworld ? get_room_from_position(enemy.position) : -1;
        enemy.hurt_seconds_remaining = glm::max(0.0F, enemy.hurt_seconds_remaining - dt_seconds);
        if (enemy.kind == EnemyKind::LikeLike && enemy.special_counter > 0) {
            enemy.special_counter -= 1;
        }

        switch (enemy.kind) {
        case EnemyKind::Octorok:
            tick_octorok_like(session, world, &enemy, target_player, dt_seconds, kOctorokSpeed,
                              1.1F, 0.9F, ProjectileKind::Rock);
            break;
        case EnemyKind::Moblin:
            tick_octorok_like(session, world, &enemy, target_player, dt_seconds, kMoblinSpeed, 0.8F,
                              0.7F, ProjectileKind::Arrow);
            break;
        case EnemyKind::Lynel:
            tick_octorok_like(session, world, &enemy, target_player, dt_seconds, kLynelSpeed, 0.65F,
                              0.45F, ProjectileKind::SwordBeam);
            break;
        case EnemyKind::Goriya:
            tick_goriya(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Darknut:
            tick_basic_walker(session, world, &enemy, target_player, dt_seconds, kDarknutSpeed,
                              true);
            break;
        case EnemyKind::Tektite:
            tick_tektite(session, world, &enemy, dt_seconds);
            break;
        case EnemyKind::Leever:
            tick_leever(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Keese:
            tick_keese(session, world, &enemy, dt_seconds);
            break;
        case EnemyKind::Zol:
            tick_basic_walker(session, world, &enemy, target_player, dt_seconds, kZolSpeed, true);
            break;
        case EnemyKind::Gel:
            tick_basic_walker(session, world, &enemy, target_player, dt_seconds, kGelSpeed, false);
            break;
        case EnemyKind::Rope:
            tick_rope(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Vire:
            tick_vire(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Stalfos:
        case EnemyKind::Gibdo:
            tick_basic_walker(session, world, &enemy, target_player, dt_seconds, kWalkerSpeed,
                              false);
            break;
        case EnemyKind::LikeLike:
            tick_basic_walker(session, world, &enemy, target_player, dt_seconds, kLikeLikeSpeed,
                              true);
            break;
        case EnemyKind::PolsVoice:
            tick_pols_voice(session, world, &enemy, dt_seconds);
            break;
        case EnemyKind::BlueWizzrobe:
            tick_blue_wizzrobe(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::RedWizzrobe:
            tick_red_wizzrobe(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Wallmaster:
            tick_wallmaster(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Ghini:
        case EnemyKind::Bubble:
            tick_ghini(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::FlyingGhini:
            tick_flying_ghini(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Trap:
            tick_trap(world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Armos:
            tick_armos(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Zora:
            tick_zora(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Peahat:
            tick_peahat(session, world, &enemy, dt_seconds);
            break;
        case EnemyKind::Dodongo:
            tick_dodongo(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Digdogger:
            tick_digdogger(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Manhandla:
            tick_manhandla(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Gohma:
            tick_gohma(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Moldorm:
            tick_moldorm(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Aquamentus:
            tick_aquamentus(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Gleeok:
            tick_gleeok(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Patra:
            tick_patra(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Ganon:
            tick_ganon(session, world, &enemy, target_player, dt_seconds);
            break;
        }

        if (target_player == nullptr || enemy.hidden) {
            continue;
        }

        if (overlaps_circle(enemy.position, player->position, kEnemyTouchRadius)) {
            if (enemy.kind == EnemyKind::Wallmaster) {
                player->position = session->area_kind == AreaKind::Overworld
                                       ? get_opening_start_position()
                                       : enemy.origin;
                set_message(session, "wallmaster drag", 1.2F);
                continue;
            }

            if (enemy.kind == EnemyKind::Bubble) {
                if (enemy.subtype == 0) {
                    player->sword_disabled_seconds =
                        glm::max(player->sword_disabled_seconds, kBubbleCurseSeconds);
                    set_message(session, "bubble curse", 0.8F);
                } else if (enemy.subtype == 1) {
                    player->sword_cursed = true;
                    player->sword_disabled_seconds = 9999.0F;
                    set_message(session, "bubble block", 0.8F);
                } else {
                    player->sword_cursed = false;
                    player->sword_disabled_seconds = 0.0F;
                    set_message(session, "bubble restore", 0.8F);
                }
                continue;
            }

            if (enemy.kind == EnemyKind::LikeLike) {
                player->stunned_seconds = glm::max(player->stunned_seconds, kLikeLikeGrabSeconds);
                player->position = enemy.position;
                enemy.special_counter += 1;
                if (enemy.special_counter >= 60 && player->has_magic_shield) {
                    player->has_magic_shield = false;
                    set_message(session, "like-like ate shield", 1.0F);
                } else {
                    set_message(session, "like-like grab", 0.2F);
                }
            }

            damage_player_from(session, world, player, 1, enemy.position);
        }
    }
}

void tick_enemy_respawns(GameSession* session, float dt_seconds) {
    for (Enemy& enemy : session->enemies) {
        if (enemy.active || !enemy.zoo_respawn) {
            continue;
        }

        bool group_active = false;
        for (const Enemy& other : session->enemies) {
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
        reset_enemy_state(session, &enemy);
    }
}

void respawn_enemy_group_internal(GameSession* session, int respawn_group) {
    for (Enemy& enemy : session->enemies) {
        if (enemy.respawn_group != respawn_group) {
            continue;
        }

        enemy.active = true;
        enemy.position = enemy.spawn_position;
        enemy.origin = enemy.spawn_position;
        enemy.respawn_seconds_remaining = 0.0F;
        reset_enemy_state(session, &enemy);
    }
}

void apply_player_pickup(Player* player, GameSession* session, Pickup* pickup) {
    if (pickup->shop_item && player->rupees < pickup->price_rupees) {
        set_message(session, "need " + std::to_string(pickup->price_rupees) + " rupees", 1.4F);
        return;
    }

    if (pickup->shop_item) {
        player->rupees -= pickup->price_rupees;
    }

    switch (pickup->kind) {
    case PickupKind::None:
        break;
    case PickupKind::Sword:
        player->has_sword = true;
        session->sword_cave_reward_taken = true;
        set_message(session, "got wooden sword", 1.8F);
        break;
    case PickupKind::Heart:
        player->health = glm::min(player->max_health, player->health + 1);
        set_message(session, "heart recovered", 0.9F);
        break;
    case PickupKind::Rupee:
        player->rupees += pickup->shop_item ? 0 : 5;
        set_message(session, "rupees +5", 0.9F);
        break;
    case PickupKind::Bombs:
        player->bombs = glm::min(player->max_bombs, player->bombs + 4);
        player->max_bombs = glm::max(player->max_bombs, 8);
        select_if_unset(player, UseItemKind::Bombs);
        set_message(session, pickup->shop_item ? "bought bombs" : "bombs +4", 1.0F);
        break;
    case PickupKind::Boomerang:
        player->has_boomerang = true;
        select_if_unset(player, UseItemKind::Boomerang);
        set_message(session, pickup->shop_item ? "bought boomerang" : "got boomerang", 1.2F);
        break;
    case PickupKind::Bow:
        player->has_bow = true;
        select_if_unset(player, UseItemKind::Bow);
        set_message(session, pickup->shop_item ? "bought bow" : "got bow", 1.2F);
        break;
    case PickupKind::Candle:
        player->has_candle = true;
        select_if_unset(player, UseItemKind::Candle);
        set_message(session, pickup->shop_item ? "bought candle" : "got candle", 1.2F);
        break;
    case PickupKind::BluePotion:
        if (pickup->area_kind == AreaKind::ItemZoo && !player->has_letter) {
            set_message(session, "need letter for potion", 1.2F);
            return;
        }
        player->has_potion = true;
        select_if_unset(player, UseItemKind::Potion);
        set_message(session, pickup->shop_item ? "bought potion" : "got potion", 1.2F);
        break;
    case PickupKind::HeartContainer:
        player->max_health += 1;
        player->health = player->max_health;
        set_message(session, "heart container", 1.4F);
        break;
    case PickupKind::Key:
        player->keys += 1;
        set_message(session, "key +1", 1.0F);
        break;
    case PickupKind::Recorder:
        player->has_recorder = true;
        select_if_unset(player, UseItemKind::Recorder);
        set_message(session, pickup->shop_item ? "bought recorder" : "got recorder", 1.2F);
        break;
    case PickupKind::Ladder:
        player->has_ladder = true;
        set_message(session, pickup->shop_item ? "bought ladder" : "got ladder", 1.2F);
        break;
    case PickupKind::Raft:
        player->has_raft = true;
        set_message(session, pickup->shop_item ? "bought raft" : "got raft", 1.2F);
        break;
    case PickupKind::Food:
        player->has_food = true;
        select_if_unset(player, UseItemKind::Food);
        set_message(session, pickup->shop_item ? "bought bait" : "got bait", 1.2F);
        break;
    case PickupKind::Letter:
        player->has_letter = true;
        set_message(session, "got letter", 1.2F);
        break;
    case PickupKind::MagicShield:
        player->has_magic_shield = true;
        set_message(session, pickup->shop_item ? "bought magic shield" : "got magic shield", 1.3F);
        break;
    case PickupKind::SilverArrows:
        player->has_silver_arrows = true;
        set_message(session, pickup->shop_item ? "bought silver arrows" : "got silver arrows",
                    1.3F);
        break;
    }

    pickup->collected = true;
    pickup->active = false;
}

void tick_pickups(GameSession* session, const World* overworld_world, Player* player,
                  float dt_seconds) {
    for (Pickup& pickup : session->pickups) {
        if (!pickup.active) {
            continue;
        }

        if (!pickup.persistent) {
            pickup.seconds_remaining -= dt_seconds;
            pickup.velocity.y += kPickupGravityTilesPerSecond * dt_seconds;
            pickup.position += pickup.velocity * dt_seconds;
            if (pickup.seconds_remaining <= 0.0F) {
                pickup.active = false;
                continue;
            }
        }

        const World* world =
            get_world_for_area(session, overworld_world, pickup.area_kind, pickup.cave_id);
        if (!world_is_walkable_tile(world, pickup.position + glm::vec2(0.0F, 0.4F))) {
            pickup.velocity = glm::vec2(0.0F);
        }

        if (!pickup_in_current_area(session, pickup)) {
            continue;
        }

        const float radius = pickup.kind == PickupKind::Sword ? kSwordPickupRadius : kPickupRadius;
        if (!overlaps_circle(pickup.position, player->position, radius)) {
            continue;
        }

        apply_player_pickup(player, session, &pickup);
    }
}

void trigger_explosion(GameSession* session, const Projectile& bomb) {
    make_projectile(session, bomb.area_kind, bomb.cave_id, ProjectileKind::Explosion, true,
                    bomb.position, glm::vec2(0.0F), kExplosionSeconds, kExplosionRadius, 2);
}

void tick_projectiles(GameSession* session, const World* overworld_world, Player* player,
                      float dt_seconds) {
    for (Projectile& projectile : session->projectiles) {
        if (!projectile.active) {
            continue;
        }

        const World* world =
            get_world_for_area(session, overworld_world, projectile.area_kind, projectile.cave_id);
        const bool active_area = in_area(session, projectile.area_kind, projectile.cave_id);
        projectile.seconds_remaining -= dt_seconds;
        if (projectile.kind == ProjectileKind::Bomb && projectile.seconds_remaining <= 0.0F) {
            projectile.active = false;
            trigger_explosion(session, projectile);
            continue;
        }

        if (projectile.seconds_remaining <= 0.0F) {
            projectile.active = false;
            continue;
        }

        if (projectile.kind == ProjectileKind::Boomerang &&
            projectile.seconds_remaining <= kBoomerangFlightSeconds * 0.5F) {
            projectile.returning = true;
            const glm::vec2 toward_player = player->position - projectile.position;
            if (glm::length(toward_player) > 0.001F) {
                projectile.velocity = glm::normalize(toward_player) * kBoomerangSpeed;
            }
        }

        if (projectile.kind != ProjectileKind::Bomb && projectile.kind != ProjectileKind::Food &&
            projectile.kind != ProjectileKind::Explosion) {
            const glm::vec2 candidate = projectile.position + projectile.velocity * dt_seconds;
            if (!world_is_walkable_tile(world, candidate)) {
                if (projectile.kind == ProjectileKind::Boomerang) {
                    projectile.returning = true;
                    projectile.velocity *= -1.0F;
                } else {
                    projectile.active = false;
                    continue;
                }
            } else {
                projectile.position = candidate;
            }
        }

        if (projectile.from_player) {
            if (projectile.kind == ProjectileKind::Food) {
                continue;
            }

            for (Enemy& enemy : session->enemies) {
                if (!enemy.active || enemy.area_kind != projectile.area_kind ||
                    enemy.cave_id != projectile.cave_id ||
                    (enemy.hidden && enemy.kind != EnemyKind::Ganon)) {
                    continue;
                }

                const float radius = projectile.kind == ProjectileKind::Explosion
                                         ? kBombExplosionRadius
                                         : projectile.radius + 0.35F;
                if (!overlaps_circle(projectile.position, enemy.position, radius)) {
                    continue;
                }

                if (enemy.kind == EnemyKind::Bubble || enemy.kind == EnemyKind::Trap) {
                    continue;
                }

                if (enemy.kind == EnemyKind::PolsVoice) {
                    continue;
                }

                if ((enemy.kind == EnemyKind::BlueWizzrobe ||
                     enemy.kind == EnemyKind::RedWizzrobe) &&
                    projectile.kind != ProjectileKind::SwordBeam &&
                    projectile.kind != ProjectileKind::Fire &&
                    projectile.kind != ProjectileKind::Explosion) {
                    continue;
                }

                if (enemy.kind == EnemyKind::Dodongo) {
                    if (!projectile_hits_dodongo_mouth(enemy, projectile)) {
                        continue;
                    }

                    if (enemy.special_counter == 0) {
                        enemy.special_counter += 1;
                        enemy.state_seconds_remaining = kDodongoBloatedSeconds;
                        set_message(session, "dodongo ate bomb", 0.8F);
                    } else {
                        damage_enemy(session, &enemy, enemy.health);
                        set_message(session, "dodongo down", 0.8F);
                    }

                    projectile.active = false;
                    continue;
                }

                if (enemy.kind == EnemyKind::Gohma) {
                    if (projectile.kind != ProjectileKind::Arrow || enemy.special_counter == 0) {
                        continue;
                    }
                }

                if (enemy.kind == EnemyKind::Patra && enemy.special_counter > 0) {
                    bool hit_orbiter = false;
                    for (int orbiter = 0; orbiter < enemy.special_counter; ++orbiter) {
                        if (!overlaps_circle(projectile.position,
                                             patra_orbiter_position(enemy, orbiter), radius)) {
                            continue;
                        }
                        enemy.special_counter -= 1;
                        hit_orbiter = true;
                        set_message(session, "patra orbiter down", 0.6F);
                        break;
                    }
                    projectile.active = false;
                    if (hit_orbiter) {
                        continue;
                    }
                    continue;
                }

                if (enemy.kind == EnemyKind::Manhandla && enemy.special_counter > 0) {
                    bool hit_petal = false;
                    for (int petal = 0; petal < enemy.special_counter; ++petal) {
                        if (!overlaps_circle(projectile.position,
                                             manhandla_petal_position(enemy, petal), radius)) {
                            continue;
                        }
                        enemy.special_counter -= 1;
                        enemy.health = glm::max(1, enemy.special_counter);
                        projectile.active = false;
                        if (enemy.special_counter <= 0) {
                            damage_enemy(session, &enemy, enemy.health);
                        } else {
                            set_message(session, "manhandla petal down", 0.6F);
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
                        if (!overlaps_circle(projectile.position, gleeok_head_position(enemy, head),
                                             radius)) {
                            continue;
                        }
                        damage_enemy(session, &enemy, projectile.damage);
                        projectile.active = false;
                        hit_head = true;
                        break;
                    }
                    if (hit_head) {
                        continue;
                    }
                    continue;
                }

                if (enemy.kind == EnemyKind::Ganon) {
                    if (projectile.kind != ProjectileKind::Arrow || enemy.special_counter == 0 ||
                        !player->has_silver_arrows || enemy.health > 1) {
                        continue;
                    }
                    damage_enemy(session, &enemy, enemy.health);
                    projectile.active = false;
                    set_message(session, "ganon down", 1.0F);
                    continue;
                }

                damage_enemy(session, &enemy, projectile.damage);
                if (projectile.kind != ProjectileKind::Boomerang &&
                    projectile.kind != ProjectileKind::Explosion) {
                    projectile.active = false;
                }
            }

            if (projectile.kind == ProjectileKind::Boomerang && active_area &&
                overlaps_circle(projectile.position, player->position, 0.65F) &&
                projectile.returning) {
                projectile.active = false;
            }
            continue;
        }

        if (!active_area || !overlaps_circle(projectile.position, player->position,
                                             projectile.radius + kProjectileHitPadding)) {
            continue;
        }

        projectile.active = false;
        damage_player_from(session, world, player, projectile.damage, projectile.position);
    }
}

void compact_vectors(GameSession* session) {
    session->projectiles.erase(
        std::remove_if(session->projectiles.begin(), session->projectiles.end(),
                       [](const Projectile& projectile) { return !projectile.active; }),
        session->projectiles.end());

    session->pickups.erase(std::remove_if(session->pickups.begin(), session->pickups.end(),
                                          [](const Pickup& pickup) {
                                              return !pickup.active && !pickup.persistent &&
                                                     !pickup.shop_item;
                                          }),
                           session->pickups.end());
}

void try_enter_overworld_cave(GameSession* session, const World* overworld_world, Player* player) {
    if (session->area_kind != AreaKind::Overworld || session->warp_cooldown_seconds > 0.0F) {
        return;
    }

    if (player->move_direction != MoveDirection::Up) {
        return;
    }

    std::array<OverworldWarp, kMaxRoomWarps> warps = {};
    int warp_count = 0;
    const OverworldWarp* warp =
        find_triggered_overworld_warp(&overworld_world->overworld, session->current_room_id,
                                      player->position, &warps, &warp_count);
    if (warp == nullptr) {
        return;
    }

    session->cave_return_room_id = session->current_room_id;
    session->cave_return_position = warp->return_position;
    const CaveDef* cave = get_cave_def(warp->cave_id);
    if (cave == nullptr) {
        return;
    }

    set_area_kind(session, player, AreaKind::Cave, warp->cave_id, cave->player_spawn);
}

void try_exit_cave(GameSession* session, Player* player) {
    if (session->area_kind != AreaKind::Cave || session->warp_cooldown_seconds > 0.0F) {
        return;
    }

    const CaveDef* cave = get_cave_def(session->current_cave_id);
    if (cave == nullptr) {
        return;
    }

    const glm::vec2 delta = player->position - cave->exit_center;
    if (std::abs(delta.x) > cave->exit_half_size.x || std::abs(delta.y) > cave->exit_half_size.y) {
        return;
    }

    set_area_kind(session, player, AreaKind::Overworld, -1, session->cave_return_position);
}

void try_area_portals(GameSession* session, Player* player) {
    if (session->warp_cooldown_seconds > 0.0F) {
        return;
    }

    std::array<AreaPortal, kMaxAreaPortals> portals = {};
    const int portal_count = gather_area_portals(session, &portals);
    for (int index = 0; index < portal_count; ++index) {
        const AreaPortal& portal = portals[static_cast<std::size_t>(index)];
        if (portal.requires_raft && !player->has_raft) {
            const glm::vec2 delta = player->position - portal.center;
            if (std::abs(delta.x) <= portal.half_size.x + 0.7F &&
                std::abs(delta.y) <= portal.half_size.y + 0.7F) {
                set_message(session, "need raft", 0.2F);
            }
            continue;
        }

        const glm::vec2 delta = player->position - portal.center;
        if (std::abs(delta.x) > portal.half_size.x || std::abs(delta.y) > portal.half_size.y) {
            continue;
        }

        set_area_kind(session, player, portal.target_area_kind, portal.target_cave_id,
                      portal.target_position);
        return;
    }
}

void process_player_command(GameSession* session, const World* overworld_world, Player* player,
                            const PlayerCommand* command) {
    if (command->previous_item_pressed) {
        select_next_item(player, -1);
    }

    if (command->next_item_pressed) {
        select_next_item(player, 1);
    }

    if (command->use_item_pressed) {
        use_selected_item(session, overworld_world, player);
    }
}

void resolve_npc_collisions(const GameSession* session, Player* player,
                            const glm::vec2& previous_position) {
    for (const Npc& npc : session->npcs) {
        if (!npc.active || npc.solved || !in_area(session, npc.area_kind, npc.cave_id)) {
            continue;
        }

        if (npc.kind != NpcKind::HungryGoriya) {
            continue;
        }

        if (!overlaps_circle(player->position, npc.position, 0.95F)) {
            continue;
        }

        player->position = previous_position;
        return;
    }
}

void tick_npcs(GameSession* session, Player* player, float dt_seconds) {
    for (Npc& npc : session->npcs) {
        if (!npc.active) {
            continue;
        }

        npc.action_seconds_remaining = glm::max(0.0F, npc.action_seconds_remaining - dt_seconds);

        if (npc.kind == NpcKind::Fairy) {
            npc.state_seconds_remaining += dt_seconds;
            const float phase = npc.state_seconds_remaining * 6.0F;
            npc.position = npc.origin + glm::vec2(std::cos(phase) * 0.55F, std::sin(phase) * 0.35F);
            if (in_area(session, npc.area_kind, npc.cave_id) &&
                overlaps_circle(player->position, npc.position, 0.8F)) {
                player->health = player->max_health;
                set_message(session, "fairy healed", 1.0F);
            }
            continue;
        }

        npc.state_seconds_remaining = glm::max(0.0F, npc.state_seconds_remaining - dt_seconds);

        if (npc.kind != NpcKind::HungryGoriya || npc.solved) {
            continue;
        }

        Projectile* food = find_active_food(session, npc.area_kind, npc.cave_id);
        if (food == nullptr || !overlaps_circle(food->position, npc.position, 1.2F)) {
            continue;
        }

        food->active = false;
        npc.solved = true;
        npc.active = false;
        set_message(session, "hungry goriya ate bait", 1.4F);
    }
}

void update_npc_messages(GameSession* session, const Player* player) {
    if (session->message_seconds_remaining > 0.0F) {
        return;
    }

    for (const Npc& npc : session->npcs) {
        if (!npc.active || !in_area(session, npc.area_kind, npc.cave_id)) {
            continue;
        }

        if (!overlaps_circle(player->position, npc.position, 1.8F)) {
            continue;
        }

        if (npc.kind == NpcKind::ShopKeeper) {
            set_message(session, "shop: touch item to buy", 0.2F);
        } else if (npc.kind == NpcKind::OldWoman) {
            set_message(session, player->has_letter ? "potion shop is open" : "show me the letter",
                        0.2F);
        } else if (npc.kind == NpcKind::HungryGoriya) {
            set_message(session, "hungry goriya needs bait", 0.2F);
        } else if (npc.kind == NpcKind::Fairy) {
            set_message(session, "fairy restores hearts", 0.2F);
        } else {
            set_message(session, npc.label, 0.2F);
        }
        return;
    }
}

} // namespace

void respawn_enemy_group(GameSession* session, int respawn_group) {
    respawn_enemy_group_internal(session, respawn_group);
}

GameSession make_game_session() {
    GameSession session;
    session.cave_world = make_world();
    resize_world(&session.cave_world, 16, 11);
    fill_world(&session.cave_world, TileKind::Ground);
    fill_world_rect(&session.cave_world, 0, 0, 16, 1, TileKind::Wall);
    fill_world_rect(&session.cave_world, 0, 0, 1, 11, TileKind::Wall);
    fill_world_rect(&session.cave_world, 15, 0, 1, 11, TileKind::Wall);
    fill_world_rect(&session.cave_world, 0, 10, 16, 1, TileKind::Wall);
    fill_world_rect(&session.cave_world, 7, 10, 3, 1, TileKind::Ground);

    session.enemy_zoo_world = make_world();
    session.item_zoo_world = make_world();
    build_enemy_zoo_world(&session.enemy_zoo_world);
    build_item_zoo_world(&session.item_zoo_world);

    session.enemies.reserve(256);
    session.projectiles.reserve(512);
    session.pickups.reserve(256);
    session.npcs.reserve(32);
    return session;
}

void init_game_session(GameSession* session, Player* player) {
    *session = make_game_session();
    *player = make_player();
    player->position = get_opening_start_position();
    player->facing = Facing::Down;
    player->selected_item = UseItemKind::Bombs;
    session->area_kind = AreaKind::Overworld;
    session->current_cave_id = -1;
    init_opening_overworld_enemies(session);
    populate_sandbox_entities(session);
    ensure_sword_cave_pickup(session);
    update_current_room(session, player);
}

const World* get_active_world(const GameSession* session, const World* overworld_world) {
    switch (session->area_kind) {
    case AreaKind::Overworld:
        return overworld_world;
    case AreaKind::Cave:
        return &session->cave_world;
    case AreaKind::EnemyZoo:
        return &session->enemy_zoo_world;
    case AreaKind::ItemZoo:
        return &session->item_zoo_world;
    }

    return overworld_world;
}

void tick_game_session(GameSession* session, const World* overworld_world, Player* player,
                       const PlayerCommand* command, float dt_seconds) {
    player->invincibility_seconds = glm::max(0.0F, player->invincibility_seconds - dt_seconds);
    session->warp_cooldown_seconds = glm::max(0.0F, session->warp_cooldown_seconds - dt_seconds);
    session->message_seconds_remaining =
        glm::max(0.0F, session->message_seconds_remaining - dt_seconds);
    if (session->message_seconds_remaining <= 0.0F) {
        session->message_text.clear();
    }

    PlayerCommand resolved = *command;
    if (!player->has_sword) {
        resolved.attack_pressed = false;
    }
    resolved.ignore_world_collision = session->area_kind == AreaKind::EnemyZoo;

    const World* active_world = get_active_world(session, overworld_world);
    const glm::vec2 previous_player_position = player->position;
    tick_player(player, active_world, &resolved, dt_seconds);
    resolve_npc_collisions(session, player, previous_player_position);
    process_player_command(session, overworld_world, player, &resolved);
    update_current_room(session, player);

    if (resolved.attack_pressed && player->has_sword && player->health == player->max_health) {
        create_player_sword_beam(session, player);
    }

    process_player_attacks(session, player);
    tick_enemies(session, overworld_world, player, dt_seconds);
    tick_enemy_respawns(session, dt_seconds);
    tick_projectiles(session, overworld_world, player, dt_seconds);
    tick_pickups(session, overworld_world, player, dt_seconds);
    tick_npcs(session, player, dt_seconds);

    if (session->area_kind == AreaKind::Overworld) {
        try_enter_overworld_cave(session, overworld_world, player);
    } else if (session->area_kind == AreaKind::Cave) {
        try_exit_cave(session, player);
    } else {
        try_area_portals(session, player);
    }

    update_npc_messages(session, player);
    compact_vectors(session);
    update_current_room(session, player);
}

void set_area_kind(GameSession* session, Player* player, AreaKind area_kind, int cave_id,
                   const glm::vec2& position) {
    session->area_kind = area_kind;
    session->current_cave_id = area_kind == AreaKind::Cave ? cave_id : -1;
    player->position = position;
    player->move_direction = MoveDirection::None;
    player->sword_cursed = false;
    player->sword_disabled_seconds = 0.0F;
    session->warp_cooldown_seconds = kAreaTransitionCooldownSeconds;
    update_current_room(session, player);
    if (area_kind == AreaKind::Cave) {
        ensure_sword_cave_pickup(session);
    }

    if (area_kind == AreaKind::Cave) {
        set_message(session, "entered cave", 1.0F);
    } else {
        set_message(session, std::string("entered ") + area_name(session), 1.0F);
    }
}

int gather_area_portals(const GameSession* session,
                        std::array<AreaPortal, kMaxAreaPortals>* portals) {
    return gather_sandbox_portals(session, portals);
}

const char* area_name(const GameSession* session) {
    switch (session->area_kind) {
    case AreaKind::Overworld:
        return "overworld";
    case AreaKind::Cave:
        return "cave";
    case AreaKind::EnemyZoo:
        return "enemy-zoo";
    case AreaKind::ItemZoo:
        return "item-zoo";
    }

    return "overworld";
}

const char* pickup_name(PickupKind kind) {
    switch (kind) {
    case PickupKind::None:
        return "none";
    case PickupKind::Sword:
        return "sword";
    case PickupKind::Heart:
        return "heart";
    case PickupKind::Rupee:
        return "rupee";
    case PickupKind::Bombs:
        return "bombs";
    case PickupKind::Boomerang:
        return "boomerang";
    case PickupKind::Bow:
        return "bow";
    case PickupKind::Candle:
        return "candle";
    case PickupKind::BluePotion:
        return "blue-potion";
    case PickupKind::HeartContainer:
        return "heart-container";
    case PickupKind::Key:
        return "key";
    case PickupKind::Recorder:
        return "recorder";
    case PickupKind::Ladder:
        return "ladder";
    case PickupKind::Raft:
        return "raft";
    case PickupKind::Food:
        return "food";
    case PickupKind::Letter:
        return "letter";
    case PickupKind::MagicShield:
        return "magic-shield";
    case PickupKind::SilverArrows:
        return "silver-arrows";
    }

    return "none";
}

const char* enemy_name(EnemyKind kind) {
    switch (kind) {
    case EnemyKind::Octorok:
        return "octorok";
    case EnemyKind::Moblin:
        return "moblin";
    case EnemyKind::Lynel:
        return "lynel";
    case EnemyKind::Goriya:
        return "goriya";
    case EnemyKind::Darknut:
        return "darknut";
    case EnemyKind::Tektite:
        return "tektite";
    case EnemyKind::Leever:
        return "leever";
    case EnemyKind::Keese:
        return "keese";
    case EnemyKind::Zol:
        return "zol";
    case EnemyKind::Gel:
        return "gel";
    case EnemyKind::Rope:
        return "rope";
    case EnemyKind::Vire:
        return "vire";
    case EnemyKind::Stalfos:
        return "stalfos";
    case EnemyKind::Gibdo:
        return "gibdo";
    case EnemyKind::LikeLike:
        return "like-like";
    case EnemyKind::PolsVoice:
        return "pols-voice";
    case EnemyKind::BlueWizzrobe:
        return "blue-wizzrobe";
    case EnemyKind::RedWizzrobe:
        return "red-wizzrobe";
    case EnemyKind::Wallmaster:
        return "wallmaster";
    case EnemyKind::Ghini:
        return "ghini";
    case EnemyKind::FlyingGhini:
        return "flying-ghini";
    case EnemyKind::Bubble:
        return "bubble";
    case EnemyKind::Trap:
        return "trap";
    case EnemyKind::Armos:
        return "armos";
    case EnemyKind::Zora:
        return "zora";
    case EnemyKind::Peahat:
        return "peahat";
    case EnemyKind::Dodongo:
        return "dodongo";
    case EnemyKind::Digdogger:
        return "digdogger";
    case EnemyKind::Manhandla:
        return "manhandla";
    case EnemyKind::Gohma:
        return "gohma";
    case EnemyKind::Moldorm:
        return "moldorm";
    case EnemyKind::Aquamentus:
        return "aquamentus";
    case EnemyKind::Gleeok:
        return "gleeok";
    case EnemyKind::Patra:
        return "patra";
    case EnemyKind::Ganon:
        return "ganon";
    }

    return "enemy";
}

const char* projectile_name(ProjectileKind kind) {
    switch (kind) {
    case ProjectileKind::Rock:
        return "rock";
    case ProjectileKind::Arrow:
        return "arrow";
    case ProjectileKind::SwordBeam:
        return "sword-beam";
    case ProjectileKind::Boomerang:
        return "boomerang";
    case ProjectileKind::Fire:
        return "fire";
    case ProjectileKind::Food:
        return "food";
    case ProjectileKind::Bomb:
        return "bomb";
    case ProjectileKind::Explosion:
        return "explosion";
    }

    return "projectile";
}

const char* npc_name(NpcKind kind) {
    switch (kind) {
    case NpcKind::OldMan:
        return "old-man";
    case NpcKind::ShopKeeper:
        return "shopkeeper";
    case NpcKind::HungryGoriya:
        return "hungry-goriya";
    case NpcKind::OldWoman:
        return "old-woman";
    case NpcKind::Fairy:
        return "fairy";
    }

    return "npc";
}

} // namespace z1m

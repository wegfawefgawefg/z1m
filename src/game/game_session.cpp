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
constexpr float kKeeseSpeed = 5.7F;
constexpr float kLeeverSpeed = 4.2F;
constexpr float kTektiteHopSpeed = 7.2F;
constexpr float kAquamentusSpeed = 3.0F;
constexpr float kLynelSpeed = 5.7F;
constexpr float kPeahatSpeed = 6.6F;
constexpr float kRockSpeed = 8.0F;
constexpr float kArrowSpeed = 12.5F;
constexpr float kSwordBeamSpeed = 11.5F;
constexpr float kBoomerangSpeed = 9.2F;
constexpr float kFireSpeed = 7.5F;
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
    enemy->hurt_seconds_remaining = 0.0F;
    enemy->hidden = false;
    enemy->invulnerable = false;
    enemy->velocity = glm::vec2(0.0F, 0.0F);

    switch (enemy->kind) {
    case EnemyKind::Octorok:
        enemy->health = glm::max(enemy->health, 1);
        enemy->move_seconds_remaining = 0.25F + random_unit(session) * 0.40F;
        enemy->action_seconds_remaining = 0.70F + random_unit(session) * 0.90F;
        break;
    case EnemyKind::Moblin:
        enemy->health = glm::max(enemy->health, 2);
        enemy->move_seconds_remaining = 0.25F + random_unit(session) * 0.45F;
        enemy->action_seconds_remaining = 0.55F + random_unit(session) * 0.80F;
        break;
    case EnemyKind::Lynel:
        enemy->health = glm::max(enemy->health, 4);
        enemy->move_seconds_remaining = 0.25F + random_unit(session) * 0.35F;
        enemy->action_seconds_remaining = 0.45F + random_unit(session) * 0.65F;
        break;
    case EnemyKind::Tektite:
        enemy->health = glm::max(enemy->health, 1);
        enemy->move_seconds_remaining = 0.0F;
        enemy->action_seconds_remaining = 0.25F + random_unit(session) * 0.35F;
        break;
    case EnemyKind::Leever:
        enemy->health = glm::max(enemy->health, 2);
        enemy->hidden = true;
        enemy->state_seconds_remaining = 0.4F + random_unit(session) * 0.4F;
        enemy->action_seconds_remaining = 0.0F;
        enemy->move_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Keese:
        enemy->health = glm::max(enemy->health, 1);
        enemy->action_seconds_remaining = 0.25F + random_unit(session) * 0.30F;
        enemy->move_seconds_remaining = 1.0F + random_unit(session) * 1.2F;
        break;
    case EnemyKind::Zora:
        enemy->health = glm::max(enemy->health, 1);
        enemy->hidden = true;
        enemy->state_seconds_remaining = 1.0F + random_unit(session) * 0.8F;
        enemy->action_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Peahat:
        enemy->health = glm::max(enemy->health, 2);
        enemy->invulnerable = true;
        enemy->state_seconds_remaining = 1.8F + random_unit(session) * 0.8F;
        enemy->action_seconds_remaining = 0.15F;
        enemy->move_seconds_remaining = 0.55F;
        break;
    case EnemyKind::Aquamentus:
        enemy->health = glm::max(enemy->health, 6);
        enemy->velocity = glm::vec2(kAquamentusSpeed, 0.0F);
        enemy->action_seconds_remaining = 0.9F;
        enemy->move_seconds_remaining = 9999.0F;
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
    case UseItemKind::Potion:
        return player->has_potion;
    }

    return false;
}

void select_next_item(Player* player, int direction) {
    constexpr std::array<UseItemKind, 6> order = {
        UseItemKind::Bombs,  UseItemKind::Boomerang, UseItemKind::Bow,
        UseItemKind::Candle, UseItemKind::Recorder,  UseItemKind::Potion,
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

void use_recorder(GameSession* session, const World* overworld_world, Player* player) {
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

    enemy->hurt_seconds_remaining = 0.20F;
    enemy->health -= damage;
    if (enemy->health > 0) {
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
        if (!enemy_in_current_area(session, enemy) || enemy.hidden) {
            continue;
        }

        if (!overlaps_circle(enemy.position, sword_pos, kSwordHitRadius)) {
            continue;
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
        case EnemyKind::Tektite:
            tick_tektite(session, world, &enemy, dt_seconds);
            break;
        case EnemyKind::Leever:
            tick_leever(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Keese:
            tick_keese(session, world, &enemy, dt_seconds);
            break;
        case EnemyKind::Zora:
            tick_zora(session, world, &enemy, target_player, dt_seconds);
            break;
        case EnemyKind::Peahat:
            tick_peahat(session, world, &enemy, dt_seconds);
            break;
        case EnemyKind::Aquamentus:
            tick_aquamentus(session, world, &enemy, target_player, dt_seconds);
            break;
        }

        if (target_player == nullptr || enemy.hidden) {
            continue;
        }

        if (overlaps_circle(enemy.position, player->position, kEnemyTouchRadius)) {
            damage_player_from(session, world, player, 1, enemy.position);
        }
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

        if (projectile.kind != ProjectileKind::Bomb &&
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
            for (Enemy& enemy : session->enemies) {
                if (!enemy.active || enemy.area_kind != projectile.area_kind ||
                    enemy.cave_id != projectile.cave_id || enemy.hidden) {
                    continue;
                }

                const float radius = projectile.kind == ProjectileKind::Explosion
                                         ? kBombExplosionRadius
                                         : projectile.radius + 0.35F;
                if (!overlaps_circle(projectile.position, enemy.position, radius)) {
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
        } else {
            set_message(session, npc.label, 0.2F);
        }
        return;
    }
}

} // namespace

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
    resolved.ignore_world_collision =
        session->area_kind == AreaKind::EnemyZoo || session->area_kind == AreaKind::ItemZoo;

    const World* active_world = get_active_world(session, overworld_world);
    tick_player(player, active_world, &resolved, dt_seconds);
    process_player_command(session, overworld_world, player, &resolved);
    update_current_room(session, player);

    if (resolved.attack_pressed && player->has_sword && player->health == player->max_health) {
        create_player_sword_beam(session, player);
    }

    process_player_attacks(session, player);
    tick_enemies(session, overworld_world, player, dt_seconds);
    tick_projectiles(session, overworld_world, player, dt_seconds);
    tick_pickups(session, overworld_world, player, dt_seconds);

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
    case EnemyKind::Tektite:
        return "tektite";
    case EnemyKind::Leever:
        return "leever";
    case EnemyKind::Keese:
        return "keese";
    case EnemyKind::Zora:
        return "zora";
    case EnemyKind::Peahat:
        return "peahat";
    case EnemyKind::Aquamentus:
        return "aquamentus";
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
    }

    return "npc";
}

} // namespace z1m

#include "game/game_session.hpp"

#include "content/opening_content.hpp"
#include "content/overworld_warps.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace z1m {

namespace {

constexpr float kEnemySpeedTilesPerSecond = 4.5F;
constexpr float kProjectileSpeedTilesPerSecond = 8.0F;
constexpr float kPickupGravityTilesPerSecond = 10.0F;
constexpr float kPickupLifetimeSeconds = 8.0F;
constexpr float kEnemyTouchRadius = 0.60F;
constexpr float kSwordHitRadius = 0.95F;
constexpr float kPickupRadius = 0.65F;
constexpr float kCavePickupRadius = 0.80F;
constexpr float kProjectileRadius = 0.30F;
constexpr float kPlayerDamageInvincibilitySeconds = 1.0F;
constexpr float kRoomTransitionCooldownSeconds = 0.25F;
constexpr int kSwordCaveId = 0x10;

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

bool overlaps_circle(const glm::vec2& a, const glm::vec2& b, float radius) {
    return glm::length(a - b) <= radius;
}

bool is_in_cave(const GameSession* session) {
    return session->area_kind == AreaKind::Cave;
}

void clear_room_entities(GameSession* session) {
    for (Enemy& enemy : session->enemies) {
        enemy.active = false;
    }

    for (Projectile& projectile : session->projectiles) {
        projectile.active = false;
    }

    for (Pickup& pickup : session->pickups) {
        if (!pickup.persistent) {
            pickup.active = false;
        }
    }
}

Enemy* add_enemy(GameSession* session) {
    for (Enemy& enemy : session->enemies) {
        if (!enemy.active) {
            enemy = Enemy{};
            enemy.active = true;
            return &enemy;
        }
    }

    return nullptr;
}

Projectile* add_projectile(GameSession* session) {
    for (Projectile& projectile : session->projectiles) {
        if (!projectile.active) {
            projectile = Projectile{};
            projectile.active = true;
            return &projectile;
        }
    }

    return nullptr;
}

Pickup* add_pickup(GameSession* session) {
    for (Pickup& pickup : session->pickups) {
        if (!pickup.active) {
            pickup = Pickup{};
            pickup.active = true;
            return &pickup;
        }
    }

    return nullptr;
}

void spawn_room_enemies(GameSession* session, int room_id) {
    int spawn_count = 0;
    const EnemySpawnDef* spawns = get_room_enemy_spawns(room_id, &spawn_count);
    if (spawns == nullptr || spawn_count <= 0) {
        session->room_runtime[static_cast<std::size_t>(room_id)].cleared = true;
        return;
    }

    for (int index = 0; index < spawn_count; ++index) {
        Enemy* enemy = add_enemy(session);
        if (enemy == nullptr) {
            return;
        }

        enemy->kind = EnemyKind::Octorok;
        enemy->room_id = room_id;
        enemy->position = make_world_position(room_id, spawns[index].local_position);
        enemy->facing = Facing::Down;
        enemy->health = 1;
        enemy->move_seconds_remaining = 0.2F + random_unit(session) * 0.5F;
        enemy->action_seconds_remaining = 0.5F + random_unit(session) * 1.0F;
    }
}

void enter_overworld_room(GameSession* session, int room_id) {
    clear_room_entities(session);
    if (room_id < 0 || room_id >= kScreenCount) {
        return;
    }

    if (!session->room_runtime[static_cast<std::size_t>(room_id)].cleared) {
        spawn_room_enemies(session, room_id);
    }
}

void ensure_sword_cave_pickup(GameSession* session) {
    if (session->sword_cave_reward_taken) {
        return;
    }

    for (const Pickup& pickup : session->pickups) {
        if (pickup.active && pickup.kind == PickupKind::Sword && pickup.cave_id == kSwordCaveId) {
            return;
        }
    }

    const CaveDef* cave = get_cave_def(kSwordCaveId);
    if (cave == nullptr) {
        return;
    }

    Pickup* pickup = add_pickup(session);
    if (pickup == nullptr) {
        return;
    }

    pickup->persistent = true;
    pickup->cave_id = kSwordCaveId;
    pickup->kind = PickupKind::Sword;
    pickup->position = cave->reward_position;
    pickup->seconds_remaining = 9999.0F;
}

void enter_cave(GameSession* session, Player* player, int cave_id) {
    const CaveDef* cave = get_cave_def(cave_id);
    if (cave == nullptr) {
        return;
    }

    clear_room_entities(session);
    session->cave_return_room_id = session->current_room_id;
    session->cave_return_position = player->position;
    session->area_kind = AreaKind::Cave;
    session->current_cave_id = cave_id;
    session->current_room_id = cave_id;
    session->warp_cooldown_seconds = kRoomTransitionCooldownSeconds;
    player->position = cave->player_spawn;
    player->facing = Facing::Down;
    player->move_direction = MoveDirection::None;
    ensure_sword_cave_pickup(session);
}

void exit_cave(GameSession* session, Player* player) {
    session->area_kind = AreaKind::Overworld;
    session->current_cave_id = -1;
    session->warp_cooldown_seconds = kRoomTransitionCooldownSeconds;
    player->position = session->cave_return_position;
    session->current_room_id = session->cave_return_room_id;
    session->previous_room_id = -1;
    enter_overworld_room(session, session->current_room_id);
}

void choose_enemy_direction(GameSession* session, const World* world, Enemy* enemy) {
    const std::array<Facing, 4> facings = {Facing::Up, Facing::Down, Facing::Left, Facing::Right};

    for (int attempt = 0; attempt < 8; ++attempt) {
        const Facing facing = facings[static_cast<std::size_t>(random_int(session, 4))];
        const glm::vec2 direction = facing_vector(facing);
        const glm::vec2 probe = enemy->position + direction * 0.8F;
        if (!world_is_walkable_tile(world, probe)) {
            continue;
        }

        enemy->facing = facing;
        enemy->move_seconds_remaining = 0.35F + random_unit(session) * 0.95F;
        enemy->action_seconds_remaining = 0.45F + random_unit(session) * 1.25F;
        return;
    }

    enemy->move_seconds_remaining = 0.2F;
    enemy->action_seconds_remaining = 0.3F;
}

void try_spawn_enemy_projectile(GameSession* session, const Enemy* enemy) {
    Projectile* projectile = add_projectile(session);
    if (projectile == nullptr) {
        return;
    }

    projectile->room_id = enemy->room_id;
    projectile->position = enemy->position + facing_vector(enemy->facing) * 0.7F;
    projectile->velocity = facing_vector(enemy->facing) * kProjectileSpeedTilesPerSecond;
    projectile->seconds_remaining = 2.2F;
}

void maybe_drop_pickup(GameSession* session, const Enemy* enemy) {
    Pickup* pickup = add_pickup(session);
    if (pickup == nullptr) {
        return;
    }

    const int roll = random_int(session, 5);
    if (roll == 0) {
        pickup->kind = PickupKind::Heart;
    } else if (roll == 1 || roll == 2) {
        pickup->kind = PickupKind::Rupee;
    } else {
        pickup->kind = PickupKind::Bombs;
    }

    pickup->room_id = enemy->room_id;
    pickup->position = enemy->position;
    pickup->velocity = glm::vec2(0.0F, -1.4F);
    pickup->seconds_remaining = kPickupLifetimeSeconds;
}

void damage_player_from(GameSession* session, const World* world, Player* player,
                        const glm::vec2& source_position) {
    if (player->invincibility_seconds > 0.0F || player->health <= 0) {
        return;
    }

    player->health = glm::max(0, player->health - 1);
    player->invincibility_seconds = kPlayerDamageInvincibilitySeconds;

    glm::vec2 push_direction = player->position - source_position;
    if (glm::length(push_direction) < 0.001F) {
        push_direction = glm::vec2(0.0F, 1.0F);
    } else {
        push_direction = glm::normalize(push_direction);
    }

    const glm::vec2 candidate = player->position + push_direction * 0.6F;
    if (world_is_walkable_tile(world, candidate)) {
        player->position = candidate;
    }

    if (player->health <= 0) {
        player->health = player->max_health;
        session->area_kind = AreaKind::Overworld;
        session->current_cave_id = -1;
        player->position = get_opening_start_position();
        player->facing = Facing::Down;
        session->current_room_id = get_opening_start_room_id();
        session->previous_room_id = -1;
        clear_room_entities(session);
        enter_overworld_room(session, session->current_room_id);
    }
}

void update_enemy_clear_state(GameSession* session, int room_id) {
    if (room_id < 0 || room_id >= kScreenCount) {
        return;
    }

    for (const Enemy& enemy : session->enemies) {
        if (enemy.active && enemy.room_id == room_id) {
            return;
        }
    }

    session->room_runtime[static_cast<std::size_t>(room_id)].cleared = true;
}

void tick_enemies(GameSession* session, const World* world, Player* player, float dt_seconds) {
    for (Enemy& enemy : session->enemies) {
        if (!enemy.active || enemy.room_id != session->current_room_id) {
            continue;
        }

        if (enemy.hurt_seconds_remaining > 0.0F) {
            enemy.hurt_seconds_remaining =
                glm::max(0.0F, enemy.hurt_seconds_remaining - dt_seconds);
        }

        enemy.action_seconds_remaining -= dt_seconds;
        enemy.move_seconds_remaining -= dt_seconds;

        if (enemy.move_seconds_remaining <= 0.0F) {
            choose_enemy_direction(session, world, &enemy);
        }

        const glm::vec2 velocity = facing_vector(enemy.facing) * kEnemySpeedTilesPerSecond;
        const glm::vec2 candidate = enemy.position + velocity * dt_seconds;
        if (world_is_walkable_tile(world, candidate)) {
            enemy.position = candidate;
        } else {
            enemy.move_seconds_remaining = 0.0F;
        }

        if (enemy.action_seconds_remaining <= 0.0F) {
            try_spawn_enemy_projectile(session, &enemy);
            enemy.action_seconds_remaining = 1.2F + random_unit(session) * 1.4F;
        }

        if (overlaps_circle(enemy.position, player->position, kEnemyTouchRadius)) {
            damage_player_from(session, world, player, enemy.position);
        }
    }
}

void tick_projectiles(GameSession* session, const World* world, Player* player, float dt_seconds) {
    for (Projectile& projectile : session->projectiles) {
        if (!projectile.active || projectile.room_id != session->current_room_id) {
            continue;
        }

        projectile.seconds_remaining -= dt_seconds;
        if (projectile.seconds_remaining <= 0.0F) {
            projectile.active = false;
            continue;
        }

        const glm::vec2 candidate = projectile.position + projectile.velocity * dt_seconds;
        if (!world_is_walkable_tile(world, candidate)) {
            projectile.active = false;
            continue;
        }

        projectile.position = candidate;
        if (overlaps_circle(projectile.position, player->position, kProjectileRadius + 0.35F)) {
            projectile.active = false;
            damage_player_from(session, world, player, projectile.position);
        }
    }
}

void apply_pickup(Pickup* pickup, Player* player, GameSession* session) {
    switch (pickup->kind) {
    case PickupKind::Sword:
        player->has_sword = true;
        session->sword_cave_reward_taken = true;
        break;
    case PickupKind::Heart:
        player->health = glm::min(player->max_health, player->health + 1);
        break;
    case PickupKind::Rupee:
        player->rupees += 1;
        break;
    case PickupKind::Bombs:
        player->bombs += 4;
        break;
    case PickupKind::None:
        break;
    }

    pickup->active = false;
}

void tick_pickups(GameSession* session, Player* player, float dt_seconds) {
    for (Pickup& pickup : session->pickups) {
        if (!pickup.active) {
            continue;
        }

        const bool pickup_in_current_area =
            (session->area_kind == AreaKind::Cave && pickup.cave_id == session->current_cave_id) ||
            (session->area_kind == AreaKind::Overworld &&
             pickup.room_id == session->current_room_id);
        if (!pickup_in_current_area) {
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

        const float radius = pickup.kind == PickupKind::Sword ? kCavePickupRadius : kPickupRadius;
        if (overlaps_circle(pickup.position, player->position, radius)) {
            apply_pickup(&pickup, player, session);
        }
    }
}

void apply_sword_hits(GameSession* session, Player* player) {
    if (!player->has_sword || !is_sword_active(player) ||
        session->area_kind != AreaKind::Overworld) {
        return;
    }

    const glm::vec2 sword_position = sword_world_position(player);
    for (Enemy& enemy : session->enemies) {
        if (!enemy.active || enemy.room_id != session->current_room_id) {
            continue;
        }

        if (enemy.hurt_seconds_remaining > 0.0F) {
            continue;
        }

        if (!overlaps_circle(enemy.position, sword_position, kSwordHitRadius)) {
            continue;
        }

        enemy.hurt_seconds_remaining = 0.2F;
        enemy.health -= 1;
        if (enemy.health > 0) {
            continue;
        }

        enemy.active = false;
        maybe_drop_pickup(session, &enemy);
        update_enemy_clear_state(session, session->current_room_id);
    }
}

void tick_overworld_session(GameSession* session, const World* overworld_world, Player* player,
                            const PlayerCommand* command, float dt_seconds) {
    PlayerCommand player_command = *command;
    if (!player->has_sword) {
        player_command.attack_pressed = false;
    }

    tick_player(player, overworld_world, &player_command, dt_seconds);

    const int room_id = get_room_id_at_world_tile(static_cast<int>(player->position.x),
                                                  static_cast<int>(player->position.y));
    session->current_room_id = room_id;
    if (session->current_room_id != session->previous_room_id) {
        session->previous_room_id = session->current_room_id;
        enter_overworld_room(session, session->current_room_id);
    }

    apply_sword_hits(session, player);
    tick_enemies(session, overworld_world, player, dt_seconds);
    tick_projectiles(session, overworld_world, player, dt_seconds);
    tick_pickups(session, player, dt_seconds);

    if (session->warp_cooldown_seconds > 0.0F) {
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
    if (warp != nullptr) {
        enter_cave(session, player, warp->cave_id);
    }
}

void tick_cave_session(GameSession* session, Player* player, const PlayerCommand* command,
                       float dt_seconds) {
    PlayerCommand player_command = *command;
    player_command.attack_pressed = false;
    tick_player(player, &session->cave_world, &player_command, dt_seconds);
    tick_pickups(session, player, dt_seconds);

    if (session->warp_cooldown_seconds > 0.0F) {
        return;
    }

    const CaveDef* cave = get_cave_def(session->current_cave_id);
    if (cave == nullptr) {
        return;
    }

    const glm::vec2 delta = player->position - cave->exit_center;
    if (delta.x < -cave->exit_half_size.x || delta.x > cave->exit_half_size.x) {
        return;
    }

    if (delta.y < -cave->exit_half_size.y || delta.y > cave->exit_half_size.y) {
        return;
    }

    exit_cave(session, player);
}

} // namespace

GameSession make_game_session() {
    GameSession session;
    session.cave_world.width = 16;
    session.cave_world.height = 11;
    session.cave_world.overworld.loaded = false;
    return session;
}

void init_game_session(GameSession* session, Player* player) {
    *session = make_game_session();
    *player = make_player();
    player->position = get_opening_start_position();
    player->facing = Facing::Down;
    session->current_room_id = get_opening_start_room_id();
    session->previous_room_id = -1;
    enter_overworld_room(session, session->current_room_id);
}

const World* get_active_world(const GameSession* session, const World* overworld_world) {
    if (is_in_cave(session)) {
        return &session->cave_world;
    }

    return overworld_world;
}

void tick_game_session(GameSession* session, const World* overworld_world, Player* player,
                       const PlayerCommand* command, float dt_seconds) {
    player->invincibility_seconds = glm::max(0.0F, player->invincibility_seconds - dt_seconds);
    session->warp_cooldown_seconds = glm::max(0.0F, session->warp_cooldown_seconds - dt_seconds);

    if (session->area_kind == AreaKind::Cave) {
        tick_cave_session(session, player, command, dt_seconds);
        return;
    }

    tick_overworld_session(session, overworld_world, player, command, dt_seconds);
}

const char* area_name(const GameSession* session) {
    if (session->area_kind == AreaKind::Cave) {
        return "cave";
    }

    return "overworld";
}

const char* pickup_name(PickupKind kind) {
    switch (kind) {
    case PickupKind::Sword:
        return "sword";
    case PickupKind::Heart:
        return "heart";
    case PickupKind::Rupee:
        return "rupee";
    case PickupKind::Bombs:
        return "bombs";
    case PickupKind::None:
        return "none";
    }

    return "none";
}

} // namespace z1m

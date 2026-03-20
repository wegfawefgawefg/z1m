#include "game/play_core.hpp"

namespace z1m {

void choose_cardinal_direction(Play* play, const World* world, Enemy* enemy) {
    const std::array<Facing, 4> facings = {Facing::Up, Facing::Down, Facing::Left, Facing::Right};

    for (int attempt = 0; attempt < 8; ++attempt) {
        const Facing facing = facings[static_cast<std::size_t>(random_int(play, 4))];
        const glm::vec2 probe = enemy->position + facing_vector(facing) * 0.8F;
        if (!world_is_walkable_tile(world, probe)) {
            continue;
        }

        enemy->facing = facing;
        enemy->move_seconds_remaining = 0.30F + random_unit(play) * 0.80F;
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

Projectile& spawn_projectile(Play* play) {
    play->projectiles.push_back(Projectile{});
    Projectile& projectile = play->projectiles.back();
    projectile.active = true;
    return projectile;
}

void make_projectile(Play* play, AreaKind area_kind, int cave_id, ProjectileKind kind,
                     bool from_player, const glm::vec2& position, const glm::vec2& velocity,
                     float seconds_remaining, float radius, int damage) {
    Projectile& projectile = spawn_projectile(play);
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

void throw_enemy_rock(Play* play, const Enemy& enemy, const glm::vec2& velocity) {
    make_projectile(play, enemy.area_kind, enemy.cave_id, ProjectileKind::Rock, false,
                    enemy.position + glm::normalize(velocity) * 0.75F, velocity,
                    kProjectileLifetimeSeconds, kRockRadius, 1);
}

void throw_spread_rocks(Play* play, const Enemy& enemy, const glm::vec2& toward_player) {
    const glm::vec2 base = glm::normalize(toward_player);
    const glm::vec2 left = glm::normalize(base + glm::vec2(-0.35F, -0.15F));
    const glm::vec2 right = glm::normalize(base + glm::vec2(0.35F, -0.15F));
    throw_enemy_rock(play, enemy, left * kRockSpeed);
    throw_enemy_rock(play, enemy, base * kRockSpeed);
    throw_enemy_rock(play, enemy, right * kRockSpeed);
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

void create_bomb(Play* play, const Player* player) {
    make_projectile(play, play->area_kind, play->current_cave_id, ProjectileKind::Bomb, true,
                    player->position + facing_vector(player->facing) * 0.9F, glm::vec2(0.0F),
                    kBombFuseSeconds, kBombRadius, 2);
}

void create_boomerang(Play* play, const Player* player) {
    for (const Projectile& projectile : play->projectiles) {
        if (projectile.active && projectile.from_player &&
            projectile.kind == ProjectileKind::Boomerang &&
            in_area(play, projectile.area_kind, projectile.cave_id)) {
            return;
        }
    }

    Projectile& projectile = spawn_projectile(play);
    projectile.kind = ProjectileKind::Boomerang;
    projectile.area_kind = play->area_kind;
    projectile.cave_id = play->current_cave_id;
    projectile.from_player = true;
    projectile.position = player->position + facing_vector(player->facing) * 0.7F;
    projectile.origin = player->position;
    projectile.velocity = facing_vector(player->facing) * kBoomerangSpeed;
    projectile.seconds_remaining = kBoomerangFlightSeconds;
    projectile.radius = kBoomerangRadius;
    projectile.damage = 1;
}

void create_arrow(Play* play, const Player* player) {
    make_projectile(play, play->area_kind, play->current_cave_id, ProjectileKind::Arrow, true,
                    player->position + facing_vector(player->facing) * 0.9F,
                    facing_vector(player->facing) * kArrowSpeed, kProjectileLifetimeSeconds,
                    kArrowRadius, 1);
}

void create_sword_beam(Play* play, const Enemy& enemy, const glm::vec2& direction) {
    if (glm::length(direction) < 0.001F) {
        return;
    }

    make_projectile(play, enemy.area_kind, enemy.cave_id, ProjectileKind::SwordBeam, false,
                    enemy.position + glm::normalize(direction) * 0.9F,
                    glm::normalize(direction) * kSwordBeamSpeed, kProjectileLifetimeSeconds,
                    kArrowRadius, 1);
}

void create_player_sword_beam(Play* play, Player* player) {
    for (const Projectile& projectile : play->projectiles) {
        if (!projectile.active || !projectile.from_player ||
            projectile.kind != ProjectileKind::SwordBeam ||
            !in_area(play, projectile.area_kind, projectile.cave_id)) {
            continue;
        }

        return;
    }

    const glm::vec2 direction = facing_vector(player->facing);
    make_projectile(play, play->area_kind, play->current_cave_id, ProjectileKind::SwordBeam, true,
                    player->position + direction * 0.9F, direction * kSwordBeamSpeed,
                    kProjectileLifetimeSeconds, kArrowRadius, 1);
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

bool try_trigger_digdogger_split(Play* play) {
    for (Enemy& enemy : play->enemies) {
        if (!enemy.active || !in_area(play, enemy.area_kind, enemy.cave_id) ||
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
            reset_enemy_state(play, &child);
            child.special_counter = 1;
            play->enemies.push_back(child);
        }

        set_message(play, "digdogger split", 1.0F);
        return true;
    }

    return false;
}

void use_recorder(Play* play, const World* overworld_world, Player* player) {
    if (try_trigger_digdogger_split(play)) {
        return;
    }

    if (play->area_kind != AreaKind::Overworld) {
        set_message(play, "recorder only works outside", 1.2F);
        return;
    }

    std::array<OverworldWarp, kScreenCount> recorder_warps = {};
    const int warp_count = gather_recorder_dungeons(overworld_world, &recorder_warps);
    if (warp_count <= 0) {
        set_message(play, "no recorder destination", 1.2F);
        return;
    }

    play->recorder_destination_index =
        (play->recorder_destination_index + 1 + warp_count) % warp_count;
    const OverworldWarp& warp =
        recorder_warps[static_cast<std::size_t>(play->recorder_destination_index)];
    player->position = warp.return_position;
    player->move_direction = MoveDirection::None;
    player->facing = Facing::Down;
    play->warp_cooldown_seconds = kAreaTransitionCooldownSeconds;
    update_current_room(play, player);
    set_message(play, "recorder -> dungeon " + std::to_string(warp.cave_id), 1.4F);
}

void create_fire(Play* play, const Player* player) {
    make_projectile(play, play->area_kind, play->current_cave_id, ProjectileKind::Fire, true,
                    player->position + facing_vector(player->facing) * 0.8F,
                    facing_vector(player->facing) * kFireSpeed, kFireSeconds, kFireRadius, 1);
}

void create_food(Play* play, const Player* player) {
    if (find_active_food(play, play->area_kind, play->current_cave_id) != nullptr) {
        return;
    }

    make_projectile(play, play->area_kind, play->current_cave_id, ProjectileKind::Food, true,
                    player->position + facing_vector(player->facing) * 0.8F, glm::vec2(0.0F),
                    kFoodSeconds, 0.45F, 0);
}

void use_selected_item(Play* play, const World* overworld_world, Player* player) {
    switch (player->selected_item) {
    case UseItemKind::Bombs:
        if (player->bombs <= 0) {
            set_message(play, "out of bombs", 1.2F);
            return;
        }
        player->bombs -= 1;
        create_bomb(play, player);
        set_message(play, "bomb placed", 0.8F);
        break;
    case UseItemKind::Boomerang:
        if (!player->has_boomerang) {
            set_message(play, "need boomerang", 1.0F);
            return;
        }
        create_boomerang(play, player);
        set_message(play, "boomerang thrown", 0.8F);
        break;
    case UseItemKind::Bow:
        if (!player->has_bow || player->rupees <= 0) {
            set_message(play, player->has_bow ? "need rupees for arrows" : "need bow", 1.2F);
            return;
        }
        player->rupees -= 1;
        create_arrow(play, player);
        set_message(play, "arrow fired", 0.8F);
        break;
    case UseItemKind::Candle:
        if (!player->has_candle) {
            set_message(play, "need candle", 1.0F);
            return;
        }
        create_fire(play, player);
        set_message(play, "fire lit", 0.8F);
        break;
    case UseItemKind::Recorder:
        if (!player->has_recorder) {
            set_message(play, "need recorder", 1.0F);
            return;
        }
        use_recorder(play, overworld_world, player);
        break;
    case UseItemKind::Food:
        if (!player->has_food) {
            set_message(play, "need bait", 1.0F);
            return;
        }
        player->has_food = false;
        create_food(play, player);
        if (player->selected_item == UseItemKind::Food) {
            select_next_item(player, 1);
        }
        set_message(play, "bait placed", 1.0F);
        break;
    case UseItemKind::Potion:
        if (!player->has_potion) {
            set_message(play, "need potion", 1.0F);
            return;
        }
        player->has_potion = false;
        player->health = player->max_health;
        if (player->selected_item == UseItemKind::Potion) {
            select_next_item(player, 1);
        }
        set_message(play, "potion used", 1.0F);
        break;
    case UseItemKind::None:
        break;
    }
}

} // namespace z1m

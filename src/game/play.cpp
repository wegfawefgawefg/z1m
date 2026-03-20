#include "game/play_core.hpp"

namespace z1m {

namespace {

void try_enter_overworld_cave(Play* play, const World* overworld_world, Player* player) {
    if (play->area_kind != AreaKind::Overworld || play->warp_cooldown_seconds > 0.0F) {
        return;
    }

    if (player->move_direction != MoveDirection::Up) {
        return;
    }

    std::array<OverworldWarp, kMaxRoomWarps> warps = {};
    int warp_count = 0;
    const OverworldWarp* warp = find_triggered_overworld_warp(
        &overworld_world->overworld, play->current_room_id, player->position, &warps, &warp_count);
    if (warp == nullptr) {
        return;
    }

    play->cave_return_room_id = play->current_room_id;
    play->cave_return_position = warp->return_position;
    const CaveDef* cave = get_cave_def(warp->cave_id);
    if (cave == nullptr) {
        return;
    }

    set_area_kind(play, player, AreaKind::Cave, warp->cave_id, cave->player_spawn);
}

void try_exit_cave(Play* play, Player* player) {
    if (play->area_kind != AreaKind::Cave || play->warp_cooldown_seconds > 0.0F) {
        return;
    }

    const CaveDef* cave = get_cave_def(play->current_cave_id);
    if (cave == nullptr) {
        return;
    }

    const glm::vec2 delta = player->position - cave->exit_center;
    if (std::abs(delta.x) > cave->exit_half_size.x || std::abs(delta.y) > cave->exit_half_size.y) {
        return;
    }

    set_area_kind(play, player, AreaKind::Overworld, -1, play->cave_return_position);
}

void try_area_portals(Play* play, Player* player) {
    if (play->warp_cooldown_seconds > 0.0F) {
        return;
    }

    std::array<AreaPortal, kMaxAreaPortals> portals = {};
    const int portal_count = gather_area_portals(play, &portals);
    for (int index = 0; index < portal_count; ++index) {
        const AreaPortal& portal = portals[static_cast<std::size_t>(index)];
        if (portal.requires_raft && !player->has_raft) {
            const glm::vec2 delta = player->position - portal.center;
            if (std::abs(delta.x) <= portal.half_size.x + 0.7F &&
                std::abs(delta.y) <= portal.half_size.y + 0.7F) {
                set_message(play, "need raft", 0.2F);
            }
            continue;
        }

        const glm::vec2 delta = player->position - portal.center;
        if (std::abs(delta.x) > portal.half_size.x || std::abs(delta.y) > portal.half_size.y) {
            continue;
        }

        set_area_kind(play, player, portal.target_area_kind, portal.target_cave_id,
                      portal.target_position);
        return;
    }
}

void process_player_command(Play* play, const World* overworld_world, Player* player,
                            const PlayerCommand* command) {
    if (command->previous_item_pressed) {
        select_next_item(player, -1);
    }

    if (command->next_item_pressed) {
        select_next_item(player, 1);
    }

    if (command->use_item_pressed) {
        use_selected_item(play, overworld_world, player);
    }
}

void resolve_npc_collisions(const Play* play, Player* player, const glm::vec2& previous_position) {
    for (const Npc& npc : play->npcs) {
        if (!npc.active || npc.solved || !in_area(play, npc.area_kind, npc.cave_id)) {
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

void tick_npcs(Play* play, Player* player, float dt_seconds) {
    for (Npc& npc : play->npcs) {
        if (!npc.active) {
            continue;
        }

        npc.action_seconds_remaining = glm::max(0.0F, npc.action_seconds_remaining - dt_seconds);

        if (npc.kind == NpcKind::Fairy) {
            npc.state_seconds_remaining += dt_seconds;
            const float phase = npc.state_seconds_remaining * 6.0F;
            npc.position = npc.origin + glm::vec2(std::cos(phase) * 0.55F, std::sin(phase) * 0.35F);
            if (in_area(play, npc.area_kind, npc.cave_id) &&
                overlaps_circle(player->position, npc.position, 0.8F)) {
                player->health = player->max_health;
                set_message(play, "fairy healed", 1.0F);
            }
            continue;
        }

        npc.state_seconds_remaining = glm::max(0.0F, npc.state_seconds_remaining - dt_seconds);

        if (npc.kind != NpcKind::HungryGoriya || npc.solved) {
            continue;
        }

        Projectile* food = find_active_food(play, npc.area_kind, npc.cave_id);
        if (food == nullptr || !overlaps_circle(food->position, npc.position, 1.2F)) {
            continue;
        }

        food->active = false;
        npc.solved = true;
        npc.active = false;
        set_message(play, "hungry goriya ate bait", 1.4F);
    }
}

void update_npc_messages(Play* play, const Player* player) {
    if (play->message_seconds_remaining > 0.0F) {
        return;
    }

    for (const Npc& npc : play->npcs) {
        if (!npc.active || !in_area(play, npc.area_kind, npc.cave_id)) {
            continue;
        }

        if (!overlaps_circle(player->position, npc.position, 1.8F)) {
            continue;
        }

        if (npc.kind == NpcKind::ShopKeeper) {
            set_message(play, "shop: touch item to buy", 0.2F);
        } else if (npc.kind == NpcKind::OldWoman) {
            set_message(play, player->has_letter ? "potion shop is open" : "show me the letter",
                        0.2F);
        } else if (npc.kind == NpcKind::HungryGoriya) {
            set_message(play, "hungry goriya needs bait", 0.2F);
        } else if (npc.kind == NpcKind::Fairy) {
            set_message(play, "fairy restores hearts", 0.2F);
        } else {
            set_message(play, npc.label, 0.2F);
        }
        return;
    }
}

} // namespace

void respawn_enemy_group(Play* play, int respawn_group) {
    respawn_enemy_group_internal(play, respawn_group);
}

Play make_play() {
    Play play;
    play.cave_world = make_world();
    resize_world(&play.cave_world, 16, 11);
    fill_world(&play.cave_world, TileKind::Ground);
    fill_world_rect(&play.cave_world, 0, 0, 16, 1, TileKind::Wall);
    fill_world_rect(&play.cave_world, 0, 0, 1, 11, TileKind::Wall);
    fill_world_rect(&play.cave_world, 15, 0, 1, 11, TileKind::Wall);
    fill_world_rect(&play.cave_world, 0, 10, 16, 1, TileKind::Wall);
    fill_world_rect(&play.cave_world, 7, 10, 3, 1, TileKind::Ground);

    play.enemy_zoo_world = make_world();
    play.item_zoo_world = make_world();
    build_enemy_zoo_world(&play.enemy_zoo_world);
    build_item_zoo_world(&play.item_zoo_world);

    play.enemies.reserve(256);
    play.projectiles.reserve(512);
    play.pickups.reserve(256);
    play.npcs.reserve(32);
    return play;
}

void init_play(Play* play, Player* player) {
    *play = make_play();
    *player = make_player();
    player->position = get_opening_start_position();
    player->facing = Facing::Down;
    player->selected_item = UseItemKind::Bombs;
    play->area_kind = AreaKind::Overworld;
    play->current_cave_id = -1;
    init_opening_overworld_enemies(play);
    populate_sandbox_entities(play);
    ensure_sword_cave_pickup(play);
    update_current_room(play, player);
}

const World* get_active_world(const Play* play, const World* overworld_world) {
    switch (play->area_kind) {
    case AreaKind::Overworld:
        return overworld_world;
    case AreaKind::Cave:
        return &play->cave_world;
    case AreaKind::EnemyZoo:
        return &play->enemy_zoo_world;
    case AreaKind::ItemZoo:
        return &play->item_zoo_world;
    }

    return overworld_world;
}

void tick_play(Play* play, const World* overworld_world, Player* player,
               const PlayerCommand* command, float dt_seconds) {
    player->invincibility_seconds = glm::max(0.0F, player->invincibility_seconds - dt_seconds);
    play->warp_cooldown_seconds = glm::max(0.0F, play->warp_cooldown_seconds - dt_seconds);
    play->message_seconds_remaining = glm::max(0.0F, play->message_seconds_remaining - dt_seconds);
    if (play->message_seconds_remaining <= 0.0F) {
        play->message_text.clear();
    }

    PlayerCommand resolved = *command;
    if (!player->has_sword) {
        resolved.attack_pressed = false;
    }
    resolved.ignore_world_collision = play->area_kind == AreaKind::EnemyZoo;

    const World* active_world = get_active_world(play, overworld_world);
    const glm::vec2 previous_player_position = player->position;
    tick_player(player, active_world, &resolved, dt_seconds);
    resolve_npc_collisions(play, player, previous_player_position);
    process_player_command(play, overworld_world, player, &resolved);
    update_current_room(play, player);

    if (resolved.attack_pressed && player->has_sword && player->health == player->max_health) {
        create_player_sword_beam(play, player);
    }

    process_player_attacks(play, player);
    tick_enemies(play, overworld_world, player, dt_seconds);
    tick_enemy_respawns(play, dt_seconds);
    tick_projectiles(play, overworld_world, player, dt_seconds);
    tick_pickups(play, overworld_world, player, dt_seconds);
    tick_npcs(play, player, dt_seconds);

    if (play->area_kind == AreaKind::Overworld) {
        try_enter_overworld_cave(play, overworld_world, player);
    } else if (play->area_kind == AreaKind::Cave) {
        try_exit_cave(play, player);
    } else {
        try_area_portals(play, player);
    }

    update_npc_messages(play, player);
    compact_vectors(play);
    update_current_room(play, player);
}

void set_area_kind(Play* play, Player* player, AreaKind area_kind, int cave_id,
                   const glm::vec2& position) {
    play->area_kind = area_kind;
    play->current_cave_id = area_kind == AreaKind::Cave ? cave_id : -1;
    player->position = position;
    player->move_direction = MoveDirection::None;
    player->sword_cursed = false;
    player->sword_disabled_seconds = 0.0F;
    play->warp_cooldown_seconds = kAreaTransitionCooldownSeconds;
    update_current_room(play, player);
    if (area_kind == AreaKind::Cave) {
        ensure_sword_cave_pickup(play);
    }

    if (area_kind == AreaKind::Cave) {
        set_message(play, "entered cave", 1.0F);
    } else {
        set_message(play, std::string("entered ") + area_name(play), 1.0F);
    }
}

int gather_area_portals(const Play* play, std::array<AreaPortal, kMaxAreaPortals>* portals) {
    return gather_sandbox_portals(play, portals);
}

const char* area_name(const Play* play) {
    switch (play->area_kind) {
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

#include "content/opening_content.hpp"
#include "content/sandbox_content.hpp"
#include "game/area_state.hpp"
#include "game/areas.hpp"
#include "game/combat.hpp"
#include "game/enemy_ticks.hpp"
#include "game/items.hpp"
#include "game/npcs.hpp"
#include "game/pickups.hpp"
#include "game/play.hpp"

#include <glm/common.hpp>

namespace z1m {

namespace {

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

} // namespace z1m

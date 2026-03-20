#include "content/opening_content.hpp"
#include "content/sandbox_content.hpp"
#include "game/area_state.hpp"
#include "game/areas.hpp"
#include "game/combat.hpp"
#include "game/enemies/ticks.hpp"
#include "game/game_state.hpp"
#include "game/items.hpp"
#include "game/npcs.hpp"
#include "game/pickups.hpp"

#include <glm/common.hpp>

namespace z1m {

namespace {

void process_player_command(GameState* game_state, const World* overworld_world, Player* player,
                            const PlayerCommand* command) {
    if (command->previous_item_pressed) {
        select_next_item(player, -1);
    }

    if (command->next_item_pressed) {
        select_next_item(player, 1);
    }

    if (command->use_item_pressed) {
        use_selected_item(game_state, overworld_world, player);
    }
}

} // namespace

void respawn_enemy_group(GameState* game_state, int respawn_group) {
    respawn_enemy_group_internal(game_state, respawn_group);
}

GameState make_game_state() {
    GameState game_state;
    game_state.cave_world = make_world();
    resize_world(&game_state.cave_world, 16, 11);
    fill_world(&game_state.cave_world, TileKind::Ground);
    fill_world_rect(&game_state.cave_world, 0, 0, 16, 1, TileKind::Wall);
    fill_world_rect(&game_state.cave_world, 0, 0, 1, 11, TileKind::Wall);
    fill_world_rect(&game_state.cave_world, 15, 0, 1, 11, TileKind::Wall);
    fill_world_rect(&game_state.cave_world, 0, 10, 16, 1, TileKind::Wall);
    fill_world_rect(&game_state.cave_world, 7, 10, 3, 1, TileKind::Ground);

    game_state.enemy_zoo_world = make_world();
    game_state.item_zoo_world = make_world();
    build_enemy_zoo_world(&game_state.enemy_zoo_world);
    build_item_zoo_world(&game_state.item_zoo_world);

    game_state.enemies.reserve(256);
    game_state.projectiles.reserve(512);
    game_state.pickups.reserve(256);
    game_state.npcs.reserve(32);
    return game_state;
}

void init_game_state(GameState* game_state, Player* player) {
    *game_state = make_game_state();
    *player = make_player();
    player->position = get_opening_start_position();
    player->facing = Facing::Down;
    player->selected_item = UseItemKind::Bombs;
    game_state->area_kind = AreaKind::Overworld;
    game_state->current_cave_id = -1;
    init_opening_overworld_enemies(game_state);
    populate_sandbox_entities(game_state);
    ensure_sword_cave_pickup(game_state);
    update_current_room(game_state, player);
}

void tick_game_state(GameState* game_state, const World* overworld_world, Player* player,
                     const PlayerCommand* command, float dt_seconds) {
    player->invincibility_seconds = glm::max(0.0F, player->invincibility_seconds - dt_seconds);
    game_state->warp_cooldown_seconds =
        glm::max(0.0F, game_state->warp_cooldown_seconds - dt_seconds);
    game_state->message_seconds_remaining =
        glm::max(0.0F, game_state->message_seconds_remaining - dt_seconds);
    if (game_state->message_seconds_remaining <= 0.0F) {
        game_state->message_text.clear();
    }

    PlayerCommand resolved = *command;
    if (!player->has_sword) {
        resolved.attack_pressed = false;
    }
    resolved.ignore_world_collision = game_state->area_kind == AreaKind::EnemyZoo;

    const World* active_world = get_active_world(game_state, overworld_world);
    const glm::vec2 previous_player_position = player->position;
    tick_player(player, active_world, &resolved, dt_seconds);
    resolve_npc_collisions(game_state, player, previous_player_position);
    process_player_command(game_state, overworld_world, player, &resolved);
    update_current_room(game_state, player);

    if (resolved.attack_pressed && player->has_sword && player->health == player->max_health) {
        create_player_sword_beam(game_state, player);
    }

    process_player_attacks(game_state, player);
    tick_enemies(game_state, overworld_world, player, dt_seconds);
    tick_enemy_respawns(game_state, dt_seconds);
    tick_projectiles(game_state, overworld_world, player, dt_seconds);
    tick_pickups(game_state, overworld_world, player, dt_seconds);
    tick_npcs(game_state, player, dt_seconds);

    if (game_state->area_kind == AreaKind::Overworld) {
        try_enter_overworld_cave(game_state, overworld_world, player);
    } else if (game_state->area_kind == AreaKind::Cave) {
        try_exit_cave(game_state, player);
    } else {
        try_area_portals(game_state, player);
    }

    update_npc_messages(game_state, player);
    compact_vectors(game_state);
    update_current_room(game_state, player);
}

} // namespace z1m

#pragma once

#include "game/game_session.hpp"
#include "game/player.hpp"
#include "game/world.hpp"
#include "render/debug_tileset.hpp"

#include <SDL3/SDL.h>

namespace z1m {

struct DebugView {
    bool show_ui = true;
    bool enabled = true;
    bool show_hitboxes = true;
    bool show_collision_tiles = true;
    bool show_interactables = true;
    bool show_labels = true;
};

struct AppState {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool running = true;
    float zoom = 1.0F;
    bool attack_queued = false;
    double fps_timer_seconds = 0.0;
    int frames_this_second = 0;
    int displayed_fps = 0;
    int tick_count = 0;
    int current_room_id = -1;
    DebugView debug_view = {};
    DebugTileset debug_tileset = {};
    World world = make_world();
    GameSession session = make_game_session();
    Player player = make_player();
};

} // namespace z1m

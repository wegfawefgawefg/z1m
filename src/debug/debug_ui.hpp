#pragma once

#include "app/app_state.hpp"

#include <SDL3/SDL.h>

namespace z1m {

bool init_debug_ui(AppState* app);
void shutdown_debug_ui();
void debug_ui_process_event(const SDL_Event* event);
bool debug_ui_wants_keyboard_capture();
void begin_debug_ui_frame();
void render_debug_ui(AppState* app);

} // namespace z1m

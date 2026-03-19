#pragma once

#include "app/app_state.hpp"

namespace z1m {

int run_app(AppState* app);
bool init_app(AppState* app);
void shutdown_app(AppState* app);
void pump_app_events(AppState* app);
void update_app(AppState* app, double dt_seconds);
void render_app(AppState* app);
void update_window_title(AppState* app);

} // namespace z1m

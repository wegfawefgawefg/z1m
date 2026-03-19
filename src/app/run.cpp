#include "app/run.hpp"

#include "app/app_config.hpp"
#include "app/input_state.hpp"
#include "render/scene_renderer.hpp"

#include <SDL3/SDL.h>
#include <cstdlib>
#include <glm/common.hpp>
#include <string>

namespace z1m {

constexpr Uint64 kNanosecondsPerSecond = 1000000000ULL;
constexpr int kWindowX = 480;
constexpr int kWindowY = 270;

void request_i3_floating_window() {
    const char* command = "i3-msg '[title=\"^z1m( \\\\|.*)?$\"] floating enable, resize set 960 "
                          "540, move position center' "
                          "> /dev/null 2>&1";
    std::system(command);
}

bool load_overworld_content(AppState* app) {
    if (load_world_overworld("content/overworld_q1.txt", &app->world)) {
        return true;
    }

    const char* base_path = SDL_GetBasePath();
    if (base_path == nullptr) {
        return false;
    }

    const std::string fallback_path = std::string(base_path) + "../content/overworld_q1.txt";
    return load_world_overworld(fallback_path.c_str(), &app->world);
}

int run_app(AppState* app) {
    if (!init_app(app)) {
        shutdown_app(app);
        return 1;
    }

    const double fixed_dt_seconds = 1.0 / app_config::kSimHz;
    Uint64 previous_counter = SDL_GetTicksNS();
    double accumulator_seconds = 0.0;

    while (app->running) {
        const Uint64 current_counter = SDL_GetTicksNS();
        const Uint64 delta_counter = current_counter - previous_counter;
        previous_counter = current_counter;

        double frame_seconds =
            static_cast<double>(delta_counter) / static_cast<double>(kNanosecondsPerSecond);
        if (frame_seconds > 0.25) {
            frame_seconds = 0.25;
        }

        accumulator_seconds += frame_seconds;
        app->fps_timer_seconds += frame_seconds;

        pump_app_events(app);
        if (!app->running) {
            break;
        }

        while (accumulator_seconds >= fixed_dt_seconds) {
            update_app(app, fixed_dt_seconds);
            accumulator_seconds -= fixed_dt_seconds;
        }

        render_app(app);
        ++app->frames_this_second;

        if (app->fps_timer_seconds >= 1.0) {
            app->displayed_fps = app->frames_this_second;
            app->fps_timer_seconds -= 1.0;
            app->frames_this_second = 0;
            update_window_title(app);
        }
    }

    shutdown_app(app);
    return 0;
}

bool init_app(AppState* app) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    if (!SDL_CreateWindowAndRenderer("z1m", app_config::kWindowWidth, app_config::kWindowHeight,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY,
                                     &app->window, &app->renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer failed: %s", SDL_GetError());
        return false;
    }

    SDL_SetWindowPosition(app->window, kWindowX, kWindowY);
    SDL_SyncWindow(app->window);

    if (!SDL_SetRenderLogicalPresentation(app->renderer, app_config::kWindowWidth,
                                          app_config::kWindowHeight,
                                          SDL_LOGICAL_PRESENTATION_LETTERBOX)) {
        SDL_Log("SDL_SetRenderLogicalPresentation failed: %s", SDL_GetError());
        return false;
    }

    if (!SDL_SetRenderVSync(app->renderer, SDL_RENDERER_VSYNC_DISABLED)) {
        SDL_Log("SDL_SetRenderVSync failed: %s", SDL_GetError());
    }

    if (!load_overworld_content(app)) {
        SDL_Log("Failed to load content/overworld_q1.txt");
    }

    app->player.position = glm::vec2(7.5F * static_cast<float>(kScreenTileWidth),
                                     3.5F * static_cast<float>(kScreenTileHeight));
    app->current_room_id = get_room_id_at_world_tile(static_cast<int>(app->player.position.x),
                                                     static_cast<int>(app->player.position.y));

    request_i3_floating_window();
    update_window_title(app);
    return true;
}

void shutdown_app(AppState* app) {
    if (app->renderer != nullptr) {
        SDL_DestroyRenderer(app->renderer);
        app->renderer = nullptr;
    }

    if (app->window != nullptr) {
        SDL_DestroyWindow(app->window);
        app->window = nullptr;
    }

    SDL_Quit();
}

void pump_app_events(AppState* app) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            app->running = false;
            return;
        }

        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.repeat) {
                continue;
            }

            if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                app->running = false;
                return;
            }

            if (event.key.scancode == SDL_SCANCODE_SPACE || event.key.scancode == SDL_SCANCODE_F) {
                app->attack_queued = true;
            }

            if (event.key.scancode == SDL_SCANCODE_EQUALS ||
                event.key.scancode == SDL_SCANCODE_KP_PLUS) {
                app->zoom = glm::min(app_config::kMaxZoom, app->zoom + 0.1F);
                update_window_title(app);
            }

            if (event.key.scancode == SDL_SCANCODE_MINUS ||
                event.key.scancode == SDL_SCANCODE_KP_MINUS) {
                app->zoom = glm::max(app_config::kMinZoom, app->zoom - 0.1F);
                update_window_title(app);
            }

            if (event.key.scancode == SDL_SCANCODE_0 || event.key.scancode == SDL_SCANCODE_KP_0) {
                app->zoom = app_config::kDefaultZoom;
                update_window_title(app);
            }
        }

        if (event.type == SDL_EVENT_MOUSE_WHEEL) {
            if (event.wheel.y > 0.0F) {
                app->zoom = glm::min(app_config::kMaxZoom, app->zoom + 0.1F);
                update_window_title(app);
            }

            if (event.wheel.y < 0.0F) {
                app->zoom = glm::max(app_config::kMinZoom, app->zoom - 0.1F);
                update_window_title(app);
            }
        }
    }
}

void update_app(AppState* app, double dt_seconds) {
    InputState input;
    const bool* keys = SDL_GetKeyboardState(nullptr);

    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) {
        input.move_axis.y -= 1.0F;
    }

    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) {
        input.move_axis.y += 1.0F;
    }

    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
        input.move_axis.x -= 1.0F;
    }

    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
        input.move_axis.x += 1.0F;
    }

    input.attack_pressed = app->attack_queued;
    app->attack_queued = false;

    PlayerCommand command;
    command.move_axis = input.move_axis;
    command.attack_pressed = input.attack_pressed;

    tick_player(&app->player, &app->world, &command, static_cast<float>(dt_seconds));
    app->current_room_id = get_room_id_at_world_tile(static_cast<int>(app->player.position.x),
                                                     static_cast<int>(app->player.position.y));
    ++app->tick_count;
}

void render_app(AppState* app) {
    render_scene(app->renderer, &app->world, &app->player, app->zoom);
}

void update_window_title(AppState* app) {
    if (app->window == nullptr) {
        return;
    }

    const std::string title =
        "z1m | SDL3 + GLM | 960x540 | fps=" + std::to_string(app->displayed_fps) +
        " | sim=60Hz | room=" + std::to_string(app->current_room_id) +
        " | zoom=" + std::to_string(app->zoom);
    SDL_SetWindowTitle(app->window, title.c_str());
}

} // namespace z1m

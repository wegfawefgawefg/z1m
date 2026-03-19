#include "render/scene_renderer.hpp"

#include "app/app_config.hpp"
#include "content/world_data.hpp"
#include "game/player.hpp"
#include "game/world.hpp"
#include "render/camera.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdint>
#include <glm/vec2.hpp>

namespace z1m {

namespace {

void set_draw_color(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

SDL_FRect make_rect(const glm::vec2& position, const glm::vec2& size) {
    SDL_FRect rect;
    rect.x = position.x;
    rect.y = position.y;
    rect.w = size.x;
    rect.h = size.y;
    return rect;
}

void fill_rect(SDL_Renderer* renderer, const SDL_FRect& rect) {
    SDL_RenderFillRect(renderer, &rect);
}

Uint8 mix_channel(int value, int bias, int mul) {
    return static_cast<Uint8>((value * mul + bias) & 0xFF);
}

bool use_overworld_map_view(const World* world, float zoom) {
    return world->overworld.loaded && zoom <= 1.0F;
}

void set_draw_color_for_overworld_tile(SDL_Renderer* renderer, std::uint8_t tile) {
    if (tile == 0x24 || tile == 0x6F) {
        set_draw_color(renderer, 90, 132, 74);
        return;
    }

    if (tile == 0x74 || tile == 0x75 || tile == 0x76 || tile == 0x77) {
        set_draw_color(renderer, 48, 96, 178);
        return;
    }

    if (tile >= 0xBC && tile <= 0xDE) {
        set_draw_color(renderer, 118, 102, 78);
        return;
    }

    if (tile >= 0xE5 && tile <= 0xEA) {
        set_draw_color(renderer, 34, 94, 42);
        return;
    }

    set_draw_color(renderer, mix_channel(tile, 40, 29), mix_channel(tile, 92, 17),
                   mix_channel(tile, 148, 11));
}

} // namespace

void render_scene(SDL_Renderer* renderer, const World* world, const Player* player, float zoom) {
    set_draw_color(renderer, 16, 24, 34);
    SDL_RenderClear(renderer);

    render_world(renderer, world, player, zoom);
    render_player(renderer, world, player, zoom);

    SDL_RenderPresent(renderer);
}

void render_world(SDL_Renderer* renderer, const World* world, const Player* player, float zoom) {
    if (use_overworld_map_view(world, zoom)) {
        const float fit_x =
            static_cast<float>(app_config::kWindowWidth) / static_cast<float>(kWorldTileWidth);
        const float fit_y =
            static_cast<float>(app_config::kWindowHeight) / static_cast<float>(kWorldTileHeight);
        const float tile_size = std::min(fit_x, fit_y) * zoom;

        for (int tile_y = 0; tile_y < kWorldTileHeight; ++tile_y) {
            for (int tile_x = 0; tile_x < kWorldTileWidth; ++tile_x) {
                const std::uint8_t tile = get_world_tile(&world->overworld, tile_x, tile_y);
                set_draw_color_for_overworld_tile(renderer, tile);

                const SDL_FRect tile_rect =
                    make_rect(glm::vec2(static_cast<float>(tile_x) * tile_size,
                                        static_cast<float>(tile_y) * tile_size),
                              glm::vec2(tile_size, tile_size));
                fill_rect(renderer, tile_rect);
            }
        }

        set_draw_color(renderer, 255, 255, 255, 72);
        for (int screen_x = 0; screen_x <= kScreenColumns; ++screen_x) {
            const float x = static_cast<float>(screen_x * kScreenTileWidth) * tile_size;
            SDL_RenderLine(renderer, x, 0.0F, x, static_cast<float>(kWorldTileHeight) * tile_size);
        }

        for (int screen_y = 0; screen_y <= kScreenRows; ++screen_y) {
            const float y = static_cast<float>(screen_y * kScreenTileHeight) * tile_size;
            SDL_RenderLine(renderer, 0.0F, y, static_cast<float>(kWorldTileWidth) * tile_size, y);
        }

        return;
    }

    const Camera camera = make_camera(player->position,
                                      glm::vec2(static_cast<float>(app_config::kWindowWidth),
                                                static_cast<float>(app_config::kWindowHeight)),
                                      zoom);

    for (int y = 0; y < world_height(world); ++y) {
        for (int x = 0; x < world_width(world); ++x) {
            const TileKind tile = world_tile_at(world, x, y);

            switch (tile) {
            case TileKind::Ground:
                set_draw_color(renderer, 78, 120, 62);
                break;
            case TileKind::Wall:
                set_draw_color(renderer, 110, 90, 72);
                break;
            case TileKind::Water:
                set_draw_color(renderer, 42, 94, 176);
                break;
            case TileKind::Tree:
                set_draw_color(renderer, 36, 96, 44);
                break;
            case TileKind::Rock:
                set_draw_color(renderer, 126, 130, 142);
                break;
            }

            const glm::vec2 world_position =
                glm::vec2(static_cast<float>(x), static_cast<float>(y));
            const SDL_FRect tile_rect = make_rect(world_to_screen(&camera, world_position),
                                                  glm::vec2(camera.pixels_per_world_unit));
            fill_rect(renderer, tile_rect);

            if (tile == TileKind::Ground) {
                set_draw_color(renderer, 88, 136, 70, 150);
                const SDL_FRect inset_rect =
                    make_rect(glm::vec2(tile_rect.x + 2.0F, tile_rect.y + 2.0F),
                              glm::vec2(tile_rect.w - 4.0F, tile_rect.h - 4.0F));
                fill_rect(renderer, inset_rect);
            }
        }
    }
}

void render_player(SDL_Renderer* renderer, const World* world, const Player* player, float zoom) {
    if (use_overworld_map_view(world, zoom)) {
        const float fit_x =
            static_cast<float>(app_config::kWindowWidth) / static_cast<float>(kWorldTileWidth);
        const float fit_y =
            static_cast<float>(app_config::kWindowHeight) / static_cast<float>(kWorldTileHeight);
        const float tile_size = std::min(fit_x, fit_y) * zoom;
        const glm::vec2 player_screen = player->position * tile_size;
        const SDL_FRect player_rect =
            make_rect(player_screen - glm::vec2(2.5F), glm::vec2(5.0F, 5.0F));

        set_draw_color(renderer, 232, 214, 94);
        fill_rect(renderer, player_rect);

        if (is_sword_active(player)) {
            const glm::vec2 sword_screen = sword_world_position(player) * tile_size;
            const SDL_FRect sword_rect =
                make_rect(sword_screen - glm::vec2(1.5F), glm::vec2(3.0F, 3.0F));
            set_draw_color(renderer, 220, 236, 244);
            fill_rect(renderer, sword_rect);
        }

        return;
    }

    const Camera camera = make_camera(player->position,
                                      glm::vec2(static_cast<float>(app_config::kWindowWidth),
                                                static_cast<float>(app_config::kWindowHeight)),
                                      zoom);

    const float player_size = camera.pixels_per_world_unit * 0.65F;
    const glm::vec2 player_screen = world_to_screen(&camera, player->position);
    const SDL_FRect player_rect = make_rect(player_screen - glm::vec2(player_size * 0.5F),
                                            glm::vec2(player_size, player_size));

    set_draw_color(renderer, 232, 214, 94);
    fill_rect(renderer, player_rect);

    const glm::vec2 eye_size(4.0F, 4.0F);
    set_draw_color(renderer, 16, 24, 34);
    fill_rect(renderer, make_rect(player_screen + glm::vec2(-8.0F, -4.0F), eye_size));
    fill_rect(renderer, make_rect(player_screen + glm::vec2(4.0F, -4.0F), eye_size));

    if (!is_sword_active(player)) {
        return;
    }

    const glm::vec2 sword_position = world_to_screen(&camera, sword_world_position(player));
    glm::vec2 sword_size(camera.pixels_per_world_unit * 0.55F,
                         camera.pixels_per_world_unit * 0.16F);
    glm::vec2 sword_origin = sword_position - sword_size * 0.5F;

    switch (player->facing) {
    case Facing::Up:
    case Facing::Down:
        sword_size =
            glm::vec2(camera.pixels_per_world_unit * 0.16F, camera.pixels_per_world_unit * 0.55F);
        sword_origin = sword_position - sword_size * 0.5F;
        break;
    case Facing::Left:
    case Facing::Right:
        break;
    }

    set_draw_color(renderer, 220, 236, 244);
    fill_rect(renderer, make_rect(sword_origin, sword_size));
}

} // namespace z1m

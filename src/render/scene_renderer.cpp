#include "render/scene_renderer.hpp"

#include "app/app_config.hpp"
#include "content/world_data.hpp"
#include "game/player.hpp"
#include "game/world.hpp"
#include "render/camera.hpp"
#include "render/debug_tileset.hpp"

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

void render_tile_texture(SDL_Renderer* renderer, const DebugTileset* tileset, std::uint8_t tile,
                         std::uint8_t palette_selector, const SDL_FRect& dst_rect) {
    const SDL_FRect src_rect = get_debug_tileset_source_rect(tileset, tile);
    SDL_Texture* texture = get_debug_tileset_texture(tileset, palette_selector);
    if (texture == nullptr) {
        return;
    }

    SDL_RenderTexture(renderer, texture, &src_rect, &dst_rect);
}

Uint8 mix_channel(int value, int bias, int mul) {
    return static_cast<Uint8>((value * mul + bias) & 0xFF);
}

bool draw_room_grid(float zoom) {
    return zoom <= 0.12F;
}

void set_draw_color_for_overworld_tile(SDL_Renderer* renderer, std::uint8_t tile) {
    if (tile == 0x6F || (tile >= 0x70 && tile <= 0x73) || tile == 0x8D || tile == 0x91) {
        set_draw_color(renderer, 0, 0, 0);
        return;
    }

    if (tile == 0x74 || tile == 0x75 || tile == 0x76 || tile == 0x77) {
        set_draw_color(renderer, 32, 56, 236);
        return;
    }

    if (tile >= 0xBC && tile <= 0xDE) {
        set_draw_color(renderer, 200, 76, 12);
        return;
    }

    if (tile >= 0xE5 && tile <= 0xEA) {
        set_draw_color(renderer, 0, 168, 0);
        return;
    }

    if ((tile >= 0x78 && tile <= 0x80) || (tile >= 0x84 && tile <= 0x98)) {
        set_draw_color(renderer, 116, 116, 116);
        return;
    }

    if (tile >= 0x99 && tile <= 0xBB) {
        set_draw_color(renderer, 189, 162, 126);
        return;
    }

    if (tile == 0x24 || tile == 0x26) {
        set_draw_color(renderer, 252, 216, 168);
        return;
    }

    set_draw_color(renderer, mix_channel(tile, 210, 3), mix_channel(tile, 174, 5),
                   mix_channel(tile, 132, 7));
}

} // namespace

void render_scene(SDL_Renderer* renderer, const DebugTileset* tileset, const World* world,
                  const Player* player, float zoom) {
    set_draw_color(renderer, 16, 24, 34);
    SDL_RenderClear(renderer);

    render_world(renderer, tileset, world, player, zoom);
    render_player(renderer, world, player, zoom);

    SDL_RenderPresent(renderer);
}

void render_world(SDL_Renderer* renderer, const DebugTileset* tileset, const World* world,
                  const Player* player, float zoom) {
    const Camera camera = make_camera(player->position,
                                      glm::vec2(static_cast<float>(app_config::kWindowWidth),
                                                static_cast<float>(app_config::kWindowHeight)),
                                      zoom);
    Camera clamped_camera = camera;
    clamp_camera_to_world(&clamped_camera, glm::vec2(static_cast<float>(world_width(world)),
                                                     static_cast<float>(world_height(world))));

    for (int y = 0; y < world_height(world); ++y) {
        for (int x = 0; x < world_width(world); ++x) {
            if (world->overworld.loaded) {
                set_draw_color_for_overworld_tile(renderer,
                                                  get_world_tile(&world->overworld, x, y));
            } else {
                const TileKind tile = world_tile_at(world, x, y);

                switch (tile) {
                case TileKind::Ground:
                    set_draw_color(renderer, 252, 216, 168);
                    break;
                case TileKind::Wall:
                    set_draw_color(renderer, 200, 76, 12);
                    break;
                case TileKind::Water:
                    set_draw_color(renderer, 32, 56, 236);
                    break;
                case TileKind::Tree:
                    set_draw_color(renderer, 0, 168, 0);
                    break;
                case TileKind::Rock:
                    set_draw_color(renderer, 116, 116, 116);
                    break;
                }
            }

            const glm::vec2 world_position =
                glm::vec2(static_cast<float>(x), static_cast<float>(y));
            const SDL_FRect tile_rect = make_rect(world_to_screen(&clamped_camera, world_position),
                                                  glm::vec2(clamped_camera.pixels_per_world_unit));
            if (world->overworld.loaded && tileset->loaded) {
                render_tile_texture(renderer, tileset, get_world_tile(&world->overworld, x, y),
                                    get_world_palette_selector(&world->overworld, x, y), tile_rect);
            } else {
                fill_rect(renderer, tile_rect);
            }
        }
    }

    if (!world->overworld.loaded || !draw_room_grid(zoom)) {
        return;
    }

    set_draw_color(renderer, 255, 255, 255, 72);
    for (int screen_x = 0; screen_x <= kScreenColumns; ++screen_x) {
        const float world_x = static_cast<float>(screen_x * kScreenTileWidth);
        const float screen_x0 = world_to_screen(&clamped_camera, glm::vec2(world_x, 0.0F)).x;
        SDL_RenderLine(renderer, screen_x0, 0.0F, screen_x0,
                       static_cast<float>(app_config::kWindowHeight));
    }

    for (int screen_y = 0; screen_y <= kScreenRows; ++screen_y) {
        const float world_y = static_cast<float>(screen_y * kScreenTileHeight);
        const float screen_y0 = world_to_screen(&clamped_camera, glm::vec2(0.0F, world_y)).y;
        SDL_RenderLine(renderer, 0.0F, screen_y0, static_cast<float>(app_config::kWindowWidth),
                       screen_y0);
    }
}

void render_player(SDL_Renderer* renderer, const World* world, const Player* player, float zoom) {
    Camera camera = make_camera(player->position,
                                glm::vec2(static_cast<float>(app_config::kWindowWidth),
                                          static_cast<float>(app_config::kWindowHeight)),
                                zoom);
    clamp_camera_to_world(&camera, glm::vec2(static_cast<float>(world_width(world)),
                                             static_cast<float>(world_height(world))));

    const float player_size = std::max(4.0F, camera.pixels_per_world_unit * 0.65F);
    const glm::vec2 player_screen = world_to_screen(&camera, player->position);
    const SDL_FRect player_rect = make_rect(player_screen - glm::vec2(player_size * 0.5F),
                                            glm::vec2(player_size, player_size));

    set_draw_color(renderer, 252, 252, 252);
    fill_rect(renderer, player_rect);

    if (player_size >= 12.0F) {
        const glm::vec2 eye_size(4.0F, 4.0F);
        set_draw_color(renderer, 0, 0, 0);
        fill_rect(renderer, make_rect(player_screen + glm::vec2(-8.0F, -4.0F), eye_size));
        fill_rect(renderer, make_rect(player_screen + glm::vec2(4.0F, -4.0F), eye_size));
    }

    if (!is_sword_active(player)) {
        return;
    }

    const glm::vec2 sword_position = world_to_screen(&camera, sword_world_position(player));
    glm::vec2 sword_size(std::max(3.0F, camera.pixels_per_world_unit * 0.55F),
                         std::max(2.0F, camera.pixels_per_world_unit * 0.16F));
    glm::vec2 sword_origin = sword_position - sword_size * 0.5F;

    switch (player->facing) {
    case Facing::Up:
    case Facing::Down:
        sword_size = glm::vec2(std::max(2.0F, camera.pixels_per_world_unit * 0.16F),
                               std::max(3.0F, camera.pixels_per_world_unit * 0.55F));
        sword_origin = sword_position - sword_size * 0.5F;
        break;
    case Facing::Left:
    case Facing::Right:
        break;
    }

    set_draw_color(renderer, 252, 252, 252);
    fill_rect(renderer, make_rect(sword_origin, sword_size));
}

} // namespace z1m

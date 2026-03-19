#include "render/scene_renderer.hpp"

#include "app/app_config.hpp"
#include "app/app_state.hpp"
#include "content/opening_content.hpp"
#include "content/overworld_warps.hpp"
#include "content/world_data.hpp"
#include "game/game_session.hpp"
#include "game/player.hpp"
#include "game/world.hpp"
#include "render/camera.hpp"
#include "render/debug_tileset.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <glm/vec2.hpp>
#include <string>

namespace z1m {

namespace {

constexpr float kPlayerDebugHalfWidth = 0.30F;
constexpr float kPlayerDebugHalfHeight = 0.45F;
constexpr float kEnemyDebugRadius = 0.40F;
constexpr float kProjectileDebugRadius = 0.18F;
constexpr float kPickupDebugRadius = 0.30F;
constexpr float kSwordDebugRadius = 0.48F;

void set_draw_color(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

SDL_FRect make_rect(const glm::vec2& position, const glm::vec2& size) {
    return SDL_FRect{position.x, position.y, size.x, size.y};
}

void fill_rect(SDL_Renderer* renderer, const SDL_FRect& rect) {
    SDL_RenderFillRect(renderer, &rect);
}

void draw_rect_outline(SDL_Renderer* renderer, const SDL_FRect& rect) {
    SDL_RenderRect(renderer, &rect);
}

SDL_FRect world_rect_to_screen(const Camera* camera, const glm::vec2& world_center,
                               const glm::vec2& world_size) {
    const glm::vec2 screen_center = world_to_screen(camera, world_center);
    return make_rect(screen_center - world_size * camera->pixels_per_world_unit * 0.5F,
                     world_size * camera->pixels_per_world_unit);
}

void render_tile_texture(SDL_Renderer* renderer, const DebugTileset* tileset, std::uint8_t tile,
                         std::uint8_t palette_selector, const SDL_FRect& dst_rect) {
    SDL_Texture* texture = get_debug_tileset_texture(tileset, palette_selector);
    if (texture == nullptr) {
        return;
    }

    const SDL_FRect src_rect = get_debug_tileset_source_rect(tileset, tile);
    SDL_RenderTexture(renderer, texture, &src_rect, &dst_rect);
}

bool draw_room_grid(float zoom) {
    return zoom <= 0.12F;
}

void set_draw_color_for_overworld_tile(SDL_Renderer* renderer, std::uint8_t tile) {
    if (tile == 0x6F || (tile >= 0x70 && tile <= 0x73) || tile == 0x8D || tile == 0x91) {
        set_draw_color(renderer, 0, 0, 0);
        return;
    }

    if (tile >= 0x74 && tile <= 0x77) {
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

    set_draw_color(renderer, 252, 216, 168);
}

Camera make_clamped_camera(const glm::vec2& focus, const World* world, float zoom) {
    Camera camera = make_camera(focus,
                                glm::vec2(static_cast<float>(app_config::kWindowWidth),
                                          static_cast<float>(app_config::kWindowHeight)),
                                zoom);
    clamp_camera_to_world(&camera, glm::vec2(static_cast<float>(world_width(world)),
                                             static_cast<float>(world_height(world))));
    return camera;
}

Camera make_session_camera(const GameSession* session, const World* world, const Player* player,
                           float zoom) {
    return make_clamped_camera(player->position, get_active_world(session, world), zoom);
}

void render_world_grid(SDL_Renderer* renderer, const Camera* camera) {
    set_draw_color(renderer, 255, 255, 255, 72);
    for (int screen_x = 0; screen_x <= kScreenColumns; ++screen_x) {
        const float world_x = static_cast<float>(screen_x * kScreenTileWidth);
        const float x = world_to_screen(camera, glm::vec2(world_x, 0.0F)).x;
        SDL_RenderLine(renderer, x, 0.0F, x, static_cast<float>(app_config::kWindowHeight));
    }

    for (int screen_y = 0; screen_y <= kScreenRows; ++screen_y) {
        const float world_y = static_cast<float>(screen_y * kScreenTileHeight);
        const float y = world_to_screen(camera, glm::vec2(0.0F, world_y)).y;
        SDL_RenderLine(renderer, 0.0F, y, static_cast<float>(app_config::kWindowWidth), y);
    }
}

void render_pickup_sprite(SDL_Renderer* renderer, const Camera* camera, const Pickup* pickup) {
    SDL_FRect rect = world_rect_to_screen(camera, pickup->position, glm::vec2(0.60F, 0.60F));

    switch (pickup->kind) {
    case PickupKind::Sword:
        set_draw_color(renderer, 252, 252, 252);
        fill_rect(renderer, make_rect(glm::vec2(rect.x + rect.w * 0.45F, rect.y),
                                      glm::vec2(rect.w * 0.10F, rect.h * 0.80F)));
        fill_rect(renderer, make_rect(glm::vec2(rect.x + rect.w * 0.25F, rect.y + rect.h * 0.70F),
                                      glm::vec2(rect.w * 0.50F, rect.h * 0.12F)));
        set_draw_color(renderer, 200, 76, 12);
        fill_rect(renderer, make_rect(glm::vec2(rect.x + rect.w * 0.35F, rect.y + rect.h * 0.82F),
                                      glm::vec2(rect.w * 0.30F, rect.h * 0.12F)));
        break;
    case PickupKind::Heart:
        set_draw_color(renderer, 220, 40, 48);
        fill_rect(renderer, rect);
        break;
    case PickupKind::Rupee:
        set_draw_color(renderer, 32, 56, 236);
        fill_rect(renderer, rect);
        break;
    case PickupKind::Bombs:
        set_draw_color(renderer, 40, 40, 40);
        fill_rect(renderer, rect);
        break;
    case PickupKind::None:
        break;
    }
}

void draw_debug_label(SDL_Renderer* renderer, const glm::vec2& screen_position,
                      const std::string& label) {
    SDL_RenderDebugText(renderer, screen_position.x, screen_position.y, label.c_str());
}

void render_collision_overlay(SDL_Renderer* renderer, const GameSession* session,
                              const World* world, const Player* player, float zoom) {
    const World* active_world = get_active_world(session, world);
    const Camera camera = make_session_camera(session, world, player, zoom);

    if (session->area_kind == AreaKind::Overworld) {
        const int room_id = session->current_room_id;
        const int room_x0 = (room_id % kScreenColumns) * kScreenTileWidth;
        const int room_y0 = (room_id / kScreenColumns) * kScreenTileHeight;

        for (int local_y = 0; local_y < kScreenTileHeight; ++local_y) {
            for (int local_x = 0; local_x < kScreenTileWidth; ++local_x) {
                const int world_x = room_x0 + local_x;
                const int world_y = room_y0 + local_y;
                if (world_is_walkable_tile(active_world, world_x, world_y)) {
                    continue;
                }

                const std::uint8_t tile =
                    get_world_tile(&active_world->overworld, world_x, world_y);
                if (tile >= 0x74 && tile <= 0x77) {
                    set_draw_color(renderer, 64, 160, 255, 220);
                } else {
                    set_draw_color(renderer, 255, 96, 96, 220);
                }

                draw_rect_outline(
                    renderer,
                    make_rect(world_to_screen(&camera, glm::vec2(static_cast<float>(world_x),
                                                                 static_cast<float>(world_y))),
                              glm::vec2(camera.pixels_per_world_unit)));

                if (!tile) {
                    continue;
                }
            }
        }
        return;
    }

    set_draw_color(renderer, 255, 96, 96, 220);
    for (int tile_y = 0; tile_y < active_world->height; ++tile_y) {
        for (int tile_x = 0; tile_x < active_world->width; ++tile_x) {
            if (world_is_walkable_tile(active_world, tile_x, tile_y)) {
                continue;
            }

            draw_rect_outline(
                renderer, make_rect(world_to_screen(&camera, glm::vec2(static_cast<float>(tile_x),
                                                                       static_cast<float>(tile_y))),
                                    glm::vec2(camera.pixels_per_world_unit)));
        }
    }
}

void render_hitbox_overlay(SDL_Renderer* renderer, const GameSession* session, const World* world,
                           const Player* player, float zoom) {
    const Camera camera = make_session_camera(session, world, player, zoom);

    set_draw_color(renderer, 255, 255, 255, 255);
    draw_rect_outline(renderer, world_rect_to_screen(&camera, player->position,
                                                     glm::vec2(kPlayerDebugHalfWidth * 2.0F,
                                                               kPlayerDebugHalfHeight * 2.0F)));

    if (player->has_sword && is_sword_active(player)) {
        set_draw_color(renderer, 255, 232, 96, 255);
        draw_rect_outline(renderer, world_rect_to_screen(&camera, sword_world_position(player),
                                                         glm::vec2(kSwordDebugRadius * 2.0F,
                                                                   kSwordDebugRadius * 2.0F)));
    }

    for (const Enemy& enemy : session->enemies) {
        if (!enemy.active || enemy.room_id != session->current_room_id) {
            continue;
        }

        set_draw_color(renderer, 255, 64, 64, 255);
        draw_rect_outline(renderer, world_rect_to_screen(&camera, enemy.position,
                                                         glm::vec2(kEnemyDebugRadius * 2.0F,
                                                                   kEnemyDebugRadius * 2.0F)));
    }

    for (const Projectile& projectile : session->projectiles) {
        if (!projectile.active || projectile.room_id != session->current_room_id) {
            continue;
        }

        set_draw_color(renderer, 255, 160, 64, 255);
        draw_rect_outline(renderer, world_rect_to_screen(&camera, projectile.position,
                                                         glm::vec2(kProjectileDebugRadius * 2.0F,
                                                                   kProjectileDebugRadius * 2.0F)));
    }

    for (const Pickup& pickup : session->pickups) {
        if (!pickup.active) {
            continue;
        }

        const bool visible =
            (session->area_kind == AreaKind::Overworld &&
             pickup.room_id == session->current_room_id) ||
            (session->area_kind == AreaKind::Cave && pickup.cave_id == session->current_cave_id);
        if (!visible) {
            continue;
        }

        set_draw_color(renderer, 96, 255, 160, 255);
        draw_rect_outline(renderer, world_rect_to_screen(&camera, pickup.position,
                                                         glm::vec2(kPickupDebugRadius * 2.0F,
                                                                   kPickupDebugRadius * 2.0F)));
    }
}

void render_interactable_overlay(SDL_Renderer* renderer, const DebugView* debug_view,
                                 const GameSession* session, const World* world,
                                 const Player* player, float zoom) {
    const Camera camera = make_session_camera(session, world, player, zoom);

    if (session->area_kind == AreaKind::Overworld) {
        std::array<OverworldWarp, kMaxRoomWarps> warps = {};
        const int warp_count =
            gather_overworld_warps(&world->overworld, session->current_room_id, &warps);

        for (int index = 0; index < warp_count; ++index) {
            const OverworldWarp& warp = warps[static_cast<std::size_t>(index)];
            if (warp.visible) {
                set_draw_color(renderer, warp.uses_stairs ? 255 : 64, warp.uses_stairs ? 208 : 255,
                               64, 255);
                draw_rect_outline(renderer, world_rect_to_screen(&camera, warp.trigger_position,
                                                                 warp.trigger_half_size * 2.0F));
                if (debug_view->show_labels) {
                    const glm::vec2 label_pos =
                        world_to_screen(&camera, warp.trigger_position + glm::vec2(-0.45F, -0.85F));
                    draw_debug_label(renderer, label_pos,
                                     std::string(overworld_warp_type_name(warp.type)) +
                                         " cave=" + std::to_string(warp.cave_id) +
                                         " tile=" + std::to_string(static_cast<int>(warp.tile)));
                }
                continue;
            }

            if (debug_view->show_labels) {
                const int room_x = (session->current_room_id % kScreenColumns) * kScreenTileWidth;
                const int room_y = (session->current_room_id / kScreenColumns) * kScreenTileHeight;
                const glm::vec2 label_pos =
                    world_to_screen(&camera, glm::vec2(static_cast<float>(room_x) + 1.0F,
                                                       static_cast<float>(room_y) + 1.0F));
                draw_debug_label(renderer, label_pos,
                                 "hidden cave attr cave=" + std::to_string(warp.cave_id));
            }
        }
    } else {
        const CaveDef* cave = get_cave_def(session->current_cave_id);
        if (cave != nullptr) {
            set_draw_color(renderer, 64, 255, 160, 255);
            draw_rect_outline(renderer, world_rect_to_screen(&camera, cave->exit_center,
                                                             cave->exit_half_size * 2.0F));
            if (debug_view->show_labels) {
                draw_debug_label(renderer, world_to_screen(&camera, cave->exit_center),
                                 "cave exit");
            }
        }
    }

    if (!debug_view->show_labels) {
        return;
    }

    for (const Pickup& pickup : session->pickups) {
        if (!pickup.active) {
            continue;
        }

        const bool visible =
            (session->area_kind == AreaKind::Overworld &&
             pickup.room_id == session->current_room_id) ||
            (session->area_kind == AreaKind::Cave && pickup.cave_id == session->current_cave_id);
        if (!visible) {
            continue;
        }

        draw_debug_label(renderer,
                         world_to_screen(&camera, pickup.position + glm::vec2(-0.3F, -0.7F)),
                         std::string("pickup ") + pickup_name(pickup.kind));
    }
}

} // namespace

void render_scene(SDL_Renderer* renderer, const DebugTileset* tileset, const DebugView* debug_view,
                  const GameSession* session, const World* world, const Player* player,
                  float zoom) {
    set_draw_color(renderer, 16, 24, 34);
    SDL_RenderClear(renderer);

    if (session->area_kind == AreaKind::Cave) {
        render_cave(renderer, session, player, zoom);
    } else {
        render_world(renderer, tileset, world, player, zoom);
    }

    render_session_entities(renderer, session, get_active_world(session, world), player, zoom);
    render_player(renderer, get_active_world(session, world), player, zoom);
    render_debug_overlay(renderer, debug_view, session, world, player, zoom);
}

void render_world(SDL_Renderer* renderer, const DebugTileset* tileset, const World* world,
                  const Player* player, float zoom) {
    const Camera camera = make_clamped_camera(player->position, world, zoom);

    for (int y = 0; y < world_height(world); ++y) {
        for (int x = 0; x < world_width(world); ++x) {
            const SDL_FRect tile_rect = make_rect(
                world_to_screen(&camera, glm::vec2(static_cast<float>(x), static_cast<float>(y))),
                glm::vec2(camera.pixels_per_world_unit));

            if (world->overworld.loaded && tileset->loaded) {
                render_tile_texture(renderer, tileset, get_world_tile(&world->overworld, x, y),
                                    get_world_palette_selector(&world->overworld, x, y), tile_rect);
                continue;
            }

            set_draw_color_for_overworld_tile(renderer, get_world_tile(&world->overworld, x, y));
            fill_rect(renderer, tile_rect);
        }
    }

    if (world->overworld.loaded && draw_room_grid(zoom)) {
        render_world_grid(renderer, &camera);
    }
}

void render_cave(SDL_Renderer* renderer, const GameSession* session, const Player* player,
                 float zoom) {
    const World* cave_world = &session->cave_world;
    const Camera camera = make_clamped_camera(player->position, cave_world, zoom);

    set_draw_color(renderer, 0, 0, 0);
    fill_rect(renderer, SDL_FRect{0.0F, 0.0F, static_cast<float>(app_config::kWindowWidth),
                                  static_cast<float>(app_config::kWindowHeight)});

    set_draw_color(renderer, 60, 44, 28);
    fill_rect(renderer,
              world_rect_to_screen(&camera, glm::vec2(8.0F, 5.5F), glm::vec2(16.0F, 11.0F)));

    set_draw_color(renderer, 252, 216, 168);
    fill_rect(renderer,
              world_rect_to_screen(&camera, glm::vec2(8.0F, 5.6F), glm::vec2(12.0F, 7.6F)));

    set_draw_color(renderer, 0, 0, 0);
    fill_rect(renderer,
              world_rect_to_screen(&camera, glm::vec2(8.0F, 9.7F), glm::vec2(3.0F, 0.9F)));

    set_draw_color(renderer, 200, 76, 12);
    fill_rect(renderer,
              world_rect_to_screen(&camera, glm::vec2(8.0F, 5.2F), glm::vec2(2.0F, 0.8F)));

    set_draw_color(renderer, 252, 188, 60);
    fill_rect(renderer,
              world_rect_to_screen(&camera, glm::vec2(5.2F, 4.6F), glm::vec2(1.2F, 1.4F)));

    set_draw_color(renderer, 255, 255, 255);
    fill_rect(renderer,
              world_rect_to_screen(&camera, glm::vec2(5.2F, 4.0F), glm::vec2(0.35F, 0.35F)));
    fill_rect(renderer,
              world_rect_to_screen(&camera, glm::vec2(5.45F, 4.0F), glm::vec2(0.35F, 0.35F)));
}

void render_session_entities(SDL_Renderer* renderer, const GameSession* session, const World* world,
                             const Player* player, float zoom) {
    const Camera camera = make_clamped_camera(player->position, world, zoom);

    for (const Enemy& enemy : session->enemies) {
        if (!enemy.active || enemy.room_id != session->current_room_id) {
            continue;
        }

        SDL_FRect rect = world_rect_to_screen(&camera, enemy.position, glm::vec2(0.8F, 0.8F));
        if (enemy.hurt_seconds_remaining > 0.0F) {
            set_draw_color(renderer, 252, 252, 252);
        } else {
            set_draw_color(renderer, 216, 40, 0);
        }

        fill_rect(renderer, rect);
        set_draw_color(renderer, 0, 0, 0);
        fill_rect(renderer, make_rect(glm::vec2(rect.x + rect.w * 0.25F, rect.y + rect.h * 0.25F),
                                      glm::vec2(rect.w * 0.18F, rect.h * 0.18F)));
        fill_rect(renderer, make_rect(glm::vec2(rect.x + rect.w * 0.58F, rect.y + rect.h * 0.25F),
                                      glm::vec2(rect.w * 0.18F, rect.h * 0.18F)));
    }

    for (const Projectile& projectile : session->projectiles) {
        if (!projectile.active || projectile.room_id != session->current_room_id) {
            continue;
        }

        set_draw_color(renderer, 252, 252, 252);
        fill_rect(renderer,
                  world_rect_to_screen(&camera, projectile.position, glm::vec2(0.35F, 0.35F)));
    }

    for (const Pickup& pickup : session->pickups) {
        if (!pickup.active) {
            continue;
        }

        const bool visible =
            (session->area_kind == AreaKind::Overworld &&
             pickup.room_id == session->current_room_id) ||
            (session->area_kind == AreaKind::Cave && pickup.cave_id == session->current_cave_id);
        if (!visible) {
            continue;
        }

        render_pickup_sprite(renderer, &camera, &pickup);
    }
}

void render_player(SDL_Renderer* renderer, const World* world, const Player* player, float zoom) {
    const Camera camera = make_clamped_camera(player->position, world, zoom);
    const float player_size = std::max(4.0F, camera.pixels_per_world_unit * 0.65F);
    const glm::vec2 player_screen = world_to_screen(&camera, player->position);
    const SDL_FRect player_rect = make_rect(player_screen - glm::vec2(player_size * 0.5F),
                                            glm::vec2(player_size, player_size));

    if (player->invincibility_seconds > 0.0F) {
        set_draw_color(renderer, 252, 216, 168);
    } else {
        set_draw_color(renderer, 252, 252, 252);
    }

    fill_rect(renderer, player_rect);
    set_draw_color(renderer, 0, 0, 0);
    fill_rect(renderer, make_rect(player_screen + glm::vec2(-8.0F, -4.0F), glm::vec2(4.0F, 4.0F)));
    fill_rect(renderer, make_rect(player_screen + glm::vec2(4.0F, -4.0F), glm::vec2(4.0F, 4.0F)));

    if (!player->has_sword || !is_sword_active(player)) {
        return;
    }

    const glm::vec2 sword_position = world_to_screen(&camera, sword_world_position(player));
    glm::vec2 sword_size(std::max(3.0F, camera.pixels_per_world_unit * 0.55F),
                         std::max(2.0F, camera.pixels_per_world_unit * 0.16F));

    if (player->facing == Facing::Up || player->facing == Facing::Down) {
        sword_size = glm::vec2(std::max(2.0F, camera.pixels_per_world_unit * 0.16F),
                               std::max(3.0F, camera.pixels_per_world_unit * 0.55F));
    }

    set_draw_color(renderer, 252, 252, 252);
    fill_rect(renderer, make_rect(sword_position - sword_size * 0.5F, sword_size));
}

void render_debug_overlay(SDL_Renderer* renderer, const DebugView* debug_view,
                          const GameSession* session, const World* world, const Player* player,
                          float zoom) {
    if (debug_view == nullptr || !debug_view->enabled) {
        return;
    }

    if (debug_view->show_collision_tiles) {
        render_collision_overlay(renderer, session, world, player, zoom);
    }

    if (debug_view->show_hitboxes) {
        render_hitbox_overlay(renderer, session, world, player, zoom);
    }

    if (debug_view->show_interactables) {
        render_interactable_overlay(renderer, debug_view, session, world, player, zoom);
    }
}

} // namespace z1m

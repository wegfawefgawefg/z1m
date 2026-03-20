#include "render/scene_renderer.hpp"

#include "app/app_config.hpp"
#include "app/app_state.hpp"
#include "content/opening_content.hpp"
#include "content/overworld_warps.hpp"
#include "content/world_data.hpp"
#include "game/areas.hpp"
#include "game/entity_names.hpp"
#include "game/game_state.hpp"
#include "game/player.hpp"
#include "game/world.hpp"
#include "render/camera.hpp"
#include "render/debug_tileset.hpp"
#include "render/sprite_assets.hpp"
#include "render/sprite_visuals.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
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
constexpr std::array<std::uint8_t, 4> kLinkHeadTiles = {0x02, 0x06, 0x08, 0x0A};
constexpr std::array<std::uint8_t, 4> kLinkHeadMagicShieldTiles = {0x80, 0x54, 0x60, 0x60};

enum class SpritePairMode {
    Flippable,
    Mirrored,
};

struct SpritePair {
    std::uint8_t left_tile = 0;
    std::uint8_t right_tile = 0;
    std::uint8_t left_attributes = 0;
    std::uint8_t right_attributes = 0;
    bool two_sided = true;
};

bool area_matches(const GameState* play, AreaKind area_kind, int cave_id) {
    if (play->area_kind != area_kind) {
        return false;
    }

    if (area_kind == AreaKind::Cave) {
        return play->current_cave_id == cave_id;
    }

    return true;
}

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

std::uint8_t toggle_frame(float seed) {
    return static_cast<std::uint8_t>(static_cast<int>(std::floor(seed)) & 0x01);
}

std::uint8_t player_walk_frame(const Player* player) {
    if (player->move_direction == MoveDirection::None) {
        return 0;
    }

    return toggle_frame((player->position.x + player->position.y) * 2.0F);
}

std::uint8_t walker_frame_for_facing(Facing facing, std::uint8_t walk_frame, bool* flip_h_out) {
    bool flip_h = false;
    std::uint8_t frame = 0;

    switch (facing) {
    case Facing::Right:
        frame = walk_frame;
        break;
    case Facing::Left:
        frame = walk_frame;
        flip_h = true;
        break;
    case Facing::Down:
        frame = 2;
        flip_h = walk_frame != 0;
        break;
    case Facing::Up:
        frame = 3;
        flip_h = walk_frame != 0;
        break;
    }

    if (flip_h_out != nullptr) {
        *flip_h_out = flip_h;
    }
    return frame;
}

SpritePair build_sprite_pair(std::uint8_t left_tile, std::uint8_t attributes, bool two_sided,
                             SpritePairMode mode, bool flip_h) {
    SpritePair pair;
    pair.left_tile = left_tile;
    pair.right_tile = two_sided ? static_cast<std::uint8_t>(left_tile + 2) : left_tile;
    pair.left_attributes = attributes;
    pair.right_attributes = attributes;
    pair.two_sided = two_sided;

    if (!two_sided) {
        if (flip_h) {
            pair.left_attributes ^= 0x40U;
        }
        return pair;
    }

    if (mode == SpritePairMode::Mirrored) {
        pair.right_tile = left_tile;
        pair.right_attributes ^= 0x40U;
        return pair;
    }

    if (flip_h) {
        std::swap(pair.left_tile, pair.right_tile);
        pair.left_attributes ^= 0x40U;
        pair.right_attributes ^= 0x40U;
    }

    return pair;
}

void patch_link_shield_tiles(const Player* player, SpritePair* pair) {
    if (player == nullptr || pair == nullptr || !pair->two_sided) {
        return;
    }

    if (!player->has_magic_shield) {
        if (player->facing != Facing::Down || pair->left_tile >= 0x0B) {
            return;
        }

        const std::uint8_t original_tile = pair->left_tile;
        pair->left_tile = static_cast<std::uint8_t>(pair->left_tile + 0x50);
        if (original_tile == 0x0A) {
            pair->left_attributes &= 0x0FU;
        }
        return;
    }

    std::uint8_t* tile = &pair->left_tile;
    std::uint8_t* attributes = &pair->left_attributes;
    if (player->facing == Facing::Right) {
        tile = &pair->right_tile;
        attributes = &pair->right_attributes;
    }

    for (std::size_t index = 0; index < kLinkHeadTiles.size(); ++index) {
        if (*tile != kLinkHeadTiles[index]) {
            continue;
        }

        const std::uint8_t original_tile = *tile;
        *tile = kLinkHeadMagicShieldTiles[index];
        if (original_tile == 0x0A) {
            *attributes &= 0x0FU;
        }
        return;
    }
}

std::uint8_t octorok_family_object_type(const Enemy& enemy) {
    switch (enemy.kind) {
    case EnemyKind::Octorok:
        return enemy.subtype > 0 ? 0x0A : enemy.max_health > 1 ? 0x09 : 0x07;
    case EnemyKind::Moblin:
        return enemy.max_health > 1 || enemy.subtype > 0 ? 0x03 : 0x04;
    case EnemyKind::Lynel:
        return enemy.subtype == 0 && enemy.max_health > 0 ? 0x01 : 0x02;
    case EnemyKind::Goriya:
        return enemy.subtype == 0 ? 0x05 : 0x06;
    case EnemyKind::Darknut:
        return enemy.subtype > 0 || enemy.max_health > 3 ? 0x0C : 0x0B;
    default:
        return 0x00;
    }
}

std::uint8_t object_type_for_enemy(const Enemy& enemy) {
    switch (enemy.kind) {
    case EnemyKind::Octorok:
    case EnemyKind::Moblin:
    case EnemyKind::Lynel:
    case EnemyKind::Goriya:
    case EnemyKind::Darknut:
        return octorok_family_object_type(enemy);
    case EnemyKind::Tektite:
        return enemy.subtype > 0 ? 0x0D : 0x0E;
    case EnemyKind::Leever:
        return enemy.subtype == 0 ? 0x10 : 0x0F;
    case EnemyKind::Zora:
        return 0x11;
    case EnemyKind::Vire:
        return 0x12;
    case EnemyKind::Zol:
        return 0x13;
    case EnemyKind::Gel:
        return enemy.subtype > 0 ? 0x15 : 0x14;
    case EnemyKind::PolsVoice:
        return 0x16;
    case EnemyKind::LikeLike:
        return 0x17;
    case EnemyKind::Peahat:
        return 0x1A;
    case EnemyKind::Keese:
        return 0x1B;
    case EnemyKind::Armos:
        return 0x1E;
    case EnemyKind::Ghini:
        return 0x21;
    case EnemyKind::FlyingGhini:
        return 0x22;
    case EnemyKind::BlueWizzrobe:
        return 0x23;
    case EnemyKind::RedWizzrobe:
        return 0x24;
    case EnemyKind::Wallmaster:
        return 0x27;
    case EnemyKind::Rope:
        return 0x28;
    case EnemyKind::Stalfos:
        return 0x2A;
    case EnemyKind::Bubble:
        return static_cast<std::uint8_t>(0x2B + std::clamp(enemy.subtype, 0, 2));
    case EnemyKind::Gibdo:
        return 0x30;
    case EnemyKind::Dodongo:
        return 0x31;
    case EnemyKind::Gohma:
        return enemy.subtype == 0 ? 0x33 : 0x34;
    case EnemyKind::Digdogger:
        return enemy.subtype == 0 ? 0x38 : 0x39;
    case EnemyKind::Moldorm:
        return 0x41;
    case EnemyKind::Aquamentus:
        return 0x3D;
    case EnemyKind::Gleeok:
        return 0x42;
    case EnemyKind::Patra:
        return 0x47;
    case EnemyKind::Trap:
        return 0x49;
    case EnemyKind::Manhandla:
        return 0x3C;
    case EnemyKind::Ganon:
        return 0x3E;
    }

    return 0x00;
}

bool enemy_uses_pair_sprite(EnemyKind kind) {
    switch (kind) {
    case EnemyKind::Gel:
    case EnemyKind::Bubble:
    case EnemyKind::Trap:
        return false;
    default:
        return true;
    }
}

glm::vec2 sprite_size_for_enemy(const Enemy& enemy) {
    switch (enemy.kind) {
    case EnemyKind::Gel:
        return glm::vec2(0.5F, 0.5F);
    case EnemyKind::Bubble:
        return glm::vec2(0.6F, 0.6F);
    case EnemyKind::Trap:
        return glm::vec2(0.7F, 0.7F);
    case EnemyKind::Dodongo:
        return glm::vec2(1.4F, 1.0F);
    case EnemyKind::Digdogger:
        return enemy.subtype == 0 ? glm::vec2(1.8F, 1.8F) : glm::vec2(0.95F, 0.95F);
    case EnemyKind::Manhandla:
        return glm::vec2(1.5F, 1.5F);
    case EnemyKind::Gohma:
        return glm::vec2(1.3F, 1.1F);
    case EnemyKind::Moldorm:
        return enemy.subtype == 0 ? glm::vec2(1.2F, 0.9F) : glm::vec2(0.8F, 0.65F);
    case EnemyKind::Aquamentus:
        return glm::vec2(1.6F, 1.4F);
    case EnemyKind::Gleeok:
        return glm::vec2(1.8F, 1.4F);
    case EnemyKind::Patra:
        return glm::vec2(1.1F, 1.1F);
    case EnemyKind::Ganon:
        return glm::vec2(1.2F, 1.4F);
    default:
        return glm::vec2(0.8F, 0.8F);
    }
}

std::uint8_t frame_for_enemy(const Enemy& enemy, bool* flip_h_out) {
    bool flip_h = false;
    std::uint8_t frame = 0;
    const std::uint8_t walk_frame =
        toggle_frame((enemy.position.x + enemy.position.y + enemy.action_seconds_remaining) * 2.0F);

    switch (enemy.kind) {
    case EnemyKind::Octorok:
        switch (enemy.facing) {
        case Facing::Left:
            frame = static_cast<std::uint8_t>(walk_frame == 0 ? 0 : 3);
            break;
        case Facing::Right:
            frame = static_cast<std::uint8_t>(walk_frame == 0 ? 0 : 3);
            flip_h = true;
            break;
        case Facing::Up:
            frame = static_cast<std::uint8_t>(walk_frame == 0 ? 1 : 4);
            break;
        case Facing::Down:
            frame = static_cast<std::uint8_t>(walk_frame == 0 ? 2 : 5);
            break;
        }
        break;
    case EnemyKind::Moblin:
    case EnemyKind::Lynel:
    case EnemyKind::Goriya:
    case EnemyKind::BlueWizzrobe:
    case EnemyKind::RedWizzrobe:
    case EnemyKind::Stalfos:
    case EnemyKind::Gibdo:
    case EnemyKind::LikeLike:
    case EnemyKind::PolsVoice:
    case EnemyKind::Wallmaster:
    case EnemyKind::Rope:
        frame = walker_frame_for_facing(enemy.facing, walk_frame, &flip_h);
        break;
    case EnemyKind::Darknut:
        switch (enemy.facing) {
        case Facing::Left:
            frame = walk_frame == 0 ? 0 : 3;
            flip_h = true;
            break;
        case Facing::Right:
            frame = walk_frame == 0 ? 0 : 3;
            break;
        case Facing::Down:
            frame = walk_frame == 0 ? 1 : 4;
            break;
        case Facing::Up:
            frame = walk_frame == 0 ? 2 : 5;
            flip_h = walk_frame != 0;
            break;
        }
        break;
    case EnemyKind::Tektite:
    case EnemyKind::Keese:
    case EnemyKind::Ghini:
    case EnemyKind::FlyingGhini:
    case EnemyKind::Peahat:
    case EnemyKind::Vire:
        frame = walk_frame;
        flip_h = enemy.velocity.x > 0.0F;
        break;
    case EnemyKind::Leever:
        frame = walk_frame;
        break;
    case EnemyKind::Armos:
        frame = static_cast<std::uint8_t>(walk_frame * 2);
        if (enemy.facing == Facing::Up) {
            ++frame;
        }
        break;
    case EnemyKind::Zora:
        frame = enemy.hidden ? 0 : static_cast<std::uint8_t>(2 + walk_frame);
        break;
    default:
        frame = walk_frame;
        break;
    }

    if (flip_h_out != nullptr) {
        *flip_h_out = flip_h;
    }
    return frame;
}

void render_debug_sprite_pair(SDL_Renderer* renderer, const DebugTileset* tileset,
                              const SpritePair& pair, const SDL_FRect& dst_rect) {
    SDL_Texture* left_texture = get_debug_sprite_texture(tileset, pair.left_attributes & 0x03);
    if (left_texture == nullptr) {
        return;
    }

    auto flip_mode_for_attributes = [](std::uint8_t attributes) {
        SDL_FlipMode flip_mode = SDL_FLIP_NONE;
        if ((attributes & 0x40U) != 0) {
            flip_mode = static_cast<SDL_FlipMode>(flip_mode | SDL_FLIP_HORIZONTAL);
        }
        if ((attributes & 0x80U) != 0) {
            flip_mode = static_cast<SDL_FlipMode>(flip_mode | SDL_FLIP_VERTICAL);
        }
        return flip_mode;
    };

    const SDL_FRect left_src = get_debug_sprite_source_rect(tileset, pair.left_tile);
    if (!pair.two_sided) {
        SDL_RenderTextureRotated(renderer, left_texture, &left_src, &dst_rect, 0.0, nullptr,
                                 flip_mode_for_attributes(pair.left_attributes));
        return;
    }

    SDL_Texture* right_texture = get_debug_sprite_texture(tileset, pair.right_attributes & 0x03);
    if (right_texture == nullptr) {
        return;
    }

    const SDL_FRect right_src = get_debug_sprite_source_rect(tileset, pair.right_tile);
    SDL_FRect left_dst = dst_rect;
    SDL_FRect right_dst = dst_rect;
    left_dst.w *= 0.5F;
    right_dst.w *= 0.5F;
    right_dst.x += right_dst.w;

    SDL_RenderTextureRotated(renderer, left_texture, &left_src, &left_dst, 0.0, nullptr,
                             flip_mode_for_attributes(pair.left_attributes));
    SDL_RenderTextureRotated(renderer, right_texture, &right_src, &right_dst, 0.0, nullptr,
                             flip_mode_for_attributes(pair.right_attributes));
}

bool render_enemy_rom_sprite(SDL_Renderer* renderer, const DebugTileset* tileset,
                             const Camera* camera, const Enemy* enemy) {
    if (tileset == nullptr || !tileset->loaded) {
        return false;
    }

    const std::uint8_t object_type = object_type_for_enemy(*enemy);
    const std::uint8_t animation_index = static_cast<std::uint8_t>(object_type + 1);
    bool flip_h = false;
    const std::uint8_t frame = frame_for_enemy(*enemy, &flip_h);
    const bool mirrored = enemy->kind == EnemyKind::Octorok &&
                          (enemy->facing == Facing::Up || enemy->facing == Facing::Down);

    std::uint8_t left_tile = 0;
    std::uint8_t attributes = 0;
    if (!lookup_debug_object_frame(animation_index, frame, &left_tile, &attributes)) {
        return false;
    }

    const SpritePair pair =
        build_sprite_pair(left_tile, attributes, enemy_uses_pair_sprite(enemy->kind),
                          mirrored ? SpritePairMode::Mirrored : SpritePairMode::Flippable, flip_h);
    const SDL_FRect dst_rect =
        world_rect_to_screen(camera, enemy->position, sprite_size_for_enemy(*enemy));
    render_debug_sprite_pair(renderer, tileset, pair, dst_rect);
    return true;
}

bool render_player_rom_sprite(SDL_Renderer* renderer, const DebugTileset* tileset, const Camera* camera,
                              const Player* player) {
    if (tileset == nullptr || !tileset->loaded) {
        return false;
    }

    std::uint8_t frame = 2;
    bool flip_h = false;
    const std::uint8_t walk = player_walk_frame(player);
    switch (player->facing) {
    case Facing::Right:
    case Facing::Left:
    case Facing::Down:
    case Facing::Up:
        frame = walker_frame_for_facing(player->facing, walk, &flip_h);
        break;
    }

    std::uint8_t left_tile = 0;
    std::uint8_t attributes = 0;
    if (!lookup_debug_animation_frame(0, frame, &left_tile, &attributes)) {
        return false;
    }

    SpritePair pair =
        build_sprite_pair(left_tile, attributes, true, SpritePairMode::Flippable, flip_h);
    patch_link_shield_tiles(player, &pair);
    const SDL_FRect dst_rect =
        world_rect_to_screen(camera, player->position + glm::vec2(0.0F, -0.04F), glm::vec2(1.0F, 1.0F));
    render_debug_sprite_pair(renderer, tileset, pair, dst_rect);
    return true;
}

bool draw_room_grid(float zoom) {
    return zoom <= 0.12F;
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

Camera make_game_camera(const GameState* play, const World* world, const Player* player,
                        float zoom) {
    return make_clamped_camera(player->position, get_active_world(play, world), zoom);
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
    case PickupKind::Boomerang:
        set_draw_color(renderer, 120, 180, 255);
        fill_rect(renderer, rect);
        break;
    case PickupKind::Bow:
        set_draw_color(renderer, 180, 120, 40);
        fill_rect(renderer, rect);
        break;
    case PickupKind::Candle:
        set_draw_color(renderer, 255, 140, 40);
        fill_rect(renderer, rect);
        break;
    case PickupKind::BluePotion:
        set_draw_color(renderer, 64, 120, 255);
        fill_rect(renderer, rect);
        break;
    case PickupKind::HeartContainer:
        set_draw_color(renderer, 220, 40, 48);
        fill_rect(renderer, rect);
        set_draw_color(renderer, 252, 252, 252);
        draw_rect_outline(renderer, rect);
        break;
    case PickupKind::Key:
        set_draw_color(renderer, 240, 220, 100);
        fill_rect(renderer, rect);
        break;
    case PickupKind::Recorder:
        set_draw_color(renderer, 160, 220, 255);
        fill_rect(renderer, rect);
        break;
    case PickupKind::Ladder:
        set_draw_color(renderer, 180, 140, 80);
        fill_rect(renderer, rect);
        break;
    case PickupKind::Raft:
        set_draw_color(renderer, 170, 110, 70);
        fill_rect(renderer, rect);
        break;
    case PickupKind::Food:
        set_draw_color(renderer, 172, 92, 48);
        fill_rect(renderer, rect);
        break;
    case PickupKind::Letter:
        set_draw_color(renderer, 248, 248, 248);
        fill_rect(renderer, rect);
        set_draw_color(renderer, 196, 48, 48);
        draw_rect_outline(renderer, rect);
        break;
    case PickupKind::MagicShield:
        set_draw_color(renderer, 96, 160, 255);
        fill_rect(renderer, rect);
        set_draw_color(renderer, 252, 252, 252);
        draw_rect_outline(renderer, rect);
        break;
    case PickupKind::SilverArrows:
        set_draw_color(renderer, 220, 220, 220);
        fill_rect(renderer, rect);
        break;
    case PickupKind::None:
        break;
    }
}

void render_enemy_sprite(SDL_Renderer* renderer, const DebugTileset* tileset,
                         const SpriteAssets* sprite_assets, const Camera* camera,
                         const Enemy* enemy) {
    if (render_enemy_asset_sprite(renderer, sprite_assets, camera, enemy)) {
        return;
    }

    if (render_enemy_rom_sprite(renderer, tileset, camera, enemy)) {
        return;
    }

    SDL_FRect rect = world_rect_to_screen(camera, enemy->position, glm::vec2(0.8F, 0.8F));

    switch (enemy->kind) {
    case EnemyKind::Octorok:
        set_draw_color(renderer, enemy->hurt_seconds_remaining > 0.0F ? 252 : 216,
                       enemy->hurt_seconds_remaining > 0.0F ? 252 : 40, 0);
        break;
    case EnemyKind::Moblin:
        set_draw_color(renderer, 200, 80, 24);
        break;
    case EnemyKind::Lynel:
        set_draw_color(renderer, 232, 120, 56);
        break;
    case EnemyKind::Goriya:
        set_draw_color(renderer, 84, 168, 216);
        break;
    case EnemyKind::Darknut:
        set_draw_color(renderer, 88, 88, 140);
        break;
    case EnemyKind::Tektite:
        set_draw_color(renderer, 160, 0, 200);
        break;
    case EnemyKind::Leever:
        set_draw_color(renderer, 210, 180, 96);
        break;
    case EnemyKind::Keese:
        set_draw_color(renderer, 96, 96, 140);
        break;
    case EnemyKind::Zol:
        set_draw_color(renderer, 44, 164, 120);
        break;
    case EnemyKind::Gel:
        set_draw_color(renderer, 72, 188, 152);
        rect = world_rect_to_screen(camera, enemy->position, glm::vec2(0.5F, 0.5F));
        break;
    case EnemyKind::Rope:
        set_draw_color(renderer, 176, 96, 48);
        break;
    case EnemyKind::Vire:
        set_draw_color(renderer, 180, 52, 140);
        break;
    case EnemyKind::Stalfos:
        set_draw_color(renderer, 212, 212, 212);
        break;
    case EnemyKind::Gibdo:
        set_draw_color(renderer, 184, 160, 132);
        break;
    case EnemyKind::LikeLike:
        set_draw_color(renderer, 188, 132, 60);
        break;
    case EnemyKind::PolsVoice:
        set_draw_color(renderer, 236, 236, 236);
        break;
    case EnemyKind::BlueWizzrobe:
        set_draw_color(renderer, 72, 88, 224);
        break;
    case EnemyKind::RedWizzrobe:
        set_draw_color(renderer, 216, 72, 88);
        break;
    case EnemyKind::Wallmaster:
        set_draw_color(renderer, 188, 148, 96);
        break;
    case EnemyKind::Ghini:
        set_draw_color(renderer, 220, 220, 255);
        break;
    case EnemyKind::FlyingGhini:
        set_draw_color(renderer, 188, 220, 255);
        break;
    case EnemyKind::Bubble:
        if (enemy->subtype == 0) {
            set_draw_color(renderer, 255, 128, 180);
        } else if (enemy->subtype == 1) {
            set_draw_color(renderer, 96, 128, 255);
        } else {
            set_draw_color(renderer, 255, 96, 96);
        }
        rect = world_rect_to_screen(camera, enemy->position, glm::vec2(0.6F, 0.6F));
        break;
    case EnemyKind::Trap:
        set_draw_color(renderer, 184, 184, 184);
        rect = world_rect_to_screen(camera, enemy->position, glm::vec2(0.7F, 0.7F));
        break;
    case EnemyKind::Armos:
        set_draw_color(renderer, 168, 112, 96);
        break;
    case EnemyKind::Zora:
        set_draw_color(renderer, 72, 168, 236);
        break;
    case EnemyKind::Peahat:
        if (enemy->invulnerable) {
            set_draw_color(renderer, 48, 176, 72);
        } else {
            set_draw_color(renderer, 216, 216, 96);
        }
        break;
    case EnemyKind::Dodongo:
        if (enemy->special_counter > 0) {
            set_draw_color(renderer, 224, 168, 84);
        } else {
            set_draw_color(renderer, 200, 120, 40);
        }
        rect = world_rect_to_screen(camera, enemy->position, glm::vec2(1.4F, 1.0F));
        break;
    case EnemyKind::Digdogger:
        set_draw_color(renderer, enemy->special_counter == 0 ? 88 : 160, 40, 176);
        rect = world_rect_to_screen(camera, enemy->position,
                                    enemy->special_counter == 0 ? glm::vec2(1.8F, 1.8F)
                                                                : glm::vec2(0.95F, 0.95F));
        break;
    case EnemyKind::Manhandla:
        set_draw_color(renderer, 44, 168, 64);
        rect = world_rect_to_screen(camera, enemy->position, glm::vec2(1.5F, 1.5F));
        break;
    case EnemyKind::Gohma:
        if (enemy->special_counter == 0) {
            set_draw_color(renderer, 120, 40, 40);
        } else if (enemy->subtype == 1) {
            set_draw_color(renderer, 96, 128, 224);
        } else {
            set_draw_color(renderer, 224, 64, 64);
        }
        rect = world_rect_to_screen(camera, enemy->position, glm::vec2(1.3F, 1.1F));
        break;
    case EnemyKind::Moldorm:
        if (enemy->subtype == 0) {
            set_draw_color(renderer, 216, 168, 88);
            rect = world_rect_to_screen(camera, enemy->position, glm::vec2(1.2F, 0.9F));
        } else {
            set_draw_color(renderer, 192, 148, 72);
            rect = world_rect_to_screen(camera, enemy->position, glm::vec2(0.8F, 0.65F));
        }
        break;
    case EnemyKind::Aquamentus:
        set_draw_color(renderer, 40, 170, 150);
        rect = world_rect_to_screen(camera, enemy->position, glm::vec2(1.6F, 1.4F));
        break;
    case EnemyKind::Gleeok:
        set_draw_color(renderer, 120, 168, 56);
        rect = world_rect_to_screen(camera, enemy->position, glm::vec2(1.8F, 1.4F));
        break;
    case EnemyKind::Patra:
        set_draw_color(renderer, 176, 40, 152);
        rect = world_rect_to_screen(camera, enemy->position, glm::vec2(1.1F, 1.1F));
        break;
    case EnemyKind::Ganon:
        set_draw_color(renderer, enemy->special_counter > 0 ? 184 : 88, 72, 120);
        rect = world_rect_to_screen(camera, enemy->position, glm::vec2(1.2F, 1.4F));
        break;
    }

    fill_rect(renderer, rect);

    if (enemy->kind == EnemyKind::Manhandla) {
        const std::array<glm::vec2, 4> petals = {glm::vec2(0.9F, 0.0F), glm::vec2(-0.9F, 0.0F),
                                                 glm::vec2(0.0F, 0.9F), glm::vec2(0.0F, -0.9F)};
        set_draw_color(renderer, 72, 196, 88);
        for (const glm::vec2& offset : petals) {
            fill_rect(renderer, world_rect_to_screen(camera, enemy->position + offset,
                                                     glm::vec2(0.55F, 0.55F)));
        }
    }

    if (enemy->kind == EnemyKind::Gleeok) {
        set_draw_color(renderer, 168, 220, 96);
        const int head_count = std::max(enemy->special_counter, 1);
        for (int head = 0; head < head_count; ++head) {
            const float x_offset =
                (static_cast<float>(head) - (static_cast<float>(head_count) - 1.0F) * 0.5F) * 0.8F;
            fill_rect(renderer,
                      world_rect_to_screen(camera, enemy->position + glm::vec2(x_offset, -0.9F),
                                           glm::vec2(0.55F, 0.55F)));
        }
    }

    if (enemy->kind == EnemyKind::Patra) {
        set_draw_color(renderer, 240, 220, 96);
        const float radius = enemy->state_seconds_remaining > 2.0F ? 2.3F : 1.25F;
        const float phase = enemy->action_seconds_remaining * 3.2F;
        for (int orbiter = 0; orbiter < enemy->special_counter; ++orbiter) {
            const float angle = phase + static_cast<float>(orbiter) * (6.28318530718F / 8.0F);
            const glm::vec2 offset(std::cos(angle) * radius, std::sin(angle) * radius);
            fill_rect(renderer, world_rect_to_screen(camera, enemy->position + offset,
                                                     glm::vec2(0.38F, 0.38F)));
        }
    }
}

void render_projectile_sprite(SDL_Renderer* renderer, const Camera* camera,
                              const Projectile* projectile) {
    SDL_FRect rect =
        world_rect_to_screen(camera, projectile->position,
                             glm::vec2(projectile->radius * 2.0F, projectile->radius * 2.0F));

    switch (projectile->kind) {
    case ProjectileKind::Rock:
        set_draw_color(renderer, 220, 220, 220);
        break;
    case ProjectileKind::Arrow:
        set_draw_color(renderer, 252, 252, 252);
        break;
    case ProjectileKind::SwordBeam:
        set_draw_color(renderer, 120, 240, 255);
        rect = world_rect_to_screen(camera, projectile->position, glm::vec2(0.36F, 0.36F));
        break;
    case ProjectileKind::Boomerang:
        set_draw_color(renderer, 120, 180, 255);
        break;
    case ProjectileKind::Fire:
        set_draw_color(renderer, 255, 132, 0);
        break;
    case ProjectileKind::Food:
        set_draw_color(renderer, 172, 92, 48);
        rect = world_rect_to_screen(camera, projectile->position, glm::vec2(0.7F, 0.55F));
        break;
    case ProjectileKind::Bomb:
        set_draw_color(renderer, 30, 30, 30);
        break;
    case ProjectileKind::Explosion:
        set_draw_color(renderer, 255, 208, 80, 220);
        rect = world_rect_to_screen(camera, projectile->position, glm::vec2(2.2F, 2.2F));
        break;
    }

    fill_rect(renderer, rect);
}

void render_npc_sprite(SDL_Renderer* renderer, const Camera* camera, const Npc* npc) {
    SDL_FRect rect = world_rect_to_screen(camera, npc->position, glm::vec2(1.0F, 1.2F));
    if (npc->kind == NpcKind::ShopKeeper) {
        set_draw_color(renderer, 80, 200, 120);
    } else if (npc->kind == NpcKind::OldWoman) {
        set_draw_color(renderer, 200, 120, 200);
    } else if (npc->kind == NpcKind::HungryGoriya) {
        set_draw_color(renderer, 88, 132, 216);
    } else if (npc->kind == NpcKind::Fairy) {
        set_draw_color(renderer, 255, 180, 220);
        rect = world_rect_to_screen(camera, npc->position, glm::vec2(0.7F, 0.7F));
    } else {
        set_draw_color(renderer, 220, 160, 60);
    }

    fill_rect(renderer, rect);
}

void render_portals(SDL_Renderer* renderer, const Camera* camera, const GameState* play) {
    std::array<AreaPortal, kMaxAreaPortals> portals = {};
    const int portal_count = gather_area_portals(play, &portals);
    for (int index = 0; index < portal_count; ++index) {
        const AreaPortal& portal = portals[static_cast<std::size_t>(index)];
        if (portal.requires_raft) {
            set_draw_color(renderer, 170, 110, 70, 255);
        } else {
            set_draw_color(renderer, 80, 180, 255, 255);
        }
        fill_rect(renderer, world_rect_to_screen(camera, portal.center, portal.half_size * 2.0F));
    }
}

void set_draw_color_for_tile_kind(SDL_Renderer* renderer, TileKind tile_kind) {
    switch (tile_kind) {
    case TileKind::Ground:
        set_draw_color(renderer, 70, 64, 54);
        break;
    case TileKind::Wall:
        set_draw_color(renderer, 44, 44, 52);
        break;
    case TileKind::Water:
        set_draw_color(renderer, 40, 96, 220);
        break;
    case TileKind::Tree:
        set_draw_color(renderer, 36, 144, 48);
        break;
    case TileKind::Rock:
        set_draw_color(renderer, 124, 112, 92);
        break;
    }
}

void draw_debug_label(SDL_Renderer* renderer, const glm::vec2& screen_position,
                      const std::string& label) {
    SDL_RenderDebugText(renderer, screen_position.x, screen_position.y, label.c_str());
}

void render_message_box(SDL_Renderer* renderer, const GameState* play) {
    if (play->message_text.empty()) {
        return;
    }

    const SDL_FRect box = SDL_FRect{24.0F, static_cast<float>(app_config::kWindowHeight) - 44.0F,
                                    static_cast<float>(app_config::kWindowWidth) - 48.0F, 28.0F};
    set_draw_color(renderer, 8, 12, 18, 220);
    fill_rect(renderer, box);
    set_draw_color(renderer, 255, 255, 255, 255);
    draw_rect_outline(renderer, box);
    draw_debug_label(renderer, glm::vec2(box.x + 8.0F, box.y + 9.0F), play->message_text);
}

void render_collision_overlay(SDL_Renderer* renderer, const GameState* play, const World* world,
                              const Player* player, float zoom) {
    const World* active_world = get_active_world(play, world);
    const Camera camera = make_game_camera(play, world, player, zoom);

    if (play->area_kind == AreaKind::Overworld) {
        const int room_id = play->current_room_id;
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

void render_hitbox_overlay(SDL_Renderer* renderer, const GameState* play, const World* world,
                           const Player* player, float zoom) {
    const Camera camera = make_game_camera(play, world, player, zoom);

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

    for (const Enemy& enemy : play->enemies) {
        if (!enemy.active || enemy.hidden || !area_matches(play, enemy.area_kind, enemy.cave_id)) {
            continue;
        }

        set_draw_color(renderer, 255, 64, 64, 255);
        draw_rect_outline(renderer, world_rect_to_screen(&camera, enemy.position,
                                                         glm::vec2(kEnemyDebugRadius * 2.0F,
                                                                   kEnemyDebugRadius * 2.0F)));
    }

    for (const Projectile& projectile : play->projectiles) {
        if (!projectile.active || !area_matches(play, projectile.area_kind, projectile.cave_id)) {
            continue;
        }

        set_draw_color(renderer, 255, 160, 64, 255);
        draw_rect_outline(renderer, world_rect_to_screen(&camera, projectile.position,
                                                         glm::vec2(kProjectileDebugRadius * 2.0F,
                                                                   kProjectileDebugRadius * 2.0F)));
    }

    for (const Pickup& pickup : play->pickups) {
        if (!pickup.active || !area_matches(play, pickup.area_kind, pickup.cave_id)) {
            continue;
        }

        set_draw_color(renderer, 96, 255, 160, 255);
        draw_rect_outline(renderer, world_rect_to_screen(&camera, pickup.position,
                                                         glm::vec2(kPickupDebugRadius * 2.0F,
                                                                   kPickupDebugRadius * 2.0F)));
    }
}

void render_interactable_overlay(SDL_Renderer* renderer, const DebugView* debug_view,
                                 const GameState* play, const World* world, const Player* player,
                                 float zoom) {
    const Camera camera = make_game_camera(play, world, player, zoom);

    if (play->area_kind == AreaKind::Overworld) {
        std::array<OverworldWarp, kMaxRoomWarps> warps = {};
        const int warp_count =
            gather_overworld_warps(&world->overworld, play->current_room_id, &warps);

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
                const int room_x = (play->current_room_id % kScreenColumns) * kScreenTileWidth;
                const int room_y = (play->current_room_id / kScreenColumns) * kScreenTileHeight;
                const glm::vec2 label_pos =
                    world_to_screen(&camera, glm::vec2(static_cast<float>(room_x) + 1.0F,
                                                       static_cast<float>(room_y) + 1.0F));
                draw_debug_label(renderer, label_pos,
                                 "hidden cave attr cave=" + std::to_string(warp.cave_id));
            }
        }
    } else if (play->area_kind == AreaKind::Cave) {
        const CaveDef* cave = get_cave_def(play->current_cave_id);
        if (cave != nullptr) {
            set_draw_color(renderer, 64, 255, 160, 255);
            draw_rect_outline(renderer, world_rect_to_screen(&camera, cave->exit_center,
                                                             cave->exit_half_size * 2.0F));
            if (debug_view->show_labels) {
                draw_debug_label(renderer, world_to_screen(&camera, cave->exit_center),
                                 "cave exit");
            }
        }
    } else {
        std::array<AreaPortal, kMaxAreaPortals> portals = {};
        const int portal_count = gather_area_portals(play, &portals);
        for (int index = 0; index < portal_count; ++index) {
            const AreaPortal& portal = portals[static_cast<std::size_t>(index)];
            if (portal.requires_raft) {
                set_draw_color(renderer, 180, 128, 80, 255);
            } else {
                set_draw_color(renderer, 96, 196, 255, 255);
            }
            draw_rect_outline(
                renderer, world_rect_to_screen(&camera, portal.center, portal.half_size * 2.0F));
            if (debug_view->show_labels) {
                std::string label = portal.label;
                if (portal.requires_raft) {
                    label += " (raft)";
                }
                draw_debug_label(renderer, world_to_screen(&camera, portal.center), label);
            }
        }
    }

    if (!debug_view->show_labels) {
        return;
    }

    for (const Pickup& pickup : play->pickups) {
        if (!pickup.active || !area_matches(play, pickup.area_kind, pickup.cave_id)) {
            continue;
        }
        std::string label = std::string("pickup ") + pickup_name(pickup.kind);
        if (pickup.shop_item) {
            label += " " + std::to_string(pickup.price_rupees) + "r";
        }
        draw_debug_label(
            renderer, world_to_screen(&camera, pickup.position + glm::vec2(-0.3F, -0.7F)), label);
    }

    for (const Npc& npc : play->npcs) {
        if (!npc.active || !area_matches(play, npc.area_kind, npc.cave_id)) {
            continue;
        }

        draw_debug_label(renderer, world_to_screen(&camera, npc.position + glm::vec2(-0.5F, -0.8F)),
                         std::string(npc_name(npc.kind)) + " " + npc.label);
    }

    for (const Enemy& enemy : play->enemies) {
        if (!enemy.active || enemy.hidden || !area_matches(play, enemy.area_kind, enemy.cave_id)) {
            continue;
        }

        std::string label = enemy_name(enemy.kind);
        if (enemy.area_kind == AreaKind::EnemyZoo && enemy.respawn_group >= 0) {
            label += " g=" + std::to_string(enemy.respawn_group);
        }
        if (enemy.kind == EnemyKind::Gohma) {
            label += enemy.subtype == 0 ? " red" : " blue";
            label += enemy.special_counter == 0 ? " eye=closed" : " eye=open";
        }
        if (enemy.kind == EnemyKind::Bubble) {
            label += enemy.subtype == 0 ? " flashing" : enemy.subtype == 1 ? " blue" : " red";
        }
        if (enemy.kind == EnemyKind::Dodongo && enemy.special_counter > 0) {
            label += " bombed";
        }
        if (enemy.kind == EnemyKind::Moldorm) {
            label += " seg=" + std::to_string(enemy.subtype);
        }
        if (enemy.kind == EnemyKind::Patra) {
            label += " orbiters=" + std::to_string(enemy.special_counter);
        }
        if (enemy.kind == EnemyKind::Gleeok) {
            label += " heads=" + std::to_string(std::max(enemy.special_counter, 1));
        }
        if (enemy.kind == EnemyKind::Ganon && enemy.special_counter > 0) {
            label += " revealed";
        }
        draw_debug_label(renderer,
                         world_to_screen(&camera, enemy.position + glm::vec2(-0.6F, -0.9F)), label);
    }
}

} // namespace

void render_scene(SDL_Renderer* renderer, const DebugTileset* tileset,
                  const SpriteAssets* sprite_assets, const DebugView* debug_view,
                  const GameState* play, const World* world, const Player* player, float zoom) {
    set_draw_color(renderer, 16, 24, 34);
    SDL_RenderClear(renderer);

    const World* active_world = get_active_world(play, world);
    if (play->area_kind == AreaKind::Cave) {
        render_cave(renderer, play, player, zoom);
    } else {
        render_world(renderer, tileset, active_world, player, zoom);
        const Camera camera = make_clamped_camera(player->position, active_world, zoom);
        render_portals(renderer, &camera, play);
    }

    render_game_entities(renderer, tileset, sprite_assets, play, active_world, player, zoom);
    render_player(renderer, tileset, sprite_assets, active_world, player, zoom);
    render_debug_overlay(renderer, debug_view, play, world, player, zoom);
    render_message_box(renderer, play);
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

            set_draw_color_for_tile_kind(renderer, world_tile_at(world, x, y));
            fill_rect(renderer, tile_rect);
        }
    }

    if (world->overworld.loaded && draw_room_grid(zoom)) {
        render_world_grid(renderer, &camera);
    }
}

void render_cave(SDL_Renderer* renderer, const GameState* play, const Player* player, float zoom) {
    const World* cave_world = &play->cave_world;
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

void render_game_entities(SDL_Renderer* renderer, const DebugTileset* tileset,
                          const SpriteAssets* sprite_assets, const GameState* play,
                          const World* world, const Player* player, float zoom) {
    const Camera camera = make_clamped_camera(player->position, world, zoom);

    for (const Enemy& enemy : play->enemies) {
        if (!enemy.active || enemy.hidden || !area_matches(play, enemy.area_kind, enemy.cave_id)) {
            continue;
        }

        render_enemy_sprite(renderer, tileset, sprite_assets, &camera, &enemy);
    }

    for (const Projectile& projectile : play->projectiles) {
        if (!projectile.active || !area_matches(play, projectile.area_kind, projectile.cave_id)) {
            continue;
        }

        render_projectile_sprite(renderer, &camera, &projectile);
    }

    for (const Pickup& pickup : play->pickups) {
        if (!pickup.active || !area_matches(play, pickup.area_kind, pickup.cave_id)) {
            continue;
        }

        render_pickup_sprite(renderer, &camera, &pickup);
    }

    for (const Npc& npc : play->npcs) {
        if (!npc.active || !area_matches(play, npc.area_kind, npc.cave_id)) {
            continue;
        }

        render_npc_sprite(renderer, &camera, &npc);
    }
}

void render_player(SDL_Renderer* renderer, const DebugTileset* tileset,
                   const SpriteAssets* sprite_assets, const World* world, const Player* player,
                   float zoom) {
    const Camera camera = make_clamped_camera(player->position, world, zoom);
    if (render_player_asset_sprite(renderer, sprite_assets, &camera, player) ||
        render_player_rom_sprite(renderer, tileset, &camera, player)) {
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
        return;
    }

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
                          const GameState* play, const World* world, const Player* player,
                          float zoom) {
    if (debug_view == nullptr || !debug_view->enabled) {
        return;
    }

    if (debug_view->show_collision_tiles) {
        render_collision_overlay(renderer, play, world, player, zoom);
    }

    if (debug_view->show_hitboxes) {
        render_hitbox_overlay(renderer, play, world, player, zoom);
    }

    if (debug_view->show_interactables) {
        render_interactable_overlay(renderer, debug_view, play, world, player, zoom);
    }
}

} // namespace z1m

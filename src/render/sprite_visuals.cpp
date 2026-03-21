#include "render/sprite_visuals.hpp"

#include "game/entities.hpp"
#include "game/player.hpp"
#include "render/camera.hpp"
#include "render/sprite_assets.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <glm/vec2.hpp>

namespace z1m {

namespace {

SDL_FRect world_rect_to_screen(const Camera* camera, const glm::vec2& world_center,
                               const glm::vec2& world_size) {
    const glm::vec2 screen_center = world_to_screen(camera, world_center);
    return SDL_FRect{
        screen_center.x - world_size.x * camera->pixels_per_world_unit * 0.5F,
        screen_center.y - world_size.y * camera->pixels_per_world_unit * 0.5F,
        world_size.x * camera->pixels_per_world_unit,
        world_size.y * camera->pixels_per_world_unit,
    };
}

std::uint8_t toggle_frame(float seed) {
    return static_cast<std::uint8_t>(static_cast<int>(std::floor(seed)) & 0x01);
}

std::uint8_t player_walk_frame(const Player* player) {
    if (player == nullptr || player->move_direction == MoveDirection::None) {
        return 0;
    }

    return toggle_frame((player->position.x + player->position.y) * 2.0F);
}

std::uint8_t enemy_walk_frame(const Enemy* enemy) {
    if (enemy == nullptr) {
        return 0;
    }

    return toggle_frame((enemy->position.x + enemy->position.y + enemy->action_seconds_remaining) *
                        2.0F);
}

SpriteClipId link_clip_id(const Player* player) {
    const bool magic_shield = player->has_magic_shield;
    switch (player->facing) {
    case Facing::Down:
        return magic_shield ? SpriteClipId::LinkMagicShieldDown : SpriteClipId::LinkShieldDown;
    case Facing::Up:
        return magic_shield ? SpriteClipId::LinkMagicShieldUp : SpriteClipId::LinkShieldUp;
    case Facing::Left:
        return magic_shield ? SpriteClipId::LinkMagicShieldLeft : SpriteClipId::LinkShieldLeft;
    case Facing::Right:
        return magic_shield ? SpriteClipId::LinkMagicShieldRight : SpriteClipId::LinkShieldRight;
    }

    return SpriteClipId::LinkShieldDown;
}

using FacingClipSet = std::array<SpriteClipId, 4>;

SpriteClipId clip_for_facing(const FacingClipSet& clips, Facing facing) {
    switch (facing) {
    case Facing::Down:
        return clips[0];
    case Facing::Up:
        return clips[1];
    case Facing::Left:
        return clips[2];
    case Facing::Right:
        return clips[3];
    }

    return clips[0];
}

bool is_blue_octorok(const Enemy& enemy) {
    return enemy.subtype > 0 || enemy.max_health > 1;
}

bool is_blue_moblin(const Enemy& enemy) {
    return enemy.subtype > 0 || enemy.max_health > 1;
}

bool is_blue_lynel(const Enemy& enemy) {
    return enemy.subtype == 0;
}

bool is_blue_goriya(const Enemy& enemy) {
    return enemy.subtype == 0;
}

bool is_red_darknut(const Enemy& enemy) {
    return enemy.subtype > 0 || enemy.max_health > 3;
}

bool clip_for_enemy(const Enemy* enemy, SpriteClipId* clip_id_out) {
    if (enemy == nullptr || clip_id_out == nullptr) {
        return false;
    }

    static constexpr FacingClipSet kOctorokRed = {
        SpriteClipId::OctorokRedDown,
        SpriteClipId::OctorokRedUp,
        SpriteClipId::OctorokRedLeft,
        SpriteClipId::OctorokRedRight,
    };
    static constexpr FacingClipSet kOctorokBlue = {
        SpriteClipId::OctorokBlueDown,
        SpriteClipId::OctorokBlueUp,
        SpriteClipId::OctorokBlueLeft,
        SpriteClipId::OctorokBlueRight,
    };
    static constexpr FacingClipSet kMoblinRed = {
        SpriteClipId::MoblinRedDown,
        SpriteClipId::MoblinRedUp,
        SpriteClipId::MoblinRedLeft,
        SpriteClipId::MoblinRedRight,
    };
    static constexpr FacingClipSet kMoblinBlue = {
        SpriteClipId::MoblinBlueDown,
        SpriteClipId::MoblinBlueUp,
        SpriteClipId::MoblinBlueLeft,
        SpriteClipId::MoblinBlueRight,
    };
    static constexpr FacingClipSet kLynelRed = {
        SpriteClipId::LynelRedDown,
        SpriteClipId::LynelRedUp,
        SpriteClipId::LynelRedLeft,
        SpriteClipId::LynelRedRight,
    };
    static constexpr FacingClipSet kLynelBlue = {
        SpriteClipId::LynelBlueDown,
        SpriteClipId::LynelBlueUp,
        SpriteClipId::LynelBlueLeft,
        SpriteClipId::LynelBlueRight,
    };
    static constexpr FacingClipSet kGoriyaRed = {
        SpriteClipId::GoriyaRedDown,
        SpriteClipId::GoriyaRedUp,
        SpriteClipId::GoriyaRedLeft,
        SpriteClipId::GoriyaRedRight,
    };
    static constexpr FacingClipSet kGoriyaBlue = {
        SpriteClipId::GoriyaBlueDown,
        SpriteClipId::GoriyaBlueUp,
        SpriteClipId::GoriyaBlueLeft,
        SpriteClipId::GoriyaBlueRight,
    };
    static constexpr FacingClipSet kDarknutRed = {
        SpriteClipId::DarknutRedDown,
        SpriteClipId::DarknutRedUp,
        SpriteClipId::DarknutRedLeft,
        SpriteClipId::DarknutRedRight,
    };
    static constexpr FacingClipSet kDarknutBlue = {
        SpriteClipId::DarknutBlueDown,
        SpriteClipId::DarknutBlueUp,
        SpriteClipId::DarknutBlueLeft,
        SpriteClipId::DarknutBlueRight,
    };

    switch (enemy->kind) {
    case EnemyKind::Octorok:
        *clip_id_out =
            clip_for_facing(is_blue_octorok(*enemy) ? kOctorokBlue : kOctorokRed, enemy->facing);
        return true;
    case EnemyKind::Moblin:
        *clip_id_out =
            clip_for_facing(is_blue_moblin(*enemy) ? kMoblinBlue : kMoblinRed, enemy->facing);
        return true;
    case EnemyKind::Lynel:
        *clip_id_out =
            clip_for_facing(is_blue_lynel(*enemy) ? kLynelBlue : kLynelRed, enemy->facing);
        return true;
    case EnemyKind::Goriya:
        *clip_id_out =
            clip_for_facing(is_blue_goriya(*enemy) ? kGoriyaBlue : kGoriyaRed, enemy->facing);
        return true;
    case EnemyKind::Darknut:
        *clip_id_out =
            clip_for_facing(is_red_darknut(*enemy) ? kDarknutRed : kDarknutBlue, enemy->facing);
        return true;
    default:
        return false;
    }
}

bool draw_clip_frame(SDL_Renderer* renderer, const SpriteAssets* assets, SpriteClipId clip_id,
                     int frame_index, const Camera* camera, const glm::vec2& position) {
    const SpriteClip* clip = get_sprite_clip(assets, clip_id);
    if (clip == nullptr || clip->texture == nullptr || camera == nullptr) {
        return false;
    }

    const SDL_FRect src_rect = get_sprite_clip_source_rect(clip, frame_index);
    const SDL_FRect dst_rect =
        world_rect_to_screen(camera, position + glm::vec2(clip->offset_x, clip->offset_y),
                             glm::vec2(clip->world_width, clip->world_height));
    SDL_RenderTexture(renderer, clip->texture, &src_rect, &dst_rect);
    return true;
}

} // namespace

bool render_enemy_asset_sprite(SDL_Renderer* renderer, const SpriteAssets* assets,
                               const Camera* camera, const Enemy* enemy) {
    if (renderer == nullptr || assets == nullptr || !assets->loaded) {
        return false;
    }

    SpriteClipId clip_id = SpriteClipId::OctorokRedDown;
    if (!clip_for_enemy(enemy, &clip_id)) {
        return false;
    }

    return draw_clip_frame(renderer, assets, clip_id, static_cast<int>(enemy_walk_frame(enemy)),
                           camera, enemy->position);
}

bool render_player_asset_sprite(SDL_Renderer* renderer, const SpriteAssets* assets,
                                const Camera* camera, const Player* player) {
    if (renderer == nullptr || assets == nullptr || !assets->loaded || player == nullptr) {
        return false;
    }

    return draw_clip_frame(renderer, assets, link_clip_id(player),
                           static_cast<int>(player_walk_frame(player)), camera, player->position);
}

} // namespace z1m

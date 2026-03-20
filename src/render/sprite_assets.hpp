#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <cstddef>

namespace z1m {

enum class SpriteClipId {
    LinkShieldDown,
    LinkShieldUp,
    LinkShieldLeft,
    LinkShieldRight,
    LinkMagicShieldDown,
    LinkMagicShieldUp,
    LinkMagicShieldLeft,
    LinkMagicShieldRight,
    OctorokRedDown,
    OctorokRedUp,
    OctorokRedLeft,
    OctorokRedRight,
    OctorokBlueDown,
    OctorokBlueUp,
    OctorokBlueLeft,
    OctorokBlueRight,
    MoblinRedDown,
    MoblinRedUp,
    MoblinRedLeft,
    MoblinRedRight,
    MoblinBlueDown,
    MoblinBlueUp,
    MoblinBlueLeft,
    MoblinBlueRight,
    LynelRedDown,
    LynelRedUp,
    LynelRedLeft,
    LynelRedRight,
    LynelBlueDown,
    LynelBlueUp,
    LynelBlueLeft,
    LynelBlueRight,
    GoriyaRedDown,
    GoriyaRedUp,
    GoriyaRedLeft,
    GoriyaRedRight,
    GoriyaBlueDown,
    GoriyaBlueUp,
    GoriyaBlueLeft,
    GoriyaBlueRight,
    DarknutRedDown,
    DarknutRedUp,
    DarknutRedLeft,
    DarknutRedRight,
    DarknutBlueDown,
    DarknutBlueUp,
    DarknutBlueLeft,
    DarknutBlueRight,
    Count,
};

struct SpriteClip {
    bool loaded = false;
    SDL_Texture* texture = nullptr;
    int texture_width = 0;
    int texture_height = 0;
    int frame_width = 0;
    int frame_height = 0;
    int frame_count = 0;
    int frame_ms = 120;
    float world_width = 1.0F;
    float world_height = 1.0F;
    float offset_x = 0.0F;
    float offset_y = 0.0F;
    float aabb_width = 0.0F;
    float aabb_height = 0.0F;
};

constexpr std::size_t kSpriteClipCount = static_cast<std::size_t>(SpriteClipId::Count);

struct SpriteAssets {
    bool loaded = false;
    std::array<SpriteClip, kSpriteClipCount> clips = {};
};

bool load_sprite_assets(SDL_Renderer* renderer, const char* root_path, SpriteAssets* assets);
void unload_sprite_assets(SpriteAssets* assets);
const SpriteClip* get_sprite_clip(const SpriteAssets* assets, SpriteClipId clip_id);
SDL_FRect get_sprite_clip_source_rect(const SpriteClip* clip, int frame_index);

} // namespace z1m

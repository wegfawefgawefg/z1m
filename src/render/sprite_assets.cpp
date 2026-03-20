#include "render/sprite_assets.hpp"

#include <png.h>

#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace z1m {

namespace {

struct ClipLoadSpec {
    SpriteClipId id;
    const char* stem;
};

constexpr std::array<ClipLoadSpec, kSpriteClipCount> kClipSpecs = {{
    {SpriteClipId::LinkShieldDown, "link/shield_down"},
    {SpriteClipId::LinkShieldUp, "link/shield_up"},
    {SpriteClipId::LinkShieldLeft, "link/shield_left"},
    {SpriteClipId::LinkShieldRight, "link/shield_right"},
    {SpriteClipId::LinkMagicShieldDown, "link/magic_shield_down"},
    {SpriteClipId::LinkMagicShieldUp, "link/magic_shield_up"},
    {SpriteClipId::LinkMagicShieldLeft, "link/magic_shield_left"},
    {SpriteClipId::LinkMagicShieldRight, "link/magic_shield_right"},
    {SpriteClipId::OctorokRedDown, "enemies/octorok_red_down"},
    {SpriteClipId::OctorokRedUp, "enemies/octorok_red_up"},
    {SpriteClipId::OctorokRedLeft, "enemies/octorok_red_left"},
    {SpriteClipId::OctorokRedRight, "enemies/octorok_red_right"},
    {SpriteClipId::OctorokBlueDown, "enemies/octorok_blue_down"},
    {SpriteClipId::OctorokBlueUp, "enemies/octorok_blue_up"},
    {SpriteClipId::OctorokBlueLeft, "enemies/octorok_blue_left"},
    {SpriteClipId::OctorokBlueRight, "enemies/octorok_blue_right"},
    {SpriteClipId::MoblinRedDown, "enemies/moblin_red_down"},
    {SpriteClipId::MoblinRedUp, "enemies/moblin_red_up"},
    {SpriteClipId::MoblinRedLeft, "enemies/moblin_red_left"},
    {SpriteClipId::MoblinRedRight, "enemies/moblin_red_right"},
    {SpriteClipId::MoblinBlueDown, "enemies/moblin_blue_down"},
    {SpriteClipId::MoblinBlueUp, "enemies/moblin_blue_up"},
    {SpriteClipId::MoblinBlueLeft, "enemies/moblin_blue_left"},
    {SpriteClipId::MoblinBlueRight, "enemies/moblin_blue_right"},
    {SpriteClipId::LynelRedDown, "enemies/lynel_red_down"},
    {SpriteClipId::LynelRedUp, "enemies/lynel_red_up"},
    {SpriteClipId::LynelRedLeft, "enemies/lynel_red_left"},
    {SpriteClipId::LynelRedRight, "enemies/lynel_red_right"},
    {SpriteClipId::LynelBlueDown, "enemies/lynel_blue_down"},
    {SpriteClipId::LynelBlueUp, "enemies/lynel_blue_up"},
    {SpriteClipId::LynelBlueLeft, "enemies/lynel_blue_left"},
    {SpriteClipId::LynelBlueRight, "enemies/lynel_blue_right"},
    {SpriteClipId::GoriyaRedDown, "enemies/goriya_red_down"},
    {SpriteClipId::GoriyaRedUp, "enemies/goriya_red_up"},
    {SpriteClipId::GoriyaRedLeft, "enemies/goriya_red_left"},
    {SpriteClipId::GoriyaRedRight, "enemies/goriya_red_right"},
    {SpriteClipId::GoriyaBlueDown, "enemies/goriya_blue_down"},
    {SpriteClipId::GoriyaBlueUp, "enemies/goriya_blue_up"},
    {SpriteClipId::GoriyaBlueLeft, "enemies/goriya_blue_left"},
    {SpriteClipId::GoriyaBlueRight, "enemies/goriya_blue_right"},
    {SpriteClipId::DarknutRedDown, "enemies/darknut_red_down"},
    {SpriteClipId::DarknutRedUp, "enemies/darknut_red_up"},
    {SpriteClipId::DarknutRedLeft, "enemies/darknut_red_left"},
    {SpriteClipId::DarknutRedRight, "enemies/darknut_red_right"},
    {SpriteClipId::DarknutBlueDown, "enemies/darknut_blue_down"},
    {SpriteClipId::DarknutBlueUp, "enemies/darknut_blue_up"},
    {SpriteClipId::DarknutBlueLeft, "enemies/darknut_blue_left"},
    {SpriteClipId::DarknutBlueRight, "enemies/darknut_blue_right"},
}};

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return {};
    }

    return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

bool parse_json_number(const std::string& json, const char* key, double* value_out) {
    const std::string needle = "\"" + std::string(key) + "\"";
    const std::size_t key_pos = json.find(needle);
    if (key_pos == std::string::npos) {
        return false;
    }

    const std::size_t colon_pos = json.find(':', key_pos + needle.size());
    if (colon_pos == std::string::npos) {
        return false;
    }

    const char* start = json.c_str() + colon_pos + 1;
    char* end = nullptr;
    const double value = std::strtod(start, &end);
    if (end == start) {
        return false;
    }

    *value_out = value;
    return true;
}

bool parse_json_int(const std::string& json, const char* key, int default_value, int* value_out) {
    double parsed_value = static_cast<double>(default_value);
    if (!parse_json_number(json, key, &parsed_value)) {
        *value_out = default_value;
        return true;
    }

    *value_out = static_cast<int>(parsed_value);
    return true;
}

bool parse_json_float(const std::string& json, const char* key, float default_value,
                      float* value_out) {
    double parsed_value = static_cast<double>(default_value);
    if (!parse_json_number(json, key, &parsed_value)) {
        *value_out = default_value;
        return true;
    }

    *value_out = static_cast<float>(parsed_value);
    return true;
}

bool load_clip_metadata(const std::filesystem::path& path, SpriteClip* clip) {
    const std::string json = read_text_file(path);
    if (json.empty()) {
        return false;
    }

    return parse_json_int(json, "frame_width", 0, &clip->frame_width) &&
           parse_json_int(json, "frame_height", 0, &clip->frame_height) &&
           parse_json_int(json, "frame_count", 0, &clip->frame_count) &&
           parse_json_int(json, "frame_ms", 120, &clip->frame_ms) &&
           parse_json_float(json, "world_width", 1.0F, &clip->world_width) &&
           parse_json_float(json, "world_height", 1.0F, &clip->world_height) &&
           parse_json_float(json, "offset_x", 0.0F, &clip->offset_x) &&
           parse_json_float(json, "offset_y", 0.0F, &clip->offset_y) &&
           parse_json_float(json, "aabb_width", 0.0F, &clip->aabb_width) &&
           parse_json_float(json, "aabb_height", 0.0F, &clip->aabb_height);
}

SDL_Surface* load_png_surface(const std::filesystem::path& path) {
    FILE* file = std::fopen(path.string().c_str(), "rb");
    if (file == nullptr) {
        return nullptr;
    }

    png_structp png_ptr =
        png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png_ptr == nullptr) {
        std::fclose(file);
        return nullptr;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == nullptr) {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        std::fclose(file);
        return nullptr;
    }

    if (setjmp(png_jmpbuf(png_ptr)) != 0) {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        std::fclose(file);
        return nullptr;
    }

    png_init_io(png_ptr, file);
    png_read_info(png_ptr, info_ptr);

    const png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
    const png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
    const png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    const png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    if (bit_depth == 16) {
        png_set_strip_16(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    }
    if ((png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) != 0U) {
        png_set_tRNS_to_alpha(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png_ptr);
    }

    png_read_update_info(png_ptr, info_ptr);
    const png_size_t row_bytes = png_get_rowbytes(png_ptr, info_ptr);

    std::vector<std::uint8_t> pixel_bytes(row_bytes * height, 0);
    std::vector<png_bytep> rows(height, nullptr);
    for (png_uint_32 row = 0; row < height; ++row) {
        rows[static_cast<std::size_t>(row)] = pixel_bytes.data() + row_bytes * row;
    }

    png_read_image(png_ptr, rows.data());
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    std::fclose(file);

    SDL_Surface* surface = SDL_CreateSurface(static_cast<int>(width), static_cast<int>(height),
                                             SDL_PIXELFORMAT_RGBA32);
    if (surface == nullptr) {
        return nullptr;
    }

    const std::size_t dst_pitch = static_cast<std::size_t>(surface->pitch);
    const std::size_t copy_pitch = std::min(dst_pitch, static_cast<std::size_t>(row_bytes));
    for (png_uint_32 row = 0; row < height; ++row) {
        std::memcpy(static_cast<std::uint8_t*>(surface->pixels) + dst_pitch * row,
                    pixel_bytes.data() + static_cast<std::size_t>(row_bytes) * row, copy_pitch);
    }

    return surface;
}

bool load_sprite_clip(SDL_Renderer* renderer, const std::filesystem::path& root_path,
                      const ClipLoadSpec& spec, SpriteClip* clip) {
    const std::filesystem::path png_path = root_path / (std::string(spec.stem) + ".png");
    const std::filesystem::path json_path = root_path / (std::string(spec.stem) + ".json");

    if (!load_clip_metadata(json_path, clip)) {
        return false;
    }

    SDL_Surface* surface = load_png_surface(png_path);
    if (surface == nullptr) {
        return false;
    }

    clip->texture_width = surface->w;
    clip->texture_height = surface->h;
    if (clip->frame_width <= 0 || clip->frame_height <= 0 || clip->frame_count <= 0) {
        SDL_DestroySurface(surface);
        return false;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (texture == nullptr) {
        return false;
    }

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    clip->texture = texture;
    clip->loaded = true;
    return true;
}

} // namespace

bool load_sprite_assets(SDL_Renderer* renderer, const char* root_path, SpriteAssets* assets) {
    unload_sprite_assets(assets);
    if (renderer == nullptr || root_path == nullptr || assets == nullptr) {
        return false;
    }

    const std::filesystem::path base_path(root_path);
    bool all_loaded = true;
    int loaded_count = 0;

    for (const ClipLoadSpec& spec : kClipSpecs) {
        SpriteClip& clip = assets->clips[static_cast<std::size_t>(spec.id)];
        if (!load_sprite_clip(renderer, base_path, spec, &clip)) {
            all_loaded = false;
            continue;
        }
        ++loaded_count;
    }

    assets->loaded = loaded_count > 0;
    return all_loaded && assets->loaded;
}

void unload_sprite_assets(SpriteAssets* assets) {
    if (assets == nullptr) {
        return;
    }

    for (SpriteClip& clip : assets->clips) {
        if (clip.texture != nullptr) {
            SDL_DestroyTexture(clip.texture);
            clip.texture = nullptr;
        }
        clip = SpriteClip{};
    }

    assets->loaded = false;
}

const SpriteClip* get_sprite_clip(const SpriteAssets* assets, SpriteClipId clip_id) {
    if (assets == nullptr) {
        return nullptr;
    }

    const SpriteClip& clip = assets->clips[static_cast<std::size_t>(clip_id)];
    return clip.loaded ? &clip : nullptr;
}

SDL_FRect get_sprite_clip_source_rect(const SpriteClip* clip, int frame_index) {
    if (clip == nullptr || clip->frame_width <= 0 || clip->frame_height <= 0 ||
        clip->frame_count <= 0) {
        return SDL_FRect{0.0F, 0.0F, 0.0F, 0.0F};
    }

    const int clamped_index = std::clamp(frame_index, 0, clip->frame_count - 1);
    return SDL_FRect{static_cast<float>(clamped_index * clip->frame_width), 0.0F,
                     static_cast<float>(clip->frame_width), static_cast<float>(clip->frame_height)};
}

} // namespace z1m

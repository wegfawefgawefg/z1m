#include "render/debug_tileset.hpp"

#include <SDL3/SDL.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <vector>

namespace z1m {

constexpr int kTileCount = 256;
constexpr int kBytesPerTile = 16;
constexpr int kAtlasColumns = 16;
constexpr int kAtlasTileSize = 8;
constexpr int kSpriteHeightPixels = 16;
constexpr int kSpriteAtlasRows = 8;
constexpr int kSpriteCount = 128;
constexpr int kCommonSpriteOffset = 32911;
constexpr int kCommonSpriteLength = 1792;
constexpr int kCommonBackgroundOffset = 34703;
constexpr int kCommonBackgroundLength = 1792;
constexpr int kCommonMiscOffset = 36495;
constexpr int kCommonMiscLength = 224;
constexpr int kOverworldBackgroundOffset = 51531;
constexpr int kOverworldBackgroundLength = 2080;
constexpr int kOverworldSpriteOffset = 53611;
constexpr int kOverworldSpriteLength = 1824;

constexpr std::array<std::uint8_t, 131> kObjAnimations = {
    0x00, 0x08, 0x0B, 0x0F, 0x13, 0x17, 0x5C, 0x60, 0x1B, 0x1B, 0x21, 0x21, 0x64, 0x6A,
    0x27, 0x29, 0x2B, 0x35, 0x3F, 0x70, 0x74, 0x76, 0x76, 0x78, 0x7A, 0x7E, 0x80, 0x49,
    0x82, 0x84, 0x86, 0x4B, 0x4F, 0x4F, 0x51, 0x51, 0x88, 0x8C, 0x90, 0x90, 0x92, 0x94,
    0x96, 0x98, 0x99, 0x99, 0x99, 0x53, 0x54, 0x9A, 0x9B, 0x9B, 0xA5, 0xA5, 0xAB, 0xAB,
    0xAC, 0xAE, 0xAE, 0xAF, 0xAF, 0xB2, 0xB8, 0xB8, 0x08, 0x08, 0xC6, 0xC6, 0xC6, 0xC6,
    0xC6, 0xC6, 0xC8, 0xC8, 0xC9, 0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA,
    0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0xCB, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55, 0x56, 0x57, 0x57, 0xCB, 0xCC, 0x58, 0x58, 0x58, 0x58, 0x58,
    0x58, 0x58, 0x58, 0x58, 0x59, 0x59, 0x59, 0x59, 0x5A, 0x5A, 0x5A, 0x5A, 0x5B, 0x5B,
    0x5B,
};

constexpr std::array<std::uint8_t, 256> kObjAnimFrameHeap = {
    0x00, 0x04, 0x08, 0x0C, 0x10, 0x10, 0x14, 0x18, 0x5C, 0x9E, 0x44, 0xCE, 0xD2, 0xD6,
    0xDA, 0xCE, 0xD2, 0xD6, 0xDA, 0xF0, 0xF4, 0xF8, 0xFC, 0xF0, 0xF4, 0xF8, 0xFC, 0xB4,
    0xB0, 0xB0, 0xB8, 0xB2, 0xB2, 0xB4, 0xB0, 0xB0, 0xB8, 0xB2, 0xB2, 0xCA, 0xCC, 0xCA,
    0xCC, 0xBC, 0xBE, 0xC0, 0xC0, 0xC2, 0xC4, 0xC0, 0xC0, 0xBC, 0xBE, 0xBC, 0xBE, 0xC0,
    0xC0, 0xC2, 0xC4, 0xC0, 0xC0, 0xBC, 0xBE, 0xBC, 0xBE, 0xEC, 0xEE, 0xEC, 0xEE, 0xEC,
    0xEE, 0xBC, 0xBE, 0xC6, 0xC8, 0xA0, 0xA8, 0xA4, 0xAC, 0x90, 0xE8, 0xE4, 0xE0, 0x94,
    0xF3, 0xC9, 0xBD, 0xC1, 0x98, 0x9A, 0x9C, 0xF8, 0xB8, 0xBC, 0xB0, 0xB4, 0xB8, 0xBC,
    0xB0, 0xB4, 0xB8, 0xAC, 0xB4, 0xBC, 0xB0, 0xB4, 0xB8, 0xAC, 0xB4, 0xBC, 0xB0, 0xB4,
    0xAC, 0xAE, 0xB0, 0xB2, 0xA8, 0xAA, 0x92, 0x94, 0xA0, 0xA2, 0xA6, 0xA4, 0xA2, 0xA4,
    0xD8, 0xDA, 0x00, 0x00, 0x9A, 0x9C, 0x9A, 0x9C, 0x9A, 0x9C, 0xB4, 0xB8, 0xBC, 0xBE,
    0xB4, 0xB8, 0xBC, 0xBE, 0xFC, 0xFE, 0xAC, 0x9C, 0xA0, 0xA4, 0xA0, 0xA4, 0xA8, 0x8E,
    0xA4, 0xDC, 0xE0, 0xE4, 0xE8, 0xEC, 0xF0, 0xF4, 0xF8, 0xFA, 0xFE, 0xF4, 0xF6, 0xFE,
    0xFC, 0xF0, 0xF8, 0xB0, 0xF6, 0xF0, 0xD4, 0xFC, 0xFE, 0xF8, 0xE8, 0xEA, 0xE0, 0xE4,
    0xEC, 0xEC, 0xD0, 0xD4, 0xD8, 0xDC, 0xE0, 0xE4, 0xC0, 0xC8, 0xC4, 0xCC, 0xE8, 0xEA,
    0x72, 0x74, 0xDE, 0xEE, 0xF8, 0x96, 0x98, 0xB1,
};

constexpr std::array<std::uint8_t, 256> kObjAnimAttrHeap = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x01, 0x01,
    0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x82, 0x02, 0x02, 0x82, 0x02, 0x01, 0x81, 0x01, 0x01, 0x81, 0x01, 0x01, 0x01, 0x02,
    0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01,
    0x02, 0x03, 0x03, 0x03, 0x02, 0x02, 0x00, 0x02, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02,
    0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x01, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x02, 0x00, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x03, 0x03, 0x02, 0x02, 0x01, 0x01, 0x02, 0x03,
};

struct TilePalette {
    std::array<SDL_Color, 4> colors = {};
};

SDL_Color make_color(Uint8 r, Uint8 g, Uint8 b) {
    return SDL_Color{r, g, b, 255};
}

TilePalette make_palette(SDL_Color c0, SDL_Color c1, SDL_Color c2, SDL_Color c3) {
    return TilePalette{{c0, c1, c2, c3}};
}

SDL_Color color_for_nes_index(std::uint8_t index) {
    switch (index) {
    case 0x07:
        return make_color(60, 24, 0);
    case 0x0A:
        return make_color(0, 64, 0);
    case 0x0C:
        return make_color(0, 40, 88);
    case 0x00:
        return make_color(92, 92, 92);
    case 0x12:
        return make_color(0, 88, 248);
    case 0x15:
        return make_color(172, 124, 0);
    case 0x16:
        return make_color(172, 124, 0);
    case 0x17:
        return make_color(200, 76, 12);
    case 0x1C:
        return make_color(0, 124, 172);
    case 0x1A:
        return make_color(0, 168, 0);
    case 0x22:
        return make_color(80, 160, 255);
    case 0x26:
        return make_color(216, 120, 0);
    case 0x27:
        return make_color(252, 160, 68);
    case 0x29:
        return make_color(0, 200, 88);
    case 0x2A:
        return make_color(88, 216, 84);
    case 0x2C:
        return make_color(88, 248, 152);
    case 0x30:
        return make_color(252, 252, 252);
    case 0x36:
        return make_color(252, 224, 168);
    case 0x37:
        return make_color(252, 188, 60);
    case 0x3C:
        return make_color(160, 214, 228);
    default:
        return make_color(252, 216, 168);
    }
}

TilePalette palette_for_selector(std::uint8_t selector) {
    const SDL_Color backdrop = make_color(252, 216, 168);
    switch (selector & 0x03) {
    case 0:
        return make_palette(backdrop, color_for_nes_index(0x30), color_for_nes_index(0x00),
                            color_for_nes_index(0x12));
    case 2:
        return make_palette(backdrop, color_for_nes_index(0x1A), color_for_nes_index(0x37),
                            color_for_nes_index(0x12));
    case 3:
        return make_palette(backdrop, color_for_nes_index(0x17), color_for_nes_index(0x37),
                            color_for_nes_index(0x12));
    default:
        return make_palette(backdrop, color_for_nes_index(0x17), color_for_nes_index(0x37),
                            color_for_nes_index(0x12));
    }
}

bool read_background_pattern_data(const char* rom_path, std::vector<std::uint8_t>* pattern_data) {
    std::ifstream input(rom_path, std::ios::binary);
    if (!input.is_open()) {
        return false;
    }

    std::vector<std::uint8_t> rom((std::istreambuf_iterator<char>(input)),
                                  std::istreambuf_iterator<char>());
    if (rom.size() <
        static_cast<std::size_t>(kOverworldBackgroundOffset + kOverworldBackgroundLength)) {
        return false;
    }

    pattern_data->assign(static_cast<std::size_t>(kTileCount * kBytesPerTile), 0);

    for (int index = 0; index < kCommonBackgroundLength; ++index) {
        (*pattern_data)[static_cast<std::size_t>(index)] =
            rom[static_cast<std::size_t>(kCommonBackgroundOffset + index)];
    }

    const std::size_t overworld_start = static_cast<std::size_t>(0x70 * kBytesPerTile);
    for (int index = 0; index < kOverworldBackgroundLength; ++index) {
        (*pattern_data)[overworld_start + static_cast<std::size_t>(index)] =
            rom[static_cast<std::size_t>(kOverworldBackgroundOffset + index)];
    }

    const std::size_t misc_start = static_cast<std::size_t>(0xF2 * kBytesPerTile);
    for (int index = 0; index < kCommonMiscLength; ++index) {
        (*pattern_data)[misc_start + static_cast<std::size_t>(index)] =
            rom[static_cast<std::size_t>(kCommonMiscOffset + index)];
    }

    return true;
}

bool read_sprite_pattern_data(const char* rom_path, std::vector<std::uint8_t>* pattern_data) {
    std::ifstream input(rom_path, std::ios::binary);
    if (!input.is_open()) {
        return false;
    }

    std::vector<std::uint8_t> rom((std::istreambuf_iterator<char>(input)),
                                  std::istreambuf_iterator<char>());
    if (rom.size() < static_cast<std::size_t>(kOverworldSpriteOffset + kOverworldSpriteLength)) {
        return false;
    }

    pattern_data->assign(static_cast<std::size_t>(kTileCount * kBytesPerTile), 0);

    for (int index = 0; index < kCommonSpriteLength; ++index) {
        (*pattern_data)[static_cast<std::size_t>(index)] =
            rom[static_cast<std::size_t>(kCommonSpriteOffset + index)];
    }

    const std::size_t overworld_start = static_cast<std::size_t>(0x8E * kBytesPerTile);
    for (int index = 0; index < kOverworldSpriteLength; ++index) {
        (*pattern_data)[overworld_start + static_cast<std::size_t>(index)] =
            rom[static_cast<std::size_t>(kOverworldSpriteOffset + index)];
    }

    return true;
}

bool build_tile_surface(const std::vector<std::uint8_t>& pattern_data, const TilePalette& palette,
                        SDL_Surface* surface) {
    for (int tile_index = 0; tile_index < kTileCount; ++tile_index) {
        const int atlas_x = (tile_index % kAtlasColumns) * kAtlasTileSize;
        const int atlas_y = (tile_index / kAtlasColumns) * kAtlasTileSize;
        const int tile_offset = tile_index * kBytesPerTile;

        for (int row = 0; row < kAtlasTileSize; ++row) {
            const std::uint8_t plane0 = pattern_data[static_cast<std::size_t>(tile_offset + row)];
            const std::uint8_t plane1 =
                pattern_data[static_cast<std::size_t>(tile_offset + row + kAtlasTileSize)];

            for (int column = 0; column < kAtlasTileSize; ++column) {
                const int shift = 7 - column;
                const int color_index =
                    ((plane0 >> shift) & 0x01) | (((plane1 >> shift) & 0x01) << 1);
                const SDL_Color color = palette.colors[static_cast<std::size_t>(color_index)];
                if (!SDL_WriteSurfacePixel(surface, atlas_x + column, atlas_y + row, color.r,
                                           color.g, color.b, color.a)) {
                    return false;
                }
            }
        }
    }

    return true;
}

bool build_sprite_surface(const std::vector<std::uint8_t>& pattern_data, const TilePalette& palette,
                          SDL_Surface* surface) {
    for (int sprite_index = 0; sprite_index < kSpriteCount; ++sprite_index) {
        const int tile_index = sprite_index * 2;
        const int atlas_x = (sprite_index % kAtlasColumns) * kAtlasTileSize;
        const int atlas_y = (sprite_index / kAtlasColumns) * kSpriteHeightPixels;

        for (int half = 0; half < 2; ++half) {
            const int half_tile_index = tile_index + half;
            const int tile_offset = half_tile_index * kBytesPerTile;
            const int dst_y_offset = half * kAtlasTileSize;

            for (int row = 0; row < kAtlasTileSize; ++row) {
                const std::uint8_t plane0 =
                    pattern_data[static_cast<std::size_t>(tile_offset + row)];
                const std::uint8_t plane1 =
                    pattern_data[static_cast<std::size_t>(tile_offset + row + kAtlasTileSize)];

                for (int column = 0; column < kAtlasTileSize; ++column) {
                    const int shift = 7 - column;
                    const int color_index =
                        ((plane0 >> shift) & 0x01) | (((plane1 >> shift) & 0x01) << 1);
                    SDL_Color color = palette.colors[static_cast<std::size_t>(color_index)];
                    if (color_index == 0) {
                        color.a = 0;
                    }
                    if (!SDL_WriteSurfacePixel(surface, atlas_x + column,
                                               atlas_y + dst_y_offset + row, color.r, color.g,
                                               color.b, color.a)) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

TilePalette sprite_palette_for_selector(std::uint8_t selector) {
    constexpr std::array<std::array<std::uint8_t, 4>, 4> kSpriteRows = {{
        {0x0F, 0x29, 0x27, 0x07},
        {0x0F, 0x22, 0x27, 0x07},
        {0x0F, 0x26, 0x27, 0x07},
        {0x0F, 0x15, 0x27, 0x30},
    }};

    const auto& row = kSpriteRows[static_cast<std::size_t>(selector & 0x03)];
    return make_palette(color_for_nes_index(row[0]), color_for_nes_index(row[1]),
                        color_for_nes_index(row[2]), color_for_nes_index(row[3]));
}

bool load_debug_tileset(SDL_Renderer* renderer, const char* rom_path, DebugTileset* tileset) {
    unload_debug_tileset(tileset);

    std::vector<std::uint8_t> background_pattern_data;
    std::vector<std::uint8_t> sprite_pattern_data;
    if (!read_background_pattern_data(rom_path, &background_pattern_data) ||
        !read_sprite_pattern_data(rom_path, &sprite_pattern_data)) {
        return false;
    }

    for (int selector = 0; selector < 4; ++selector) {
        SDL_Surface* tile_surface =
            SDL_CreateSurface(kAtlasColumns * kAtlasTileSize, kAtlasColumns * kAtlasTileSize,
                              SDL_PIXELFORMAT_RGBA8888);
        SDL_Surface* sprite_surface =
            SDL_CreateSurface(kAtlasColumns * kAtlasTileSize, kSpriteAtlasRows * kSpriteHeightPixels,
                              SDL_PIXELFORMAT_RGBA8888);
        if (tile_surface == nullptr || sprite_surface == nullptr) {
            SDL_DestroySurface(tile_surface);
            SDL_DestroySurface(sprite_surface);
            unload_debug_tileset(tileset);
            return false;
        }

        const bool tile_surface_ok =
            build_tile_surface(background_pattern_data,
                               palette_for_selector(static_cast<std::uint8_t>(selector)),
                               tile_surface);
        const bool sprite_surface_ok =
            build_sprite_surface(sprite_pattern_data,
                                 sprite_palette_for_selector(static_cast<std::uint8_t>(selector)),
                                 sprite_surface);
        if (!tile_surface_ok || !sprite_surface_ok) {
            SDL_DestroySurface(tile_surface);
            SDL_DestroySurface(sprite_surface);
            unload_debug_tileset(tileset);
            return false;
        }

        SDL_Texture* tile_texture = SDL_CreateTextureFromSurface(renderer, tile_surface);
        SDL_Texture* sprite_texture = SDL_CreateTextureFromSurface(renderer, sprite_surface);
        SDL_DestroySurface(tile_surface);
        SDL_DestroySurface(sprite_surface);
        if (tile_texture == nullptr || sprite_texture == nullptr) {
            if (tile_texture != nullptr) {
                SDL_DestroyTexture(tile_texture);
            }
            if (sprite_texture != nullptr) {
                SDL_DestroyTexture(sprite_texture);
            }
            unload_debug_tileset(tileset);
            return false;
        }

        SDL_SetTextureScaleMode(tile_texture, SDL_SCALEMODE_NEAREST);
        SDL_SetTextureScaleMode(sprite_texture, SDL_SCALEMODE_NEAREST);
        tileset->textures[selector] = tile_texture;
        tileset->sprite_textures[selector] = sprite_texture;
    }

    tileset->loaded = true;
    tileset->tile_pixel_size = kAtlasTileSize;
    tileset->atlas_columns = kAtlasColumns;
    tileset->sprite_pixel_width = kAtlasTileSize;
    tileset->sprite_pixel_height = kSpriteHeightPixels;
    tileset->sprite_atlas_columns = kAtlasColumns;
    return true;
}

void unload_debug_tileset(DebugTileset* tileset) {
    for (SDL_Texture*& texture : tileset->textures) {
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }

    for (SDL_Texture*& texture : tileset->sprite_textures) {
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }

    tileset->loaded = false;
}

SDL_FRect get_debug_tileset_source_rect(const DebugTileset* tileset, std::uint8_t tile) {
    const int tile_index = static_cast<int>(tile);
    const int tile_x = tile_index % tileset->atlas_columns;
    const int tile_y = tile_index / tileset->atlas_columns;

    SDL_FRect rect;
    rect.x = static_cast<float>(tile_x * tileset->tile_pixel_size);
    rect.y = static_cast<float>(tile_y * tileset->tile_pixel_size);
    rect.w = static_cast<float>(tileset->tile_pixel_size);
    rect.h = static_cast<float>(tileset->tile_pixel_size);
    return rect;
}

SDL_Texture* get_debug_tileset_texture(const DebugTileset* tileset, std::uint8_t palette_selector) {
    if (tileset == nullptr) {
        return nullptr;
    }

    return tileset->textures[palette_selector & 0x03];
}

SDL_FRect get_debug_sprite_source_rect(const DebugTileset* tileset, std::uint8_t tile) {
    const int sprite_index = static_cast<int>(tile & 0xFE) / 2;
    const int tile_x = sprite_index % tileset->sprite_atlas_columns;
    const int tile_y = sprite_index / tileset->sprite_atlas_columns;

    SDL_FRect rect;
    rect.x = static_cast<float>(tile_x * tileset->sprite_pixel_width);
    rect.y = static_cast<float>(tile_y * tileset->sprite_pixel_height);
    rect.w = static_cast<float>(tileset->sprite_pixel_width);
    rect.h = static_cast<float>(tileset->sprite_pixel_height);
    return rect;
}

SDL_Texture* get_debug_sprite_texture(const DebugTileset* tileset, std::uint8_t palette_selector) {
    if (tileset == nullptr) {
        return nullptr;
    }

    return tileset->sprite_textures[palette_selector & 0x03];
}

bool lookup_debug_animation_frame(std::uint8_t animation_index, std::uint8_t frame,
                                  std::uint8_t* left_tile_out, std::uint8_t* attributes_out) {
    if (animation_index >= kObjAnimations.size()) {
        return false;
    }

    const std::size_t heap_index =
        static_cast<std::size_t>(kObjAnimations[animation_index]) + static_cast<std::size_t>(frame);
    if (heap_index >= kObjAnimFrameHeap.size()) {
        return false;
    }

    if (left_tile_out != nullptr) {
        *left_tile_out = kObjAnimFrameHeap[heap_index];
    }
    if (attributes_out != nullptr) {
        *attributes_out = kObjAnimAttrHeap[heap_index];
    }
    return true;
}

bool lookup_debug_object_frame(std::uint8_t object_type, std::uint8_t frame,
                               std::uint8_t* left_tile_out, std::uint8_t* attributes_out) {
    return lookup_debug_animation_frame(static_cast<std::uint8_t>(object_type + 1), frame,
                                        left_tile_out, attributes_out);
}

} // namespace z1m

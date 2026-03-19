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
constexpr int kCommonBackgroundOffset = 34687;
constexpr int kCommonBackgroundLength = 1792;
constexpr int kCommonMiscOffset = 36479;
constexpr int kCommonMiscLength = 224;
constexpr int kOverworldBackgroundOffset = 51515;
constexpr int kOverworldBackgroundLength = 2080;

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
    case 0x00:
        return make_color(92, 92, 92);
    case 0x12:
        return make_color(0, 88, 248);
    case 0x17:
        return make_color(200, 76, 12);
    case 0x1A:
        return make_color(0, 168, 0);
    case 0x30:
        return make_color(252, 252, 252);
    case 0x37:
        return make_color(252, 188, 60);
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

bool build_tileset_surface(const std::vector<std::uint8_t>& pattern_data,
                           const TilePalette& palette, SDL_Surface* surface) {
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

bool load_debug_tileset(SDL_Renderer* renderer, const char* rom_path, DebugTileset* tileset) {
    unload_debug_tileset(tileset);

    std::vector<std::uint8_t> pattern_data;
    if (!read_background_pattern_data(rom_path, &pattern_data)) {
        return false;
    }

    for (int selector = 0; selector < 4; ++selector) {
        SDL_Surface* surface =
            SDL_CreateSurface(kAtlasColumns * kAtlasTileSize, kAtlasColumns * kAtlasTileSize,
                              SDL_PIXELFORMAT_RGBA8888);
        if (surface == nullptr) {
            unload_debug_tileset(tileset);
            return false;
        }

        const bool surface_ok = build_tileset_surface(
            pattern_data, palette_for_selector(static_cast<std::uint8_t>(selector)), surface);
        if (!surface_ok) {
            SDL_DestroySurface(surface);
            unload_debug_tileset(tileset);
            return false;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        if (texture == nullptr) {
            unload_debug_tileset(tileset);
            return false;
        }

        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
        tileset->textures[selector] = texture;
    }

    tileset->loaded = true;
    tileset->tile_pixel_size = kAtlasTileSize;
    tileset->atlas_columns = kAtlasColumns;
    return true;
}

void unload_debug_tileset(DebugTileset* tileset) {
    for (SDL_Texture*& texture : tileset->textures) {
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

} // namespace z1m

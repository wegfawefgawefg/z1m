#pragma once

#include <SDL3/SDL.h>
#include <cstdint>

namespace z1m {

struct DebugTileset {
    bool loaded = false;
    SDL_Texture* textures[4] = {};
    int tile_pixel_size = 8;
    int atlas_columns = 16;
};

bool load_debug_tileset(SDL_Renderer* renderer, const char* rom_path, DebugTileset* tileset);
void unload_debug_tileset(DebugTileset* tileset);
SDL_FRect get_debug_tileset_source_rect(const DebugTileset* tileset, std::uint8_t tile);
SDL_Texture* get_debug_tileset_texture(const DebugTileset* tileset, std::uint8_t palette_selector);

} // namespace z1m

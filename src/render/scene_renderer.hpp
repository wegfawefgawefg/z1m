#pragma once

#include <SDL3/SDL.h>

namespace z1m {

struct DebugTileset;
struct Player;
struct World;

void render_scene(SDL_Renderer* renderer, const DebugTileset* tileset, const World* world,
                  const Player* player, float zoom);
void render_world(SDL_Renderer* renderer, const DebugTileset* tileset, const World* world,
                  const Player* player, float zoom);
void render_player(SDL_Renderer* renderer, const World* world, const Player* player, float zoom);

} // namespace z1m

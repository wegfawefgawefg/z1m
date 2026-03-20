#pragma once

#include <SDL3/SDL.h>

namespace z1m {

struct DebugView;
struct DebugTileset;
struct GameState;
struct Player;
struct World;

void render_scene(SDL_Renderer* renderer, const DebugTileset* tileset, const DebugView* debug_view,
                  const GameState* game_state, const World* world, const Player* player,
                  float zoom);
void render_world(SDL_Renderer* renderer, const DebugTileset* tileset, const World* world,
                  const Player* player, float zoom);
void render_cave(SDL_Renderer* renderer, const GameState* game_state, const Player* player,
                 float zoom);
void render_player(SDL_Renderer* renderer, const World* world, const Player* player, float zoom);
void render_game_entities(SDL_Renderer* renderer, const GameState* game_state, const World* world,
                          const Player* player, float zoom);
void render_debug_overlay(SDL_Renderer* renderer, const DebugView* debug_view,
                          const GameState* game_state, const World* world, const Player* player,
                          float zoom);

} // namespace z1m

#pragma once

#include <SDL3/SDL.h>

namespace z1m {

struct Camera;
struct Enemy;
struct Player;
struct SpriteAssets;

bool render_enemy_asset_sprite(SDL_Renderer* renderer, const SpriteAssets* assets,
                               const Camera* camera, const Enemy* enemy);
bool render_player_asset_sprite(SDL_Renderer* renderer, const SpriteAssets* assets,
                                const Camera* camera, const Player* player);

} // namespace z1m

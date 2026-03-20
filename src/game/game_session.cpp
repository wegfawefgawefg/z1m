#include "game/game_session.hpp"

#include "content/opening_content.hpp"
#include "content/overworld_warps.hpp"
#include "content/sandbox_content.hpp"

#include <algorithm>
#include <cmath>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace z1m {

// clang-format off
#include "game/game_session_shared.inc"
#include "game/game_session_items.inc"
#include "game/game_session_combat.inc"
#include "game/game_session_enemy_core.inc"
#include "game/game_session_enemy_ground.inc"
#include "game/game_session_enemy_boss.inc"
#include "game/game_session_enemy_tick.inc"
#include "game/game_session_pickups.inc"
#include "game/game_session_runtime.inc"
// clang-format on

#include "game/areas.hpp"

#include "content/opening_content.hpp"
#include "content/overworld_warps.hpp"
#include "content/sandbox_content.hpp"
#include "game/area_state.hpp"
#include "game/combat.hpp"
#include "game/tuning.hpp"

#include <array>
#include <cmath>
#include <string>

namespace z1m {

void try_enter_overworld_cave(GameState* play, const World* overworld_world, Player* player) {
    if (play->area_kind != AreaKind::Overworld || play->warp_cooldown_seconds > 0.0F) {
        return;
    }

    if (player->move_direction != MoveDirection::Up) {
        return;
    }

    std::array<OverworldWarp, kMaxRoomWarps> warps = {};
    int warp_count = 0;
    const OverworldWarp* warp = find_triggered_overworld_warp(
        &overworld_world->overworld, play->current_room_id, player->position, &warps, &warp_count);
    if (warp == nullptr) {
        return;
    }

    play->cave_return_room_id = play->current_room_id;
    play->cave_return_position = warp->return_position;
    const CaveDef* cave = get_cave_def(warp->cave_id);
    if (cave == nullptr) {
        return;
    }

    set_area_kind(play, player, AreaKind::Cave, warp->cave_id, cave->player_spawn);
}

void try_exit_cave(GameState* play, Player* player) {
    if (play->area_kind != AreaKind::Cave || play->warp_cooldown_seconds > 0.0F) {
        return;
    }

    const CaveDef* cave = get_cave_def(play->current_cave_id);
    if (cave == nullptr) {
        return;
    }

    const glm::vec2 delta = player->position - cave->exit_center;
    if (std::abs(delta.x) > cave->exit_half_size.x || std::abs(delta.y) > cave->exit_half_size.y) {
        return;
    }

    set_area_kind(play, player, AreaKind::Overworld, -1, play->cave_return_position);
}

void try_area_portals(GameState* play, Player* player) {
    if (play->warp_cooldown_seconds > 0.0F) {
        return;
    }

    std::array<AreaPortal, kMaxAreaPortals> portals = {};
    const int portal_count = gather_area_portals(play, &portals);
    for (int index = 0; index < portal_count; ++index) {
        const AreaPortal& portal = portals[static_cast<std::size_t>(index)];
        if (portal.requires_raft && !player->has_raft) {
            const glm::vec2 delta = player->position - portal.center;
            if (std::abs(delta.x) <= portal.half_size.x + 0.7F &&
                std::abs(delta.y) <= portal.half_size.y + 0.7F) {
                set_message(play, "need raft", 0.2F);
            }
            continue;
        }

        const glm::vec2 delta = player->position - portal.center;
        if (std::abs(delta.x) > portal.half_size.x || std::abs(delta.y) > portal.half_size.y) {
            continue;
        }

        set_area_kind(play, player, portal.target_area_kind, portal.target_cave_id,
                      portal.target_position);
        return;
    }
}

const World* get_active_world(const GameState* play, const World* overworld_world) {
    switch (play->area_kind) {
    case AreaKind::Overworld:
        return overworld_world;
    case AreaKind::Cave:
        return &play->cave_world;
    case AreaKind::EnemyZoo:
        return &play->enemy_zoo_world;
    case AreaKind::ItemZoo:
        return &play->item_zoo_world;
    }

    return overworld_world;
}

void set_area_kind(GameState* play, Player* player, AreaKind area_kind, int cave_id,
                   const glm::vec2& position) {
    play->area_kind = area_kind;
    play->current_cave_id = area_kind == AreaKind::Cave ? cave_id : -1;
    player->position = position;
    player->move_direction = MoveDirection::None;
    player->sword_cursed = false;
    player->sword_disabled_seconds = 0.0F;
    play->warp_cooldown_seconds = kAreaTransitionCooldownSeconds;
    update_current_room(play, player);
    if (area_kind == AreaKind::Cave) {
        ensure_sword_cave_pickup(play);
    }

    if (area_kind == AreaKind::Cave) {
        set_message(play, "entered cave", 1.0F);
    } else {
        set_message(play, std::string("entered ") + area_name(play), 1.0F);
    }
}

int gather_area_portals(const GameState* play, std::array<AreaPortal, kMaxAreaPortals>* portals) {
    return gather_sandbox_portals(play, portals);
}

const char* area_name(const GameState* play) {
    switch (play->area_kind) {
    case AreaKind::Overworld:
        return "overworld";
    case AreaKind::Cave:
        return "cave";
    case AreaKind::EnemyZoo:
        return "enemy-zoo";
    case AreaKind::ItemZoo:
        return "item-zoo";
    }

    return "overworld";
}

} // namespace z1m

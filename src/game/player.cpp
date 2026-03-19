#include "game/player.hpp"

#include "game/world.hpp"

#include <array>
#include <glm/common.hpp>

namespace z1m {

constexpr float kMoveSpeedTilesPerSecond = 11.25F;
constexpr float kSwordDurationSeconds = 5.0F / 60.0F;
constexpr float kBodyHalfWidthTiles = 0.30F;
constexpr float kFrontProbeTiles = 0.45F;

MoveDirection choose_move_direction(const Player* player, const PlayerCommand* command) {
    bool up = command->move_axis.y < 0.0F;
    bool down = command->move_axis.y > 0.0F;
    bool left = command->move_axis.x < 0.0F;
    bool right = command->move_axis.x > 0.0F;

    if (up && down) {
        up = false;
        down = false;
    }

    if (left && right) {
        left = false;
        right = false;
    }

    int pressed_count = 0;
    pressed_count += up ? 1 : 0;
    pressed_count += down ? 1 : 0;
    pressed_count += left ? 1 : 0;
    pressed_count += right ? 1 : 0;

    if (pressed_count == 0) {
        return MoveDirection::None;
    }

    if (pressed_count == 1) {
        if (up) {
            return MoveDirection::Up;
        }

        if (down) {
            return MoveDirection::Down;
        }

        if (left) {
            return MoveDirection::Left;
        }

        return MoveDirection::Right;
    }

    if (player->move_direction == MoveDirection::Up && up) {
        return MoveDirection::Up;
    }

    if (player->move_direction == MoveDirection::Down && down) {
        return MoveDirection::Down;
    }

    if (player->move_direction == MoveDirection::Left && left) {
        return MoveDirection::Left;
    }

    if (player->move_direction == MoveDirection::Right && right) {
        return MoveDirection::Right;
    }

    if (player->facing == Facing::Up && up) {
        return MoveDirection::Up;
    }

    if (player->facing == Facing::Down && down) {
        return MoveDirection::Down;
    }

    if (player->facing == Facing::Left && left) {
        return MoveDirection::Left;
    }

    if (player->facing == Facing::Right && right) {
        return MoveDirection::Right;
    }

    if (up) {
        return MoveDirection::Up;
    }

    if (down) {
        return MoveDirection::Down;
    }

    if (left) {
        return MoveDirection::Left;
    }

    return MoveDirection::Right;
}

Facing facing_from_move_direction(MoveDirection move_direction) {
    switch (move_direction) {
    case MoveDirection::Up:
        return Facing::Up;
    case MoveDirection::Down:
        return Facing::Down;
    case MoveDirection::Left:
        return Facing::Left;
    case MoveDirection::Right:
        return Facing::Right;
    case MoveDirection::None:
        return Facing::Down;
    }

    return Facing::Down;
}

glm::vec2 move_vector(MoveDirection move_direction) {
    switch (move_direction) {
    case MoveDirection::Up:
        return glm::vec2(0.0F, -1.0F);
    case MoveDirection::Down:
        return glm::vec2(0.0F, 1.0F);
    case MoveDirection::Left:
        return glm::vec2(-1.0F, 0.0F);
    case MoveDirection::Right:
        return glm::vec2(1.0F, 0.0F);
    case MoveDirection::None:
        return glm::vec2(0.0F, 0.0F);
    }

    return glm::vec2(0.0F, 0.0F);
}

bool probe_is_walkable(const Player* player, const World* world, const glm::vec2& probe,
                       MoveDirection move_direction) {
    if (world_is_walkable_tile(world, probe)) {
        return true;
    }

    if (!player->has_ladder) {
        return false;
    }

    const int tile_x = static_cast<int>(probe.x);
    const int tile_y = static_cast<int>(probe.y);
    if (world_tile_at(world, tile_x, tile_y) != TileKind::Water) {
        return false;
    }

    const glm::vec2 landing = probe + move_vector(move_direction);
    return world_is_walkable_tile(world, landing);
}

bool can_move_to(const Player* player, const World* world, const glm::vec2& position,
                 MoveDirection move_direction) {
    std::array<glm::vec2, 2> probes = {};

    switch (move_direction) {
    case MoveDirection::Up:
        probes = {
            position + glm::vec2(-kBodyHalfWidthTiles, -kFrontProbeTiles),
            position + glm::vec2(kBodyHalfWidthTiles, -kFrontProbeTiles),
        };
        break;
    case MoveDirection::Down:
        probes = {
            position + glm::vec2(-kBodyHalfWidthTiles, kFrontProbeTiles),
            position + glm::vec2(kBodyHalfWidthTiles, kFrontProbeTiles),
        };
        break;
    case MoveDirection::Left:
        probes = {
            position + glm::vec2(-kFrontProbeTiles, -kBodyHalfWidthTiles),
            position + glm::vec2(-kFrontProbeTiles, kBodyHalfWidthTiles),
        };
        break;
    case MoveDirection::Right:
        probes = {
            position + glm::vec2(kFrontProbeTiles, -kBodyHalfWidthTiles),
            position + glm::vec2(kFrontProbeTiles, kBodyHalfWidthTiles),
        };
        break;
    case MoveDirection::None:
        return true;
    }

    for (const glm::vec2& probe : probes) {
        if (!probe_is_walkable(player, world, probe, move_direction)) {
            return false;
        }
    }

    return true;
}

void move_with_collision(Player* player, const World* world, MoveDirection move_direction,
                         float distance) {
    if (move_direction == MoveDirection::None) {
        return;
    }

    const glm::vec2 candidate = player->position + move_vector(move_direction) * distance;
    if (!can_move_to(player, world, candidate, move_direction)) {
        return;
    }

    player->position = candidate;
}

Player make_player() {
    return Player{};
}

void tick_player(Player* player, const World* world, const PlayerCommand* command,
                 float dt_seconds) {
    if (player->sword_seconds_remaining > 0.0F) {
        player->sword_seconds_remaining =
            glm::max(0.0F, player->sword_seconds_remaining - dt_seconds);
    }

    const MoveDirection move_direction = choose_move_direction(player, command);
    player->move_direction = move_direction;

    if (move_direction != MoveDirection::None) {
        player->facing = facing_from_move_direction(move_direction);
        move_with_collision(player, world, move_direction, kMoveSpeedTilesPerSecond * dt_seconds);
    }

    if (command->attack_pressed) {
        player->sword_seconds_remaining = kSwordDurationSeconds;
    }
}

bool is_sword_active(const Player* player) {
    return player->sword_seconds_remaining > 0.0F;
}

glm::vec2 sword_world_position(const Player* player) {
    glm::vec2 offset(0.0F, 0.0F);

    switch (player->facing) {
    case Facing::Up:
        offset = glm::vec2(0.0F, -0.8F);
        break;
    case Facing::Down:
        offset = glm::vec2(0.0F, 0.8F);
        break;
    case Facing::Left:
        offset = glm::vec2(-0.8F, 0.0F);
        break;
    case Facing::Right:
        offset = glm::vec2(0.8F, 0.0F);
        break;
    }

    return player->position + offset;
}

const char* use_item_name(UseItemKind kind) {
    switch (kind) {
    case UseItemKind::None:
        return "none";
    case UseItemKind::Bombs:
        return "bombs";
    case UseItemKind::Boomerang:
        return "boomerang";
    case UseItemKind::Bow:
        return "bow";
    case UseItemKind::Candle:
        return "candle";
    case UseItemKind::Recorder:
        return "recorder";
    case UseItemKind::Potion:
        return "potion";
    }

    return "none";
}

} // namespace z1m

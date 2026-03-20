#include "game/geometry.hpp"

#include "game/rng.hpp"
#include "game/tuning.hpp"

#include <array>
#include <cmath>
#include <glm/geometric.hpp>

namespace z1m {

namespace {

constexpr float kDiag8 = 0.70710678F;

constexpr std::array<glm::vec2, 8> kDir8Vectors = {
    glm::vec2(1.0F, 0.0F),      glm::vec2(kDiag8, kDiag8),  glm::vec2(0.0F, 1.0F),
    glm::vec2(-kDiag8, kDiag8), glm::vec2(-1.0F, 0.0F),     glm::vec2(-kDiag8, -kDiag8),
    glm::vec2(0.0F, -1.0F),     glm::vec2(kDiag8, -kDiag8),
};

} // namespace

glm::vec2 facing_vector(Facing facing) {
    switch (facing) {
    case Facing::Up:
        return glm::vec2(0.0F, -1.0F);
    case Facing::Down:
        return glm::vec2(0.0F, 1.0F);
    case Facing::Left:
        return glm::vec2(-1.0F, 0.0F);
    case Facing::Right:
        return glm::vec2(1.0F, 0.0F);
    }

    return glm::vec2(0.0F, 1.0F);
}

float qspeed_to_speed(int qspeed) {
    return static_cast<float>(qspeed) * 7.5F / 64.0F;
}

bool near_tile_center(const glm::vec2& position) {
    const float cell_x = std::abs(position.x - (std::floor(position.x) + 0.5F));
    const float cell_y = std::abs(position.y - (std::floor(position.y) + 0.5F));
    return cell_x <= kGridCenterTolerance && cell_y <= kGridCenterTolerance;
}

void snap_to_tile_center(glm::vec2* position) {
    position->x = std::floor(position->x) + 0.5F;
    position->y = std::floor(position->y) + 0.5F;
}

Facing axis_facing_toward(const glm::vec2& from, const glm::vec2& to, bool vertical) {
    if (vertical) {
        return to.y < from.y ? Facing::Up : Facing::Down;
    }

    return to.x < from.x ? Facing::Left : Facing::Right;
}

glm::vec2 eight_way_direction_toward(const glm::vec2& from, const glm::vec2& to) {
    glm::vec2 delta = to - from;
    glm::vec2 direction(0.0F);

    if (std::abs(delta.x) > 0.2F) {
        direction.x = delta.x < 0.0F ? -1.0F : 1.0F;
    }
    if (std::abs(delta.y) > 0.2F) {
        direction.y = delta.y < 0.0F ? -1.0F : 1.0F;
    }

    if (direction.x == 0.0F && direction.y == 0.0F) {
        direction.y = 1.0F;
    }

    if (direction.x == 0.0F) {
        direction.x = delta.x < 0.0F ? -1.0F : 1.0F;
    }

    return glm::normalize(direction);
}

glm::vec2 flyer_direction_toward(const glm::vec2& from, const glm::vec2& to) {
    glm::vec2 delta = to - from;
    glm::vec2 direction(0.0F);

    if (std::abs(delta.x) > 0.2F) {
        direction.x = delta.x < 0.0F ? -1.0F : 1.0F;
    }
    if (std::abs(delta.y) > 0.2F) {
        direction.y = delta.y < 0.0F ? -1.0F : 1.0F;
    }

    if (direction.x == 0.0F && direction.y == 0.0F) {
        return glm::vec2(1.0F, 0.0F);
    }

    return glm::normalize(direction);
}

int dir8_index_from_vector(const glm::vec2& direction) {
    if (glm::length(direction) < 0.001F) {
        return 0;
    }

    float best_dot = -2.0F;
    int best_index = 0;
    const glm::vec2 unit = glm::normalize(direction);
    for (int index = 0; index < static_cast<int>(kDir8Vectors.size()); ++index) {
        const float dot = glm::dot(unit, kDir8Vectors[static_cast<std::size_t>(index)]);
        if (dot > best_dot) {
            best_dot = dot;
            best_index = index;
        }
    }
    return best_index;
}

glm::vec2 rotate_dir8_once_toward(const glm::vec2& current, const glm::vec2& target) {
    const int current_index = dir8_index_from_vector(current);
    const int target_index = dir8_index_from_vector(target);
    const int clockwise = (target_index - current_index + 8) % 8;
    const int counter_clockwise = (current_index - target_index + 8) % 8;
    if (clockwise == 0 || counter_clockwise == 0) {
        return kDir8Vectors[static_cast<std::size_t>(current_index)];
    }

    if (clockwise <= counter_clockwise) {
        return kDir8Vectors[static_cast<std::size_t>((current_index + 1) % 8)];
    }

    return kDir8Vectors[static_cast<std::size_t>((current_index + 7) % 8)];
}

glm::vec2 rotate_dir8_random(Play* play, const glm::vec2& current) {
    const int current_index = dir8_index_from_vector(current);
    const int roll = random_int(play, 256);
    if (roll >= 0xA0) {
        return kDir8Vectors[static_cast<std::size_t>(current_index)];
    }
    if (roll >= 0x50) {
        return kDir8Vectors[static_cast<std::size_t>((current_index + 1) % 8)];
    }
    return kDir8Vectors[static_cast<std::size_t>((current_index + 7) % 8)];
}

bool choose_cardinal_shot_direction(const Play* play, const Enemy& enemy, const Player& player,
                                    Facing* facing_out) {
    if (enemy.area_kind == AreaKind::Overworld && enemy.room_id != play->current_room_id) {
        return false;
    }

    const glm::vec2 delta = player.position - enemy.position;
    if (glm::length(delta) > kShootMaxDistance) {
        return false;
    }

    if (std::abs(delta.x) <= kShootAlignThreshold) {
        *facing_out = delta.y < 0.0F ? Facing::Up : Facing::Down;
        return true;
    }

    if (std::abs(delta.y) <= kShootAlignThreshold) {
        *facing_out = delta.x < 0.0F ? Facing::Left : Facing::Right;
        return true;
    }

    return false;
}

bool overlaps_circle(const glm::vec2& a, const glm::vec2& b, float radius) {
    return glm::length(a - b) <= radius;
}

} // namespace z1m

#pragma once

#include <glm/vec2.hpp>

namespace z1m {

struct World;

enum class Facing {
    Up,
    Down,
    Left,
    Right,
};

enum class MoveDirection {
    None,
    Up,
    Down,
    Left,
    Right,
};

struct PlayerCommand {
    glm::vec2 move_axis = glm::vec2(0.0F, 0.0F);
    bool attack_pressed = false;
};

struct Player {
    glm::vec2 position = glm::vec2(6.5F, 5.5F);
    Facing facing = Facing::Down;
    MoveDirection move_direction = MoveDirection::None;
    float sword_seconds_remaining = 0.0F;
    float invincibility_seconds = 0.0F;
    int max_health = 3;
    int health = 3;
    int rupees = 0;
    int bombs = 0;
    bool has_sword = false;
};

Player make_player();
void tick_player(Player* player, const World* world, const PlayerCommand* command,
                 float dt_seconds);
bool is_sword_active(const Player* player);
glm::vec2 sword_world_position(const Player* player);

} // namespace z1m

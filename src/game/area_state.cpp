#include "game/area_state.hpp"

namespace z1m {

void set_message(GameState* play, const std::string& text, float seconds) {
    play->message_text = text;
    play->message_seconds_remaining = seconds;
}

Projectile* find_active_food(GameState* play, AreaKind area_kind, int cave_id) {
    for (Projectile& projectile : play->projectiles) {
        if (!projectile.active || projectile.kind != ProjectileKind::Food ||
            projectile.area_kind != area_kind || projectile.cave_id != cave_id) {
            continue;
        }

        return &projectile;
    }

    return nullptr;
}

const World* get_world_for_area(const GameState* play, const World* overworld_world,
                                AreaKind area_kind, int cave_id) {
    switch (area_kind) {
    case AreaKind::Overworld:
        return overworld_world;
    case AreaKind::Cave:
        if (play->current_cave_id == cave_id || cave_id >= 0) {
            return &play->cave_world;
        }
        return &play->cave_world;
    case AreaKind::EnemyZoo:
        return &play->enemy_zoo_world;
    case AreaKind::ItemZoo:
        return &play->item_zoo_world;
    }

    return overworld_world;
}

bool in_area(const GameState* play, AreaKind area_kind, int cave_id) {
    if (play->area_kind != area_kind) {
        return false;
    }

    if (area_kind == AreaKind::Cave) {
        return play->current_cave_id == cave_id;
    }

    return true;
}

bool enemy_in_current_area(const GameState* play, const Enemy& enemy) {
    return enemy.active && in_area(play, enemy.area_kind, enemy.cave_id);
}

bool pickup_in_current_area(const GameState* play, const Pickup& pickup) {
    return pickup.active && in_area(play, pickup.area_kind, pickup.cave_id);
}

int get_room_from_position(const glm::vec2& position) {
    return get_room_id_at_world_tile(static_cast<int>(position.x), static_cast<int>(position.y));
}

void update_current_room(GameState* play, const Player* player) {
    if (play->area_kind != AreaKind::Overworld) {
        play->current_room_id = -1;
        play->previous_room_id = -1;
        return;
    }

    play->current_room_id = get_room_from_position(player->position);
    if (play->current_room_id != play->previous_room_id) {
        play->previous_room_id = play->current_room_id;
    }
}

} // namespace z1m

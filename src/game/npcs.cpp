#include "game/npcs.hpp"

#include "game/area_state.hpp"
#include "game/geometry.hpp"

#include <cmath>
#include <glm/common.hpp>

namespace z1m {

void resolve_npc_collisions(const Play* play, Player* player, const glm::vec2& previous_position) {
    for (const Npc& npc : play->npcs) {
        if (!npc.active || npc.solved || !in_area(play, npc.area_kind, npc.cave_id)) {
            continue;
        }

        if (npc.kind != NpcKind::HungryGoriya) {
            continue;
        }

        if (!overlaps_circle(player->position, npc.position, 0.95F)) {
            continue;
        }

        player->position = previous_position;
        return;
    }
}

void tick_npcs(Play* play, Player* player, float dt_seconds) {
    for (Npc& npc : play->npcs) {
        if (!npc.active) {
            continue;
        }

        npc.action_seconds_remaining = glm::max(0.0F, npc.action_seconds_remaining - dt_seconds);

        if (npc.kind == NpcKind::Fairy) {
            npc.state_seconds_remaining += dt_seconds;
            const float phase = npc.state_seconds_remaining * 6.0F;
            npc.position = npc.origin + glm::vec2(std::cos(phase) * 0.55F, std::sin(phase) * 0.35F);
            if (in_area(play, npc.area_kind, npc.cave_id) &&
                overlaps_circle(player->position, npc.position, 0.8F)) {
                player->health = player->max_health;
                set_message(play, "fairy healed", 1.0F);
            }
            continue;
        }

        npc.state_seconds_remaining = glm::max(0.0F, npc.state_seconds_remaining - dt_seconds);

        if (npc.kind != NpcKind::HungryGoriya || npc.solved) {
            continue;
        }

        Projectile* food = find_active_food(play, npc.area_kind, npc.cave_id);
        if (food == nullptr || !overlaps_circle(food->position, npc.position, 1.2F)) {
            continue;
        }

        food->active = false;
        npc.solved = true;
        npc.active = false;
        set_message(play, "hungry goriya ate bait", 1.4F);
    }
}

void update_npc_messages(Play* play, const Player* player) {
    if (play->message_seconds_remaining > 0.0F) {
        return;
    }

    for (const Npc& npc : play->npcs) {
        if (!npc.active || !in_area(play, npc.area_kind, npc.cave_id)) {
            continue;
        }

        if (!overlaps_circle(player->position, npc.position, 1.8F)) {
            continue;
        }

        if (npc.kind == NpcKind::ShopKeeper) {
            set_message(play, "shop: touch item to buy", 0.2F);
        } else if (npc.kind == NpcKind::OldWoman) {
            set_message(play, player->has_letter ? "potion shop is open" : "show me the letter",
                        0.2F);
        } else if (npc.kind == NpcKind::HungryGoriya) {
            set_message(play, "hungry goriya needs bait", 0.2F);
        } else if (npc.kind == NpcKind::Fairy) {
            set_message(play, "fairy restores hearts", 0.2F);
        } else {
            set_message(play, npc.label, 0.2F);
        }
        return;
    }
}

} // namespace z1m

#include "game/enemy_state.hpp"

#include "content/sandbox_content.hpp"
#include "game/enemy_ticks.hpp"
#include "game/rng.hpp"
#include "game/tuning.hpp"

#include <glm/common.hpp>

namespace z1m {

void clamp_enemy_to_zoo_pen(Enemy* enemy) {
    if (enemy->area_kind != AreaKind::EnemyZoo || enemy->respawn_group < 0) {
        return;
    }

    glm::vec2 min_position(0.0F);
    glm::vec2 max_position(0.0F);
    if (!get_enemy_zoo_pen_bounds(enemy->respawn_group, &min_position, &max_position)) {
        return;
    }

    enemy->position.x = glm::clamp(enemy->position.x, min_position.x, max_position.x);
    enemy->position.y = glm::clamp(enemy->position.y, min_position.y, max_position.y);
}

void reset_enemy_state(GameState* play, Enemy* enemy) {
    enemy->health = glm::max(enemy->max_health, 1);
    enemy->hurt_seconds_remaining = 0.0F;
    enemy->hidden = false;
    enemy->invulnerable = false;
    enemy->velocity = glm::vec2(0.0F, 0.0F);
    enemy->special_counter = 0;

    switch (enemy->kind) {
    case EnemyKind::Octorok:
        enemy->move_seconds_remaining = 0.25F + random_unit(play) * 0.40F;
        enemy->action_seconds_remaining = 0.70F + random_unit(play) * 0.90F;
        break;
    case EnemyKind::Moblin:
        enemy->move_seconds_remaining = 0.25F + random_unit(play) * 0.45F;
        enemy->action_seconds_remaining = 0.55F + random_unit(play) * 0.80F;
        break;
    case EnemyKind::Lynel:
    case EnemyKind::Goriya:
    case EnemyKind::Darknut:
        enemy->move_seconds_remaining = 0.25F + random_unit(play) * 0.35F;
        enemy->action_seconds_remaining = 0.45F + random_unit(play) * 0.65F;
        break;
    case EnemyKind::Tektite:
        enemy->move_seconds_remaining = 0.0F;
        enemy->action_seconds_remaining = 0.25F + random_unit(play) * 0.35F;
        break;
    case EnemyKind::Leever:
        enemy->hidden = true;
        enemy->invulnerable = true;
        enemy->special_counter = 0;
        enemy->action_seconds_remaining =
            enemy->subtype == 0 ? frames_to_seconds(20) : frames_to_seconds(0x80);
        enemy->move_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Keese:
    case EnemyKind::Ghini:
        enemy->special_counter = 0;
        enemy->action_seconds_remaining = 0.25F + random_unit(play) * 0.30F;
        enemy->move_seconds_remaining = 1.0F + random_unit(play) * 1.2F;
        break;
    case EnemyKind::Bubble:
        enemy->invulnerable = true;
        enemy->action_seconds_remaining = 0.18F + random_unit(play) * 0.25F;
        enemy->move_seconds_remaining = 1.0F + random_unit(play) * 1.0F;
        break;
    case EnemyKind::FlyingGhini:
        enemy->action_seconds_remaining = 0.15F + random_unit(play) * 0.20F;
        enemy->move_seconds_remaining = 1.0F + random_unit(play) * 1.2F;
        break;
    case EnemyKind::Zol:
        enemy->special_counter = 0;
        enemy->move_seconds_remaining = 0.0F;
        enemy->action_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Gel:
        enemy->special_counter = enemy->subtype == 1 ? 0 : 2;
        enemy->move_seconds_remaining = 0.0F;
        enemy->action_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Vire:
    case EnemyKind::Stalfos:
    case EnemyKind::Gibdo:
    case EnemyKind::LikeLike:
    case EnemyKind::PolsVoice:
        enemy->move_seconds_remaining = 0.25F + random_unit(play) * 0.45F;
        enemy->action_seconds_remaining = 0.35F + random_unit(play) * 0.55F;
        break;
    case EnemyKind::Rope:
        enemy->special_counter = 0;
        enemy->move_seconds_remaining = 0.0F;
        enemy->action_seconds_remaining = frames_to_seconds(random_byte(play) & 0x3F);
        break;
    case EnemyKind::Armos:
        enemy->special_counter = 0;
        enemy->move_seconds_remaining = 0.0F;
        enemy->action_seconds_remaining = 0.0F;
        enemy->facing = Facing::Down;
        break;
    case EnemyKind::Wallmaster:
        enemy->hidden = true;
        enemy->action_seconds_remaining = 0.8F + random_unit(play) * 0.8F;
        enemy->move_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Trap:
        enemy->invulnerable = true;
        enemy->velocity = glm::vec2(0.0F);
        enemy->state_seconds_remaining = 0.0F;
        enemy->action_seconds_remaining = 0.0F;
        enemy->move_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Zora:
        enemy->hidden = true;
        enemy->state_seconds_remaining = 1.0F + random_unit(play) * 0.8F;
        enemy->action_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Peahat:
        enemy->invulnerable = true;
        enemy->state_seconds_remaining = 1.8F + random_unit(play) * 0.8F;
        enemy->action_seconds_remaining = 0.15F;
        enemy->move_seconds_remaining = 0.55F;
        break;
    case EnemyKind::BlueWizzrobe:
        enemy->action_seconds_remaining = frames_to_seconds(0x70 | random_byte(play));
        enemy->move_seconds_remaining = 0.0F;
        enemy->state_seconds_remaining = 0.0F;
        enemy->facing = Facing::Right;
        break;
    case EnemyKind::RedWizzrobe:
        enemy->hidden = true;
        enemy->special_counter = 0;
        enemy->action_seconds_remaining = 0.0F;
        enemy->state_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Dodongo:
        enemy->facing = Facing::Right;
        enemy->move_seconds_remaining = 0.9F + random_unit(play) * 0.6F;
        enemy->action_seconds_remaining = 0.0F;
        enemy->state_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Digdogger:
        enemy->velocity = glm::vec2(kDigdoggerSpeed, 0.0F);
        enemy->action_seconds_remaining = 0.45F;
        enemy->move_seconds_remaining = 9999.0F;
        break;
    case EnemyKind::Manhandla:
        enemy->special_counter = 4;
        enemy->velocity = glm::vec2(kManhandlaSpeed, kManhandlaSpeed * 0.65F);
        enemy->action_seconds_remaining = 1.0F;
        enemy->move_seconds_remaining = 9999.0F;
        break;
    case EnemyKind::Gohma:
        enemy->facing = Facing::Right;
        enemy->velocity = glm::vec2(enemy->subtype == 0 ? kGohmaSpeed : kGohmaSpeed * 1.15F, 0.0F);
        enemy->action_seconds_remaining = enemy->subtype == 0 ? 1.0F : 0.7F;
        enemy->state_seconds_remaining =
            enemy->subtype == 0 ? kGohmaEyeClosedSeconds : kGohmaEyeClosedSeconds * 0.7F;
        break;
    case EnemyKind::Moldorm:
        enemy->facing = Facing::Right;
        enemy->action_seconds_remaining = 0.3F + random_unit(play) * 0.4F;
        enemy->move_seconds_remaining = 9999.0F;
        enemy->velocity = enemy->subtype == 0 ? glm::vec2(kMoldormSpeed, 0.0F) : glm::vec2(0.0F);
        break;
    case EnemyKind::Aquamentus:
        enemy->velocity = glm::vec2(kAquamentusSpeed, 0.0F);
        enemy->action_seconds_remaining = 0.9F;
        enemy->move_seconds_remaining = 9999.0F;
        break;
    case EnemyKind::Gleeok:
        enemy->velocity = glm::vec2(kGleeokSpeed, 0.0F);
        enemy->action_seconds_remaining = 0.8F;
        enemy->move_seconds_remaining = 9999.0F;
        enemy->special_counter = enemy->subtype > 0 ? enemy->subtype : 3;
        break;
    case EnemyKind::Patra:
        enemy->velocity = glm::vec2(kPatraSpeed, 0.0F);
        enemy->action_seconds_remaining = 0.0F;
        enemy->state_seconds_remaining = 4.0F;
        enemy->special_counter = 8;
        break;
    case EnemyKind::Ganon:
        enemy->hidden = true;
        enemy->action_seconds_remaining = 0.6F;
        enemy->state_seconds_remaining = kWizzrobeTeleportSeconds;
        break;
    }
}

} // namespace z1m

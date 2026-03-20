namespace {

constexpr float kEnemyTouchRadius = 0.60F;
constexpr float kProjectileHitPadding = 0.35F;
constexpr float kPickupRadius = 0.65F;
constexpr float kSwordPickupRadius = 0.80F;
constexpr float kSwordHitRadius = 0.95F;
constexpr float kPlayerDamageInvincibilitySeconds = 1.0F;
constexpr float kAreaTransitionCooldownSeconds = 0.25F;
constexpr float kBombFuseSeconds = 1.0F;
constexpr float kExplosionSeconds = 0.22F;
constexpr float kBoomerangFlightSeconds = 0.65F;
constexpr float kFireSeconds = 0.55F;
constexpr float kFoodSeconds = 5.0F;
constexpr float kProjectileLifetimeSeconds = 2.5F;
constexpr float kPickupLifetimeSeconds = 8.0F;
constexpr float kPickupGravityTilesPerSecond = 10.0F;
constexpr float kBombExplosionRadius = 1.55F;
constexpr float kBoomerangRadius = 0.42F;
constexpr float kArrowRadius = 0.24F;
constexpr float kFireRadius = 0.44F;
constexpr float kRockRadius = 0.30F;
constexpr float kBombRadius = 0.42F;
constexpr float kExplosionRadius = 1.10F;
constexpr float kShootAlignThreshold = 1.0F;
constexpr float kWandererTargetThreshold = 1.125F;
constexpr float kGridCenterTolerance = 0.12F;
constexpr float kShootMaxDistance = 14.0F;
constexpr float kOctorokSpeed = 4.5F;
constexpr float kMoblinSpeed = 5.0F;
constexpr float kGoriyaSpeed = 4.8F;
constexpr float kDarknutSpeed = 4.2F;
constexpr float kGelSpeed = 2.8F;
constexpr float kZolSpeed = 3.6F;
constexpr float kRopeSpeed = 4.2F;
constexpr float kVireSpeed = 5.8F;
constexpr float kWizzrobeSpeed = 4.0F;
constexpr float kRopeChargeSpeed = 10.0F;
constexpr float kWalkerSpeed = 3.8F;
constexpr float kLikeLikeSpeed = 2.6F;
constexpr float kPolsVoiceSpeed = 6.8F;
constexpr float kWallmasterSpeed = 6.2F;
constexpr float kGhiniSpeed = 4.4F;
constexpr float kFlyingGhiniSpeed = 6.0F;
constexpr float kTrapSpeed = 13.0F;
constexpr float kArmosSpeed = 4.0F;
constexpr float kDodongoSpeed = 3.2F;
constexpr float kDigdoggerSpeed = 4.6F;
constexpr float kManhandlaSpeed = 4.8F;
constexpr float kGohmaSpeed = 5.2F;
constexpr float kMoldormSpeed = 6.0F;
constexpr float kKeeseSpeed = 5.7F;
constexpr float kLeeverSpeed = 4.2F;
constexpr float kTektiteHopSpeed = 7.2F;
constexpr float kAquamentusSpeed = 3.0F;
constexpr float kGleeokSpeed = 2.4F;
constexpr float kPatraSpeed = 5.4F;
constexpr float kGanonSpeed = 4.2F;
constexpr float kLynelSpeed = 5.7F;
constexpr float kPeahatSpeed = 6.6F;
constexpr float kRockSpeed = 8.0F;
constexpr float kArrowSpeed = 12.5F;
constexpr float kSwordBeamSpeed = 11.5F;
constexpr float kBoomerangSpeed = 9.2F;
constexpr float kFireSpeed = 7.5F;
constexpr float kLikeLikeGrabSeconds = 0.25F;
constexpr float kBubbleCurseSeconds = 3.0F;
constexpr float kDodongoBloatedSeconds = 1.2F;
constexpr float kDodongoStunnedSeconds = 0.8F;
constexpr float kGohmaEyeClosedSeconds = 1.1F;
constexpr float kGohmaEyeOpenSeconds = 0.8F;
constexpr float kWizzrobeTeleportSeconds = 0.55F;
constexpr float kPatraOrbitRadiusWide = 2.3F;
constexpr float kPatraOrbitRadiusTight = 1.25F;
constexpr int kSwordCaveId = 0x10;

void update_current_room(GameSession* session, const Player* player);

std::uint32_t next_random(GameSession* session) {
    session->rng_state = session->rng_state * 1664525U + 1013904223U;
    return session->rng_state;
}

float random_unit(GameSession* session) {
    const std::uint32_t value = next_random(session) >> 8;
    return static_cast<float>(value & 0x00FFFFFFU) / static_cast<float>(0x01000000U);
}

int random_int(GameSession* session, int max_value) {
    if (max_value <= 0) {
        return 0;
    }

    return static_cast<int>(next_random(session) % static_cast<std::uint32_t>(max_value));
}

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

constexpr float kDiag8 = 0.70710678F;

const std::array<glm::vec2, 8> kDir8Vectors = {
    glm::vec2(1.0F, 0.0F),      glm::vec2(kDiag8, kDiag8),  glm::vec2(0.0F, 1.0F),
    glm::vec2(-kDiag8, kDiag8), glm::vec2(-1.0F, 0.0F),     glm::vec2(-kDiag8, -kDiag8),
    glm::vec2(0.0F, -1.0F),     glm::vec2(kDiag8, -kDiag8),
};

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

glm::vec2 rotate_dir8_random(GameSession* session, const glm::vec2& current) {
    const int current_index = dir8_index_from_vector(current);
    const int roll = random_int(session, 256);
    if (roll >= 0xA0) {
        return kDir8Vectors[static_cast<std::size_t>(current_index)];
    }
    if (roll >= 0x50) {
        return kDir8Vectors[static_cast<std::size_t>((current_index + 1) % 8)];
    }
    return kDir8Vectors[static_cast<std::size_t>((current_index + 7) % 8)];
}

bool choose_cardinal_shot_direction(const GameSession* session, const Enemy& enemy,
                                    const Player& player, Facing* facing_out) {
    if (enemy.area_kind == AreaKind::Overworld && enemy.room_id != session->current_room_id) {
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

void set_message(GameSession* session, const std::string& text, float seconds) {
    session->message_text = text;
    session->message_seconds_remaining = seconds;
}

Projectile* find_active_food(GameSession* session, AreaKind area_kind, int cave_id) {
    for (Projectile& projectile : session->projectiles) {
        if (!projectile.active || projectile.kind != ProjectileKind::Food ||
            projectile.area_kind != area_kind || projectile.cave_id != cave_id) {
            continue;
        }
        return &projectile;
    }
    return nullptr;
}

const World* get_world_for_area(const GameSession* session, const World* overworld_world,
                                AreaKind area_kind, int cave_id) {
    switch (area_kind) {
    case AreaKind::Overworld:
        return overworld_world;
    case AreaKind::Cave:
        if (session->current_cave_id == cave_id || cave_id >= 0) {
            return &session->cave_world;
        }
        return &session->cave_world;
    case AreaKind::EnemyZoo:
        return &session->enemy_zoo_world;
    case AreaKind::ItemZoo:
        return &session->item_zoo_world;
    }

    return overworld_world;
}

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

bool in_area(const GameSession* session, AreaKind area_kind, int cave_id) {
    if (session->area_kind != area_kind) {
        return false;
    }

    if (area_kind == AreaKind::Cave) {
        return session->current_cave_id == cave_id;
    }

    return true;
}

bool enemy_in_current_area(const GameSession* session, const Enemy& enemy) {
    return enemy.active && in_area(session, enemy.area_kind, enemy.cave_id);
}

bool pickup_in_current_area(const GameSession* session, const Pickup& pickup) {
    return pickup.active && in_area(session, pickup.area_kind, pickup.cave_id);
}

int get_room_from_position(const glm::vec2& position) {
    return get_room_id_at_world_tile(static_cast<int>(position.x), static_cast<int>(position.y));
}

void reset_enemy_state(GameSession* session, Enemy* enemy) {
    enemy->health = glm::max(enemy->max_health, 1);
    enemy->hurt_seconds_remaining = 0.0F;
    enemy->hidden = false;
    enemy->invulnerable = false;
    enemy->velocity = glm::vec2(0.0F, 0.0F);
    enemy->special_counter = 0;

    switch (enemy->kind) {
    case EnemyKind::Octorok:
        enemy->move_seconds_remaining = 0.25F + random_unit(session) * 0.40F;
        enemy->action_seconds_remaining = 0.70F + random_unit(session) * 0.90F;
        break;
    case EnemyKind::Moblin:
        enemy->move_seconds_remaining = 0.25F + random_unit(session) * 0.45F;
        enemy->action_seconds_remaining = 0.55F + random_unit(session) * 0.80F;
        break;
    case EnemyKind::Lynel:
    case EnemyKind::Goriya:
    case EnemyKind::Darknut:
        enemy->move_seconds_remaining = 0.25F + random_unit(session) * 0.35F;
        enemy->action_seconds_remaining = 0.45F + random_unit(session) * 0.65F;
        break;
    case EnemyKind::Tektite:
        enemy->move_seconds_remaining = 0.0F;
        enemy->action_seconds_remaining = 0.25F + random_unit(session) * 0.35F;
        break;
    case EnemyKind::Leever:
        enemy->hidden = true;
        enemy->state_seconds_remaining = 0.4F + random_unit(session) * 0.4F;
        enemy->action_seconds_remaining = 0.0F;
        enemy->move_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Keese:
    case EnemyKind::Ghini:
        enemy->action_seconds_remaining = 0.25F + random_unit(session) * 0.30F;
        enemy->move_seconds_remaining = 1.0F + random_unit(session) * 1.2F;
        break;
    case EnemyKind::Bubble:
        enemy->invulnerable = true;
        enemy->action_seconds_remaining = 0.18F + random_unit(session) * 0.25F;
        enemy->move_seconds_remaining = 1.0F + random_unit(session) * 1.0F;
        break;
    case EnemyKind::FlyingGhini:
        enemy->action_seconds_remaining = 0.15F + random_unit(session) * 0.20F;
        enemy->move_seconds_remaining = 1.0F + random_unit(session) * 1.2F;
        break;
    case EnemyKind::Zol:
    case EnemyKind::Gel:
    case EnemyKind::Vire:
    case EnemyKind::Stalfos:
    case EnemyKind::Gibdo:
    case EnemyKind::LikeLike:
    case EnemyKind::PolsVoice:
    case EnemyKind::Rope:
    case EnemyKind::Armos:
        enemy->move_seconds_remaining = 0.25F + random_unit(session) * 0.45F;
        enemy->action_seconds_remaining = 0.35F + random_unit(session) * 0.55F;
        break;
    case EnemyKind::Wallmaster:
        enemy->hidden = true;
        enemy->action_seconds_remaining = 0.8F + random_unit(session) * 0.8F;
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
        enemy->state_seconds_remaining = 1.0F + random_unit(session) * 0.8F;
        enemy->action_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Peahat:
        enemy->invulnerable = true;
        enemy->state_seconds_remaining = 1.8F + random_unit(session) * 0.8F;
        enemy->action_seconds_remaining = 0.15F;
        enemy->move_seconds_remaining = 0.55F;
        break;
    case EnemyKind::BlueWizzrobe:
        enemy->action_seconds_remaining = 1.1F + random_unit(session) * 0.6F;
        enemy->move_seconds_remaining = 0.35F;
        break;
    case EnemyKind::RedWizzrobe:
        enemy->hidden = true;
        enemy->state_seconds_remaining = 0.9F + random_unit(session) * 0.7F;
        enemy->action_seconds_remaining = 0.0F;
        break;
    case EnemyKind::Dodongo:
        enemy->facing = Facing::Right;
        enemy->move_seconds_remaining = 0.9F + random_unit(session) * 0.6F;
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
            (enemy->subtype == 0 ? kGohmaEyeClosedSeconds : kGohmaEyeClosedSeconds * 0.7F);
        break;
    case EnemyKind::Moldorm:
        enemy->facing = Facing::Right;
        enemy->action_seconds_remaining = 0.3F + random_unit(session) * 0.4F;
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

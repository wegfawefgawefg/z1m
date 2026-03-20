#pragma once

#include "game/entities.hpp"

namespace z1m {

const char* pickup_name(PickupKind kind);
const char* enemy_name(EnemyKind kind);
const char* projectile_name(ProjectileKind kind);
const char* npc_name(NpcKind kind);

} // namespace z1m

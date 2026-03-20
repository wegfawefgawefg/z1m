#include "game/entity_names.hpp"

namespace z1m {

const char* pickup_name(PickupKind kind) {
    switch (kind) {
    case PickupKind::None:
        return "none";
    case PickupKind::Sword:
        return "sword";
    case PickupKind::Heart:
        return "heart";
    case PickupKind::Rupee:
        return "rupee";
    case PickupKind::Bombs:
        return "bombs";
    case PickupKind::Boomerang:
        return "boomerang";
    case PickupKind::Bow:
        return "bow";
    case PickupKind::Candle:
        return "candle";
    case PickupKind::BluePotion:
        return "blue-potion";
    case PickupKind::HeartContainer:
        return "heart-container";
    case PickupKind::Key:
        return "key";
    case PickupKind::Recorder:
        return "recorder";
    case PickupKind::Ladder:
        return "ladder";
    case PickupKind::Raft:
        return "raft";
    case PickupKind::Food:
        return "food";
    case PickupKind::Letter:
        return "letter";
    case PickupKind::MagicShield:
        return "magic-shield";
    case PickupKind::SilverArrows:
        return "silver-arrows";
    }

    return "none";
}

const char* enemy_name(EnemyKind kind) {
    switch (kind) {
    case EnemyKind::Octorok:
        return "octorok";
    case EnemyKind::Moblin:
        return "moblin";
    case EnemyKind::Lynel:
        return "lynel";
    case EnemyKind::Goriya:
        return "goriya";
    case EnemyKind::Darknut:
        return "darknut";
    case EnemyKind::Tektite:
        return "tektite";
    case EnemyKind::Leever:
        return "leever";
    case EnemyKind::Keese:
        return "keese";
    case EnemyKind::Zol:
        return "zol";
    case EnemyKind::Gel:
        return "gel";
    case EnemyKind::Rope:
        return "rope";
    case EnemyKind::Vire:
        return "vire";
    case EnemyKind::Stalfos:
        return "stalfos";
    case EnemyKind::Gibdo:
        return "gibdo";
    case EnemyKind::LikeLike:
        return "like-like";
    case EnemyKind::PolsVoice:
        return "pols-voice";
    case EnemyKind::BlueWizzrobe:
        return "blue-wizzrobe";
    case EnemyKind::RedWizzrobe:
        return "red-wizzrobe";
    case EnemyKind::Wallmaster:
        return "wallmaster";
    case EnemyKind::Ghini:
        return "ghini";
    case EnemyKind::FlyingGhini:
        return "flying-ghini";
    case EnemyKind::Bubble:
        return "bubble";
    case EnemyKind::Trap:
        return "trap";
    case EnemyKind::Armos:
        return "armos";
    case EnemyKind::Zora:
        return "zora";
    case EnemyKind::Peahat:
        return "peahat";
    case EnemyKind::Dodongo:
        return "dodongo";
    case EnemyKind::Digdogger:
        return "digdogger";
    case EnemyKind::Manhandla:
        return "manhandla";
    case EnemyKind::Gohma:
        return "gohma";
    case EnemyKind::Moldorm:
        return "moldorm";
    case EnemyKind::Aquamentus:
        return "aquamentus";
    case EnemyKind::Gleeok:
        return "gleeok";
    case EnemyKind::Patra:
        return "patra";
    case EnemyKind::Ganon:
        return "ganon";
    }

    return "enemy";
}

const char* projectile_name(ProjectileKind kind) {
    switch (kind) {
    case ProjectileKind::Rock:
        return "rock";
    case ProjectileKind::Arrow:
        return "arrow";
    case ProjectileKind::SwordBeam:
        return "sword-beam";
    case ProjectileKind::Boomerang:
        return "boomerang";
    case ProjectileKind::Fire:
        return "fire";
    case ProjectileKind::Food:
        return "food";
    case ProjectileKind::Bomb:
        return "bomb";
    case ProjectileKind::Explosion:
        return "explosion";
    }

    return "projectile";
}

const char* npc_name(NpcKind kind) {
    switch (kind) {
    case NpcKind::OldMan:
        return "old-man";
    case NpcKind::ShopKeeper:
        return "shopkeeper";
    case NpcKind::HungryGoriya:
        return "hungry-goriya";
    case NpcKind::OldWoman:
        return "old-woman";
    case NpcKind::Fairy:
        return "fairy";
    }

    return "npc";
}

} // namespace z1m

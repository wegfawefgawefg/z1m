#include "content/sandbox_content.hpp"

#include "content/opening_content.hpp"

namespace z1m {

namespace {

struct EnemySpawnSpec {
    AreaKind area_kind = AreaKind::EnemyZoo;
    EnemyKind kind = EnemyKind::Octorok;
    glm::vec2 position = glm::vec2(0.0F, 0.0F);
    int health = 1;
};

struct PickupSpawnSpec {
    AreaKind area_kind = AreaKind::ItemZoo;
    PickupKind kind = PickupKind::None;
    glm::vec2 position = glm::vec2(0.0F, 0.0F);
    int price_rupees = 0;
    bool shop_item = false;
};

struct NpcSpawnSpec {
    AreaKind area_kind = AreaKind::ItemZoo;
    NpcKind kind = NpcKind::OldMan;
    glm::vec2 position = glm::vec2(0.0F, 0.0F);
    int shop_item_index = -1;
    const char* label = "";
};

constexpr std::array<EnemySpawnSpec, 13> kEnemyZooSpawns = {
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Octorok,
                   .position = glm::vec2(16.0F, 12.0F),
                   .health = 1},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Octorok,
                   .position = glm::vec2(20.0F, 18.0F),
                   .health = 1},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Moblin,
                   .position = glm::vec2(34.0F, 12.0F),
                   .health = 2},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Moblin,
                   .position = glm::vec2(38.0F, 18.0F),
                   .health = 2},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Tektite,
                   .position = glm::vec2(54.0F, 14.0F),
                   .health = 1},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Tektite,
                   .position = glm::vec2(58.0F, 20.0F),
                   .health = 1},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Leever,
                   .position = glm::vec2(16.0F, 38.0F),
                   .health = 2},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Leever,
                   .position = glm::vec2(20.0F, 44.0F),
                   .health = 2},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Keese,
                   .position = glm::vec2(36.0F, 40.0F),
                   .health = 1},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Keese,
                   .position = glm::vec2(42.0F, 44.0F),
                   .health = 1},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Keese,
                   .position = glm::vec2(46.0F, 38.0F),
                   .health = 1},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Aquamentus,
                   .position = glm::vec2(72.0F, 40.0F),
                   .health = 6},
    EnemySpawnSpec{.area_kind = AreaKind::Overworld,
                   .kind = EnemyKind::Octorok,
                   .position = glm::vec2(232.0F, 162.0F),
                   .health = 1},
};

constexpr std::array<PickupSpawnSpec, 18> kItemZooPickups = {
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Rupee,
                    .position = glm::vec2(12.0F, 12.0F)},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Rupee,
                    .position = glm::vec2(14.5F, 12.0F)},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Rupee,
                    .position = glm::vec2(12.0F, 15.0F)},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Rupee,
                    .position = glm::vec2(14.5F, 15.0F)},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Rupee,
                    .position = glm::vec2(12.0F, 28.0F)},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Rupee,
                    .position = glm::vec2(14.5F, 28.0F)},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Rupee,
                    .position = glm::vec2(18.0F, 28.0F)},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Bombs,
                    .position = glm::vec2(18.0F, 12.0F)},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Boomerang,
                    .position = glm::vec2(24.0F, 12.0F)},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Bow,
                    .position = glm::vec2(30.0F, 12.0F)},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Candle,
                    .position = glm::vec2(36.0F, 12.0F)},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::BluePotion,
                    .position = glm::vec2(42.0F, 12.0F)},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::HeartContainer,
                    .position = glm::vec2(48.0F, 12.0F)},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Key,
                    .position = glm::vec2(54.0F, 12.0F)},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Recorder,
                    .position = glm::vec2(22.0F, 26.0F),
                    .price_rupees = 10,
                    .shop_item = true},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Ladder,
                    .position = glm::vec2(28.0F, 26.0F),
                    .price_rupees = 15,
                    .shop_item = true},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Raft,
                    .position = glm::vec2(34.0F, 26.0F),
                    .price_rupees = 20,
                    .shop_item = true},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Heart,
                    .position = glm::vec2(60.0F, 12.0F)},
};

constexpr std::array<NpcSpawnSpec, 3> kNpcSpawns = {
    NpcSpawnSpec{.area_kind = AreaKind::ItemZoo,
                 .kind = NpcKind::ShopKeeper,
                 .position = glm::vec2(28.0F, 21.0F),
                 .shop_item_index = -1,
                 .label = "item shop"},
    NpcSpawnSpec{.area_kind = AreaKind::ItemZoo,
                 .kind = NpcKind::OldMan,
                 .position = glm::vec2(46.0F, 21.0F),
                 .shop_item_index = -1,
                 .label = "take anything you want"},
    NpcSpawnSpec{.area_kind = AreaKind::EnemyZoo,
                 .kind = NpcKind::OldMan,
                 .position = glm::vec2(74.0F, 10.0F),
                 .shop_item_index = -1,
                 .label = "boss pen ahead"},
};

void build_box_walls(World* world) {
    fill_world_rect(world, 0, 0, world->width, 1, TileKind::Wall);
    fill_world_rect(world, 0, world->height - 1, world->width, 1, TileKind::Wall);
    fill_world_rect(world, 0, 0, 1, world->height, TileKind::Wall);
    fill_world_rect(world, world->width - 1, 0, 1, world->height, TileKind::Wall);
}

void push_portal(std::array<AreaPortal, kMaxAreaPortals>* portals, int* count, AreaKind source_area,
                 const glm::vec2& center, const glm::vec2& half_size, AreaKind target_area,
                 const glm::vec2& target_position, const char* label) {
    if (*count >= kMaxAreaPortals) {
        return;
    }

    AreaPortal& portal = (*portals)[static_cast<std::size_t>(*count)];
    portal = AreaPortal{};
    portal.source_area_kind = source_area;
    portal.center = center;
    portal.half_size = half_size;
    portal.target_area_kind = target_area;
    portal.target_position = target_position;
    portal.label = label;
    ++(*count);
}

} // namespace

void build_enemy_zoo_world(World* world) {
    resize_world(world, 96, 64);
    fill_world(world, TileKind::Ground);
    build_box_walls(world);

    fill_world_rect(world, 8, 8, 20, 18, TileKind::Wall);
    fill_world_rect(world, 10, 10, 16, 14, TileKind::Ground);
    fill_world_rect(world, 28, 8, 20, 18, TileKind::Wall);
    fill_world_rect(world, 30, 10, 16, 14, TileKind::Ground);
    fill_world_rect(world, 48, 8, 20, 18, TileKind::Wall);
    fill_world_rect(world, 50, 10, 16, 14, TileKind::Ground);

    fill_world_rect(world, 8, 32, 20, 18, TileKind::Wall);
    fill_world_rect(world, 10, 34, 16, 14, TileKind::Ground);
    fill_world_rect(world, 28, 32, 24, 18, TileKind::Wall);
    fill_world_rect(world, 30, 34, 20, 14, TileKind::Ground);
    fill_world_rect(world, 58, 30, 30, 22, TileKind::Wall);
    fill_world_rect(world, 60, 32, 26, 18, TileKind::Ground);

    fill_world_rect(world, 69, 34, 8, 8, TileKind::Water);
    fill_world_rect(world, 70, 35, 6, 6, TileKind::Ground);

    fill_world_rect(world, 16, 26, 1, 6, TileKind::Ground);
    fill_world_rect(world, 38, 26, 1, 6, TileKind::Ground);
    fill_world_rect(world, 58, 26, 1, 6, TileKind::Ground);
    fill_world_rect(world, 18, 50, 1, 5, TileKind::Ground);
    fill_world_rect(world, 40, 50, 1, 5, TileKind::Ground);

    fill_world_rect(world, 5, 56, 86, 2, TileKind::Rock);
}

void build_item_zoo_world(World* world) {
    resize_world(world, 72, 40);
    fill_world(world, TileKind::Ground);
    build_box_walls(world);

    fill_world_rect(world, 6, 8, 58, 2, TileKind::Rock);
    fill_world_rect(world, 6, 18, 58, 2, TileKind::Rock);
    fill_world_rect(world, 6, 30, 58, 2, TileKind::Rock);
    fill_world_rect(world, 6, 8, 2, 24, TileKind::Rock);
    fill_world_rect(world, 62, 8, 2, 24, TileKind::Rock);

    fill_world_rect(world, 14, 20, 2, 10, TileKind::Wall);
    fill_world_rect(world, 50, 20, 2, 10, TileKind::Wall);
    fill_world_rect(world, 20, 22, 18, 1, TileKind::Tree);
    fill_world_rect(world, 20, 27, 18, 1, TileKind::Tree);

    fill_world_rect(world, 54, 22, 6, 6, TileKind::Water);
    fill_world_rect(world, 55, 23, 4, 4, TileKind::Ground);
}

void populate_sandbox_entities(GameSession* session) {
    for (const EnemySpawnSpec& spawn : kEnemyZooSpawns) {
        Enemy enemy;
        enemy.active = true;
        enemy.kind = spawn.kind;
        enemy.area_kind = spawn.area_kind;
        enemy.position = spawn.position;
        enemy.health = spawn.health;
        enemy.room_id = spawn.area_kind == AreaKind::Overworld
                            ? get_room_id_at_world_tile(static_cast<int>(spawn.position.x),
                                                        static_cast<int>(spawn.position.y))
                            : -1;
        session->enemies.push_back(enemy);
    }

    for (const PickupSpawnSpec& spawn : kItemZooPickups) {
        Pickup pickup;
        pickup.active = true;
        pickup.persistent = true;
        pickup.area_kind = spawn.area_kind;
        pickup.kind = spawn.kind;
        pickup.position = spawn.position;
        pickup.shop_item = spawn.shop_item;
        pickup.price_rupees = spawn.price_rupees;
        session->pickups.push_back(pickup);
    }

    for (const NpcSpawnSpec& spawn : kNpcSpawns) {
        Npc npc;
        npc.active = true;
        npc.area_kind = spawn.area_kind;
        npc.kind = spawn.kind;
        npc.position = spawn.position;
        npc.shop_item_index = spawn.shop_item_index;
        npc.label = spawn.label;
        session->npcs.push_back(npc);
    }
}

int gather_sandbox_portals(const GameSession* session,
                           std::array<AreaPortal, kMaxAreaPortals>* portals) {
    portals->fill(AreaPortal{});
    int count = 0;

    if (session->area_kind == AreaKind::EnemyZoo) {
        push_portal(portals, &count, AreaKind::EnemyZoo, glm::vec2(10.0F, 60.0F),
                    glm::vec2(2.0F, 1.5F), AreaKind::Overworld, get_opening_start_position(),
                    "to overworld");
        push_portal(portals, &count, AreaKind::EnemyZoo, glm::vec2(86.0F, 60.0F),
                    glm::vec2(2.0F, 1.5F), AreaKind::ItemZoo, glm::vec2(10.0F, 10.0F),
                    "to item zoo");
    }

    if (session->area_kind == AreaKind::ItemZoo) {
        push_portal(portals, &count, AreaKind::ItemZoo, glm::vec2(10.0F, 35.0F),
                    glm::vec2(2.0F, 1.5F), AreaKind::Overworld, get_opening_start_position(),
                    "to overworld");
        push_portal(portals, &count, AreaKind::ItemZoo, glm::vec2(62.0F, 35.0F),
                    glm::vec2(2.0F, 1.5F), AreaKind::EnemyZoo, glm::vec2(10.0F, 10.0F),
                    "to enemy zoo");
    }

    return count;
}

} // namespace z1m

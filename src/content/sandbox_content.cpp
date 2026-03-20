#include "content/sandbox_content.hpp"

#include "content/opening_content.hpp"

namespace z1m {

namespace {

struct EnemySpawnSpec {
    AreaKind area_kind = AreaKind::EnemyZoo;
    EnemyKind kind = EnemyKind::Octorok;
    glm::vec2 position = glm::vec2(0.0F, 0.0F);
    int health = 1;
    int subtype = 0;
    int respawn_group = -1;
    bool zoo_respawn = false;
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
    int subtype = 0;
    int shop_item_index = -1;
    const char* label = "";
};

struct EnemyZooPenBounds {
    int respawn_group = -1;
    glm::vec2 min_position = glm::vec2(0.0F);
    glm::vec2 max_position = glm::vec2(0.0F);
};

constexpr std::array<EnemyZooPenBounds, 28> kEnemyZooPenBounds = {{
    EnemyZooPenBounds{0, glm::vec2(10.5F, 10.5F), glm::vec2(25.5F, 23.5F)},
    EnemyZooPenBounds{1, glm::vec2(34.5F, 10.5F), glm::vec2(49.5F, 23.5F)},
    EnemyZooPenBounds{2, glm::vec2(58.5F, 10.5F), glm::vec2(73.5F, 23.5F)},
    EnemyZooPenBounds{3, glm::vec2(82.5F, 10.5F), glm::vec2(97.5F, 23.5F)},
    EnemyZooPenBounds{4, glm::vec2(106.5F, 10.5F), glm::vec2(121.5F, 23.5F)},
    EnemyZooPenBounds{5, glm::vec2(130.5F, 10.5F), glm::vec2(145.5F, 23.5F)},
    EnemyZooPenBounds{6, glm::vec2(10.5F, 34.5F), glm::vec2(25.5F, 47.5F)},
    EnemyZooPenBounds{7, glm::vec2(34.5F, 34.5F), glm::vec2(49.5F, 47.5F)},
    EnemyZooPenBounds{8, glm::vec2(58.5F, 34.5F), glm::vec2(73.5F, 47.5F)},
    EnemyZooPenBounds{9, glm::vec2(82.5F, 34.5F), glm::vec2(97.5F, 47.5F)},
    EnemyZooPenBounds{10, glm::vec2(106.5F, 34.5F), glm::vec2(121.5F, 47.5F)},
    EnemyZooPenBounds{11, glm::vec2(130.5F, 34.5F), glm::vec2(145.5F, 47.5F)},
    EnemyZooPenBounds{12, glm::vec2(10.5F, 58.5F), glm::vec2(25.5F, 71.5F)},
    EnemyZooPenBounds{13, glm::vec2(34.5F, 58.5F), glm::vec2(49.5F, 71.5F)},
    EnemyZooPenBounds{14, glm::vec2(58.5F, 58.5F), glm::vec2(73.5F, 71.5F)},
    EnemyZooPenBounds{15, glm::vec2(82.5F, 58.5F), glm::vec2(97.5F, 71.5F)},
    EnemyZooPenBounds{16, glm::vec2(106.5F, 58.5F), glm::vec2(121.5F, 71.5F)},
    EnemyZooPenBounds{17, glm::vec2(130.5F, 58.5F), glm::vec2(145.5F, 71.5F)},
    EnemyZooPenBounds{18, glm::vec2(106.5F, 82.5F), glm::vec2(121.5F, 95.5F)},
    EnemyZooPenBounds{19, glm::vec2(130.5F, 82.5F), glm::vec2(145.5F, 95.5F)},
    EnemyZooPenBounds{20, glm::vec2(10.5F, 106.5F), glm::vec2(25.5F, 119.5F)},
    EnemyZooPenBounds{21, glm::vec2(34.5F, 106.5F), glm::vec2(49.5F, 119.5F)},
    EnemyZooPenBounds{22, glm::vec2(58.5F, 106.5F), glm::vec2(73.5F, 119.5F)},
    EnemyZooPenBounds{23, glm::vec2(82.5F, 106.5F), glm::vec2(97.5F, 119.5F)},
    EnemyZooPenBounds{24, glm::vec2(106.5F, 106.5F), glm::vec2(121.5F, 119.5F)},
    EnemyZooPenBounds{25, glm::vec2(130.5F, 106.5F), glm::vec2(145.5F, 119.5F)},
    EnemyZooPenBounds{26, glm::vec2(106.5F, 128.5F), glm::vec2(121.5F, 139.5F)},
    EnemyZooPenBounds{27, glm::vec2(130.5F, 128.5F), glm::vec2(145.5F, 139.5F)},
}};

constexpr std::array<EnemySpawnSpec, 48> kEnemyZooSpawns = {
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Octorok,
                   .position = glm::vec2(16.0F, 12.0F),
                   .health = 1,
                   .respawn_group = 0,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Octorok,
                   .position = glm::vec2(20.0F, 18.0F),
                   .health = 1,
                   .respawn_group = 0,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Moblin,
                   .position = glm::vec2(40.0F, 12.0F),
                   .health = 2,
                   .respawn_group = 1,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Lynel,
                   .position = glm::vec2(64.0F, 12.0F),
                   .health = 4,
                   .respawn_group = 2,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Goriya,
                   .position = glm::vec2(88.0F, 12.0F),
                   .health = 3,
                   .respawn_group = 3,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Darknut,
                   .position = glm::vec2(112.0F, 12.0F),
                   .health = 4,
                   .respawn_group = 4,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Tektite,
                   .position = glm::vec2(136.0F, 12.0F),
                   .health = 1,
                   .respawn_group = 5,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Leever,
                   .position = glm::vec2(16.0F, 38.0F),
                   .health = 2,
                   .subtype = 0,
                   .respawn_group = 6,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Leever,
                   .position = glm::vec2(20.0F, 44.0F),
                   .health = 2,
                   .subtype = 1,
                   .respawn_group = 6,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Zol,
                   .position = glm::vec2(40.0F, 38.0F),
                   .health = 2,
                   .respawn_group = 7,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Gel,
                   .position = glm::vec2(44.0F, 42.0F),
                   .health = 1,
                   .respawn_group = 7,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Rope,
                   .position = glm::vec2(64.0F, 38.0F),
                   .health = 2,
                   .respawn_group = 8,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Stalfos,
                   .position = glm::vec2(88.0F, 38.0F),
                   .health = 2,
                   .respawn_group = 9,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Gibdo,
                   .position = glm::vec2(92.0F, 42.0F),
                   .health = 3,
                   .respawn_group = 9,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::LikeLike,
                   .position = glm::vec2(112.0F, 38.0F),
                   .health = 3,
                   .respawn_group = 10,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::PolsVoice,
                   .position = glm::vec2(116.0F, 42.0F),
                   .health = 2,
                   .respawn_group = 10,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Keese,
                   .position = glm::vec2(136.0F, 38.0F),
                   .health = 1,
                   .respawn_group = 11,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Ghini,
                   .position = glm::vec2(140.0F, 42.0F),
                   .health = 1,
                   .respawn_group = 11,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Bubble,
                   .position = glm::vec2(138.0F, 38.0F),
                   .health = 99,
                   .subtype = 0,
                   .respawn_group = 11,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Bubble,
                   .position = glm::vec2(144.0F, 40.0F),
                   .health = 99,
                   .subtype = 1,
                   .respawn_group = 11,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Bubble,
                   .position = glm::vec2(140.0F, 44.0F),
                   .health = 99,
                   .subtype = 2,
                   .respawn_group = 11,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Wallmaster,
                   .position = glm::vec2(16.0F, 62.0F),
                   .health = 3,
                   .respawn_group = 12,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Trap,
                   .position = glm::vec2(20.0F, 66.0F),
                   .health = 99,
                   .respawn_group = 12,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Armos,
                   .position = glm::vec2(40.0F, 62.0F),
                   .health = 3,
                   .respawn_group = 13,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Armos,
                   .position = glm::vec2(44.0F, 66.0F),
                   .health = 3,
                   .respawn_group = 13,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Zora,
                   .position = glm::vec2(64.0F, 62.0F),
                   .health = 1,
                   .respawn_group = 14,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Zora,
                   .position = glm::vec2(68.0F, 66.0F),
                   .health = 1,
                   .respawn_group = 14,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Peahat,
                   .position = glm::vec2(88.0F, 62.0F),
                   .health = 2,
                   .respawn_group = 15,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Peahat,
                   .position = glm::vec2(92.0F, 66.0F),
                   .health = 2,
                   .respawn_group = 15,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Dodongo,
                   .position = glm::vec2(112.0F, 62.0F),
                   .health = 6,
                   .respawn_group = 16,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Gohma,
                   .position = glm::vec2(134.0F, 62.0F),
                   .health = 5,
                   .subtype = 0,
                   .respawn_group = 17,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Gohma,
                   .position = glm::vec2(142.0F, 62.0F),
                   .health = 6,
                   .subtype = 1,
                   .respawn_group = 17,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Moldorm,
                   .position = glm::vec2(136.0F, 68.0F),
                   .health = 5,
                   .subtype = 0,
                   .respawn_group = 17,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Moldorm,
                   .position = glm::vec2(137.0F, 68.0F),
                   .health = 1,
                   .subtype = 1,
                   .respawn_group = 17,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Moldorm,
                   .position = glm::vec2(138.0F, 68.0F),
                   .health = 1,
                   .subtype = 2,
                   .respawn_group = 17,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Moldorm,
                   .position = glm::vec2(139.0F, 68.0F),
                   .health = 1,
                   .subtype = 3,
                   .respawn_group = 17,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Moldorm,
                   .position = glm::vec2(140.0F, 68.0F),
                   .health = 1,
                   .subtype = 4,
                   .respawn_group = 17,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Aquamentus,
                   .position = glm::vec2(112.0F, 84.0F),
                   .health = 6,
                   .respawn_group = 18,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Vire,
                   .position = glm::vec2(136.0F, 84.0F),
                   .health = 2,
                   .respawn_group = 19,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::BlueWizzrobe,
                   .position = glm::vec2(16.0F, 108.0F),
                   .health = 3,
                   .respawn_group = 20,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::RedWizzrobe,
                   .position = glm::vec2(40.0F, 108.0F),
                   .health = 3,
                   .respawn_group = 21,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::FlyingGhini,
                   .position = glm::vec2(64.0F, 108.0F),
                   .health = 2,
                   .respawn_group = 22,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Digdogger,
                   .position = glm::vec2(88.0F, 108.0F),
                   .health = 6,
                   .respawn_group = 23,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Manhandla,
                   .position = glm::vec2(112.0F, 108.0F),
                   .health = 6,
                   .respawn_group = 24,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Gleeok,
                   .position = glm::vec2(136.0F, 108.0F),
                   .health = 4,
                   .subtype = 4,
                   .respawn_group = 25,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Patra,
                   .position = glm::vec2(112.0F, 132.0F),
                   .health = 6,
                   .respawn_group = 26,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::EnemyZoo,
                   .kind = EnemyKind::Ganon,
                   .position = glm::vec2(136.0F, 132.0F),
                   .health = 9,
                   .respawn_group = 27,
                   .zoo_respawn = true},
    EnemySpawnSpec{.area_kind = AreaKind::Overworld,
                   .kind = EnemyKind::Octorok,
                   .position = glm::vec2(232.0F, 162.0F),
                   .health = 1},
};

constexpr std::array<PickupSpawnSpec, 22> kItemZooPickups = {
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
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Food,
                    .position = glm::vec2(36.0F, 28.0F)},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::Letter,
                    .position = glm::vec2(42.0F, 28.0F)},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::MagicShield,
                    .position = glm::vec2(24.0F, 28.0F)},
    PickupSpawnSpec{.area_kind = AreaKind::ItemZoo,
                    .kind = PickupKind::SilverArrows,
                    .position = glm::vec2(30.0F, 28.0F)},
};

constexpr std::array<NpcSpawnSpec, 6> kNpcSpawns = {
    NpcSpawnSpec{.area_kind = AreaKind::ItemZoo,
                 .kind = NpcKind::ShopKeeper,
                 .position = glm::vec2(28.0F, 21.0F),
                 .shop_item_index = -1,
                 .label = "item shop"},
    NpcSpawnSpec{.area_kind = AreaKind::ItemZoo,
                 .kind = NpcKind::OldWoman,
                 .position = glm::vec2(46.0F, 21.0F),
                 .shop_item_index = -1,
                 .label = "show me the letter"},
    NpcSpawnSpec{.area_kind = AreaKind::ItemZoo,
                 .kind = NpcKind::HungryGoriya,
                 .position = glm::vec2(45.0F, 25.0F),
                 .shop_item_index = -1,
                 .label = "grumble grumble"},
    NpcSpawnSpec{.area_kind = AreaKind::ItemZoo,
                 .kind = NpcKind::Fairy,
                 .position = glm::vec2(58.0F, 25.0F),
                 .shop_item_index = -1,
                 .label = "fairy pond"},
    NpcSpawnSpec{.area_kind = AreaKind::ItemZoo,
                 .kind = NpcKind::OldMan,
                 .position = glm::vec2(58.0F, 14.0F),
                 .shop_item_index = -1,
                 .label = "take any road you want"},
    NpcSpawnSpec{.area_kind = AreaKind::EnemyZoo,
                 .kind = NpcKind::OldMan,
                 .position = glm::vec2(74.0F, 136.0F),
                 .shop_item_index = -1,
                 .label = "enemy zoo respawns by pen"},
};

void build_box_walls(World* world) {
    fill_world_rect(world, 0, 0, world->width, 1, TileKind::Wall);
    fill_world_rect(world, 0, world->height - 1, world->width, 1, TileKind::Wall);
    fill_world_rect(world, 0, 0, 1, world->height, TileKind::Wall);
    fill_world_rect(world, world->width - 1, 0, 1, world->height, TileKind::Wall);
}

void build_pen(World* world, int x, int y, int width, int height, TileKind fill) {
    fill_world_rect(world, x, y, width, height, TileKind::Wall);
    fill_world_rect(world, x + 2, y + 2, width - 4, height - 4, fill);
}

void push_portal(std::array<AreaPortal, kMaxAreaPortals>* portals, int* count, AreaKind source_area,
                 const glm::vec2& center, const glm::vec2& half_size, AreaKind target_area,
                 const glm::vec2& target_position, const char* label, bool requires_raft = false) {
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
    portal.requires_raft = requires_raft;
    portal.label = label;
    ++(*count);
}

} // namespace

void build_enemy_zoo_world(World* world) {
    resize_world(world, 152, 144);
    fill_world(world, TileKind::Ground);
    build_box_walls(world);

    build_pen(world, 8, 8, 20, 18, TileKind::Ground);
    build_pen(world, 32, 8, 20, 18, TileKind::Ground);
    build_pen(world, 56, 8, 20, 18, TileKind::Ground);
    build_pen(world, 80, 8, 20, 18, TileKind::Ground);
    build_pen(world, 104, 8, 20, 18, TileKind::Ground);
    build_pen(world, 128, 8, 20, 18, TileKind::Ground);

    build_pen(world, 8, 32, 20, 18, TileKind::Ground);
    build_pen(world, 32, 32, 20, 18, TileKind::Ground);
    build_pen(world, 56, 32, 20, 18, TileKind::Ground);
    build_pen(world, 80, 32, 20, 18, TileKind::Ground);
    build_pen(world, 104, 32, 20, 18, TileKind::Ground);
    build_pen(world, 128, 32, 20, 18, TileKind::Ground);

    build_pen(world, 8, 56, 20, 18, TileKind::Ground);
    build_pen(world, 32, 56, 20, 18, TileKind::Ground);
    build_pen(world, 56, 56, 20, 18, TileKind::Water);
    build_pen(world, 80, 56, 20, 18, TileKind::Ground);
    build_pen(world, 104, 56, 20, 18, TileKind::Ground);
    build_pen(world, 128, 56, 20, 18, TileKind::Ground);

    build_pen(world, 8, 80, 20, 18, TileKind::Ground);
    build_pen(world, 32, 80, 20, 18, TileKind::Ground);
    build_pen(world, 56, 80, 20, 18, TileKind::Ground);
    build_pen(world, 80, 80, 20, 18, TileKind::Ground);
    build_pen(world, 104, 80, 20, 18, TileKind::Ground);
    build_pen(world, 128, 80, 20, 18, TileKind::Ground);

    build_pen(world, 8, 104, 20, 18, TileKind::Ground);
    build_pen(world, 32, 104, 20, 18, TileKind::Ground);
    build_pen(world, 56, 104, 20, 18, TileKind::Ground);
    build_pen(world, 80, 104, 20, 18, TileKind::Ground);
    build_pen(world, 104, 104, 20, 18, TileKind::Ground);
    build_pen(world, 128, 104, 20, 18, TileKind::Ground);
    build_pen(world, 104, 126, 20, 16, TileKind::Ground);
    build_pen(world, 128, 126, 20, 16, TileKind::Ground);

    fill_world_rect(world, 10, 130, 84, 4, TileKind::Rock);
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
    fill_world_rect(world, 40, 20, 2, 10, TileKind::Wall);
    fill_world_rect(world, 48, 20, 2, 10, TileKind::Wall);
    fill_world_rect(world, 42, 20, 6, 2, TileKind::Rock);
    fill_world_rect(world, 42, 28, 6, 2, TileKind::Rock);
    fill_world_rect(world, 20, 22, 18, 1, TileKind::Tree);
    fill_world_rect(world, 20, 27, 18, 1, TileKind::Tree);

    fill_world_rect(world, 52, 22, 12, 6, TileKind::Water);
    fill_world_rect(world, 52, 24, 2, 2, TileKind::Ground);
    fill_world_rect(world, 62, 24, 2, 2, TileKind::Ground);
}

void populate_sandbox_entities(GameState* play) {
    for (const EnemySpawnSpec& spawn : kEnemyZooSpawns) {
        Enemy enemy;
        enemy.active = true;
        enemy.kind = spawn.kind;
        enemy.area_kind = spawn.area_kind;
        enemy.position = spawn.position;
        enemy.spawn_position = spawn.position;
        enemy.origin = spawn.position;
        enemy.health = spawn.health;
        enemy.max_health = spawn.health;
        enemy.subtype = spawn.subtype;
        enemy.respawn_group = spawn.respawn_group;
        enemy.zoo_respawn = spawn.zoo_respawn;
        enemy.room_id = spawn.area_kind == AreaKind::Overworld
                            ? get_room_id_at_world_tile(static_cast<int>(spawn.position.x),
                                                        static_cast<int>(spawn.position.y))
                            : -1;
        play->enemies.push_back(enemy);
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
        play->pickups.push_back(pickup);
    }

    for (const NpcSpawnSpec& spawn : kNpcSpawns) {
        Npc npc;
        npc.active = true;
        npc.area_kind = spawn.area_kind;
        npc.kind = spawn.kind;
        npc.position = spawn.position;
        npc.origin = spawn.position;
        npc.subtype = spawn.subtype;
        npc.shop_item_index = spawn.shop_item_index;
        npc.label = spawn.label;
        play->npcs.push_back(npc);
    }
}

int gather_sandbox_portals(const GameState* play,
                           std::array<AreaPortal, kMaxAreaPortals>* portals) {
    portals->fill(AreaPortal{});
    int count = 0;

    if (play->area_kind == AreaKind::EnemyZoo) {
        push_portal(portals, &count, AreaKind::EnemyZoo, glm::vec2(10.0F, 138.0F),
                    glm::vec2(2.0F, 1.5F), AreaKind::Overworld, get_opening_start_position(),
                    "to overworld");
        push_portal(portals, &count, AreaKind::EnemyZoo, glm::vec2(98.0F, 138.0F),
                    glm::vec2(2.0F, 1.5F), AreaKind::ItemZoo, glm::vec2(10.0F, 10.0F),
                    "to item zoo");
    }

    if (play->area_kind == AreaKind::ItemZoo) {
        push_portal(portals, &count, AreaKind::ItemZoo, glm::vec2(10.0F, 35.0F),
                    glm::vec2(2.0F, 1.5F), AreaKind::Overworld, get_opening_start_position(),
                    "to overworld");
        push_portal(portals, &count, AreaKind::ItemZoo, glm::vec2(62.0F, 35.0F),
                    glm::vec2(2.0F, 1.5F), AreaKind::EnemyZoo, glm::vec2(5.0F, 138.0F),
                    "to enemy zoo");
        push_portal(portals, &count, AreaKind::ItemZoo, glm::vec2(53.0F, 25.0F),
                    glm::vec2(0.9F, 0.9F), AreaKind::ItemZoo, glm::vec2(63.0F, 25.0F), "raft dock",
                    true);
        push_portal(portals, &count, AreaKind::ItemZoo, glm::vec2(63.0F, 25.0F),
                    glm::vec2(0.9F, 0.9F), AreaKind::ItemZoo, glm::vec2(53.0F, 25.0F), "raft dock",
                    true);
    }

    return count;
}

bool get_enemy_zoo_pen_bounds(int respawn_group, glm::vec2* min_position, glm::vec2* max_position) {
    for (const EnemyZooPenBounds& pen : kEnemyZooPenBounds) {
        if (pen.respawn_group != respawn_group) {
            continue;
        }

        *min_position = pen.min_position;
        *max_position = pen.max_position;
        return true;
    }

    return false;
}

} // namespace z1m

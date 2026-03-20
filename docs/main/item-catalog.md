# Item Catalog

This is the single-player item/pickup checklist for `z1m`.

Status labels:

- `none`: not implemented yet
- `partial`: wired but still too loose or incomplete
- `zoo`: implemented/testable in sandbox
- `opening`: implemented in the main opening slice

## Core Weapons and Active Tools

- `Wooden Sword`: first sword pickup, melee, `opening`
- `White Sword`: stronger melee, `none`
- `Magic Sword`: strongest melee, `none`
- `Sword Beam`: full-health projectile, `partial`
- `Boomerang`: active projectile tool, `zoo`
- `Magic Boomerang`: stronger/faster boomerang, `none`
- `Bombs`: placed explosive, `zoo`
- `Bow`: active arrow weapon, `zoo`
- `Wood Arrow`: basic arrow pairing with bow, `partial`
- `Silver Arrow`: Ganon-capable arrow upgrade, `none`
- `Blue Candle`: fire tool with original restrictions, `partial`
- `Red Candle`: upgraded repeat-use candle, `none`
- `Recorder`: now cycles real visible dungeon entrances from warp metadata, `partial`
- `Food / Bait`: needed for hungry Goriya and some interactions, `none`
- `Magical Rod`: projectile staff, `none`
- `Book of Magic`: rod fire upgrade, `none`

## Passive Traversal and Progression Items

- `Ladder`: one-water crossing, `partial`
- `Raft`: dock travel, sandbox dock path exists, overworld dock logic still missing, `partial`
- `Power Bracelet`: move heavy rocks / reveal paths, `none`
- `Letter`: unlock potion shop, `none`
- `Magic Key`: infinite key behavior, `none`

## Defensive / Character Upgrades

- `Blue Ring`: damage reduction, `none`
- `Red Ring`: stronger damage reduction, `none`
- `Magical Shield`: upgrade shield behavior, `none`
- `Heart Container`: max HP increase, `zoo`
- `Triforce Fragment`: dungeon progression, `none`

## Dungeon Utility Items

- `Dungeon Map`: local dungeon layout reveal, `none`
- `Compass`: triforce location helper, `none`
- `Small Key`: door/key consumption, `partial`

## World Pickups and Drops

- `Heart`: healing pickup, `zoo`
- `Fairy`: healing pickup/actor, `none`
- `Rupee`: currency pickup, `zoo`
- `5-Rupee / Large Rupee`: higher-value currency pickup, `none`
- `Bomb Drop`: enemy drop and supply pickup, `zoo`
- `Clock`: time stop pickup, `none`

## Notes

- The current sandbox gives us a place to validate item feel, but it is not the
  final content graph.
- Traversal items should ultimately be validated in real overworld rooms, not
  only in item-zoo lanes.

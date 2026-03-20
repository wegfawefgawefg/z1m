# Script Catalog

This is the checklist for world logic, room scripts, and one-off progression
rules that are easy to forget if we only think in terms of tiles and enemies.

## Overworld Scripts

- visible cave entrance handling
- hidden cave reveal by candle
- hidden cave reveal by bomb
- recorder secret reveal
- Armos activation / reveal
- staircase reveal
- raft dock travel
- ladder water crossing
- pushable rock / bracelet interactions
- shop purchase flow
- potion shop / letter flow
- take-any-road flow
- old man gift / hint caves
- fairy pond / healing rooms

## Underworld / Dungeon Room Scripts

- room clear opens shutter doors
- room clear spawns key
- room clear reveals stairs
- room clear drops item
- push block reveals stairs
- push block opens passage
- bomb wall opens passage
- key door consume / open
- magic key bypass
- transport stair routing
- boss room clear rules
- triforce room completion
- old man / hint / item cellars
- hungry Goriya bait gate
- dark room visibility rules

## Boss- and Item-Specific Scripts

- Dodongo bomb eat / bloated / damage cycle
- Digdogger recorder reaction and split
- Gohma open-eye arrow damage gate
- Gleeok head sever / flying head logic
- Patra orbit pattern logic
- Ganon silver-arrow kill requirement
- recorder whirlwind / destination cycling
- food use against hungry Goriya
- book modifies magical rod shots

## Current Runtime State

- visible overworld cave entry: `partial`
- cave exit / simple cave room: `partial`
- ladder crossing: `partial`
- raft dock travel: `partial`
- recorder dungeon transport: `partial`
- potion shop / letter flow: `zoo`
- fairy pond / healing rooms: `zoo`
- hungry Goriya bait gate: `zoo`
- food use against hungry Goriya: `zoo`

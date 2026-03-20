# Enemy Catalog

This is the concrete enemy and boss checklist for `z1m`.

Primary reference pass:

- `zelda1-disassembly/src/Z_07.asm` object update/init tables
- `zelda1-disassembly/src/Z_04.asm` enemy-specific update routines

Status labels:

- `none`: not implemented yet
- `partial`: some behavior exists but is not faithful enough
- `zoo`: implemented in sandbox only
- `opening`: implemented in the main opening slice

## Overworld Enemies

- `Octorok`: cardinal wandering, cardinal rocks, should not fire diagonal aimed shots, `opening`
- `Moblin`: cardinal wandering, arrow shooter, `zoo`
- `Lynel`: stronger ranged walker, sword-beam shooter, `zoo`
- `Tektite`: hopping movement, `zoo`
- `Leever`: burrow/emerge cycle, `zoo`
- `Zora`: water-only emerge/shoot/submerge, `zoo`
- `Peahat`: flying/invulnerable phase and vulnerable grounded phase, `zoo`
- `Armos`: dormant statue until triggered, `none`

## Underworld Common Enemies

- `Goriya`: boomerang user, red/blue variants, `none`
- `Darknut`: directional shielded melee, red/blue variants, `none`
- `Keese`: flying bat, `zoo`
- `Gel`: small split slime, `none`
- `Zol`: larger slime that splits, `none`
- `Rope`: charge snake, `none`
- `Vire`: split-capable flyer, `none`
- `Pols Voice`: hopping rabbit-like enemy, `none`
- `Like Like`: shield eater, `none`
- `Gibdo`: mummy walker, `none`
- `Stalfos`: skeleton walker, `none`
- `Wallmaster`: wall grabber, `none`
- `Blue Wizzrobe`: teleporting mage, `none`
- `Red Wizzrobe`: fade/shoot mage, `none`
- `Ghini`: graveyard ghost, `none`
- `Flying Ghini`: airborne ghost, `none`
- `Bubbles`: anti-sword curse skull, `none`
- `Traps`: blade traps, `none`

## Bosses

- `Aquamentus`: move and 3-way fireball spread, `zoo`
- `Dodongo`: bomb-eating boss, `none`
- `Digdogger`: recorder-reactive boss, large and split forms, `none`
- `Manhandla`: segmented plant boss, `none`
- `Gohma`: eye-state bow boss, red/blue variants, `none`
- `Moldorm`: segmented worm boss, `none`
- `Gleeok`: multi-head dragon, `none`
- `Patra`: orbiting eye boss, two patterns, `none`
- `Ganon`: final boss, `none`

## NPC and Neutral Actors That Still Matter

- `Old Man`: cave/dungeon hint and gift logic, `partial`
- `Shop Keeper`: purchase logic, `partial`
- `Hungry Goriya`: bait gate script, `none`
- `Old Woman`: potion/letter logic, `none`
- `Fairy`: healing motion and collision, `none`

## Notes

- The current zoo is for behavior tuning, not for calling something done.
- For overworld shooters, fidelity target is screen-local/cardinal threat, not
  “always normalize directly toward Link from anywhere.”

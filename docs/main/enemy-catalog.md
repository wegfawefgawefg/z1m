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

Current count:

- `39` catalog entries below
- `39` currently modeled in code
- enemy zoo now covers the hostile set plus the neutral/script actors needed to
  validate enemy-specific interactions

## Overworld Enemies

- `Octorok`: cardinal wandering, cardinal rocks, should not fire diagonal aimed shots, `opening`
- `Moblin`: cardinal wandering, arrow shooter, `zoo`
- `Lynel`: stronger ranged walker, sword-beam shooter, `zoo`
- `Tektite`: hopping movement, `zoo`
- `Leever`: burrow/emerge cycle, `zoo`
- `Zora`: water-only emerge/shoot/submerge, `zoo`
- `Peahat`: flying/invulnerable phase and vulnerable grounded phase, `zoo`
- `Armos`: dormant statue until triggered, `zoo`

## Underworld Common Enemies

- `Goriya`: boomerang user, red/blue variants, `zoo`
- `Darknut`: directional shielded melee, red/blue variants, `zoo`
- `Keese`: flying bat, `zoo`
- `Gel`: small split slime, `zoo`
- `Zol`: larger slime that splits, `zoo`
- `Rope`: charge snake, `zoo`
- `Vire`: split-capable flyer, `zoo`
- `Pols Voice`: hopping rabbit-like enemy, `zoo`
- `Like Like`: shield eater, `zoo`
- `Gibdo`: mummy walker, `zoo`
- `Stalfos`: skeleton walker, `zoo`
- `Wallmaster`: wall grabber, `zoo`
- `Blue Wizzrobe`: teleporting mage, `zoo`
- `Red Wizzrobe`: fade/shoot mage, `zoo`
- `Ghini`: graveyard ghost, `zoo`
- `Flying Ghini`: airborne ghost, `zoo`
- `Bubbles`: anti-sword curse skull, `zoo`
- `Traps`: blade traps, `zoo`

## Bosses

- `Aquamentus`: move and 3-way fireball spread, `zoo`
- `Dodongo`: bomb-eating boss, `zoo`
- `Digdogger`: recorder-reactive boss, large and split forms, `zoo`
- `Manhandla`: segmented plant boss, `zoo`
- `Gohma`: eye-state bow boss, red/blue variants, `zoo`
- `Moldorm`: segmented worm boss, `zoo`
- `Gleeok`: multi-head dragon, `zoo`
- `Patra`: orbiting eye boss, two patterns, `zoo`
- `Ganon`: final boss, `zoo`

## NPC and Neutral Actors That Still Matter

- `Old Man`: cave/dungeon hint and gift logic, `zoo`
- `Shop Keeper`: purchase logic, `zoo`
- `Hungry Goriya`: bait gate script, `zoo`
- `Old Woman`: potion/letter logic, `zoo`
- `Fairy`: healing motion and collision, `zoo`

## Notes

- The current zoo is the behavior-validation area; world assembly is still a
  separate pass.
- For overworld shooters, fidelity target is screen-local/cardinal threat, not
  “always normalize directly toward Link from anywhere.”

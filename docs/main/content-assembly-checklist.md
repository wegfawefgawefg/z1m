# Content Assembly Checklist

This is the working order for getting from isolated behaviors to a completable
single-player base game.

## Rule

Every content feature should pass through three stages:

1. isolated in a zoo or focused test room
2. placed into the real overworld / dungeon graph
3. validated against progression and script consequences

## Recommended Order

### 1. Lock Core Overworld Enemies

- octorok
- moblin
- lynel
- tektite
- leever
- zora
- peahat
- armos

Validation:

- movement feel
- projectile direction rules
- spawn/despawn / persistence expectations
- correct damage, knockback, drops

### 2. Lock Core Active Items

- swords and sword beam
- boomerangs
- bombs
- bow / arrows
- candle
- recorder
- ladder
- raft

Validation:

- sandbox item-zoo
- one real overworld use case for each traversal item
- one real combat use case for each weapon item

### 3. Lock Overworld Script Set

- visible caves
- hidden caves
- Armos reveals
- recorder secrets
- raft docks
- take-any-road
- shops and potion caves

### 4. Lock Dungeon Room Feature Set

- doors, shutters, keys
- push-block stairs
- bomb walls
- item rooms
- triforce rooms
- transport stairs

### 5. Build Dungeons In Order

- levels 1 through 9
- each dungeon gets entrance, room scripts, item, boss, reward, exit

## Zoo Validation Rules

- player is effectively invulnerable
- player can move through zoo walls
- enemies stay constrained by zoo pens
- every enemy/item/script station should have a visible label or obvious purpose

## Campaign Validation Rules

- if it exists in a zoo, it must later be placed in a real world/dungeon context
- if it exists in the campaign, it must map back to a checklist item here
- do not mark anything “done” until both zoo and campaign validation pass

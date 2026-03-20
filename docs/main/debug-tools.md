#Debug Tools

`z1m` now has a built-in runtime debug layer for world/logic inspection.

## Goals

- Make warp triggers, collision, and item interactions visible while gameplay is
  still incomplete.
- Keep debug controls in-engine instead of scattering temporary hotkeys and
  hardcoded printfs everywhere.
- Show ROM/decomp-driven data directly in the running game so loader mistakes
  are obvious.

## Dear ImGui Debug Panel

The debug panel is built with vendored Dear ImGui and sits on SDL3 +
`SDL_Renderer`.

Current panel contents:

- master toggle for the panel itself
- master toggle for the world debug overlay
- hitbox toggle
- collision-tile toggle
- interactables toggle
- label toggle
- current session stats
- current transient gameplay message
- current room warp metadata
- quick travel buttons for overworld, enemy zoo, and item zoo
- enemy-zoo pen respawn buttons

Current hotkeys:

- `F1`: show/hide the Dear ImGui debug panel

Gameplay test controls:

- `Space` or `F`: sword attack
- sword throws a beam at full health
- `E` or `Left Shift`: use selected item
- `Q`: previous item
- `R` or `Tab`: next item

## Overlay Types

### Hitboxes

Shows:

- Link body box
- sword hit area while attacking
- enemy hurt/contact boxes
- projectile boxes
- pickup boxes

### Collision Tiles

Shows:

- non-walkable tiles in the current room
- water separated visually from other solid tiles
- cave wall tiles inside simple cave rooms

### Interactables

Shows:

- overworld cave/dungeon/shortcut warp trigger boxes
- cave exit box
- sandbox portal pads, including raft-only docks
- pickup labels
- NPC labels
- enemy labels and zoo respawn-group ids
- hidden cave room attributes when a room has cave metadata but no visible
  entrance tiles yet

## Zoo Areas

The repo now has two explicit test areas:

- `enemy-zoo`: grouped pens for walking enemies, flyers, water enemies, and a
  boss pen
- `item-zoo`: free item pickups, shop-style purchases, and NPCs

Use them to tune behavior in isolation before pushing more content into the
main campaign world.

The session now keeps sandbox entities alive across area changes instead of
despawning everything outside the currently viewed room.
The player is also treated as a zoo inspector there right now: effectively
infinite health, and no collision against zoo walls, while enemies still obey
their pen collision.
Enemy pens auto-respawn when cleared, and the debug panel also exposes a manual
respawn button per pen group.
The enemy zoo now includes the late hostile families and boss pens as well,
including recorder-reactive and orbiter-style cases.

Recorder now uses real visible dungeon warp metadata from the overworld loader
to cycle between dungeon entrances instead of using a fake hardcoded list.

## Warp Rules

Overworld cave entry is no longer a giant guessed rectangle.

Current runtime rule:

- read room attribute B from extracted world data
- derive the cave/dungeon index from the ROM-style high bits
- scan the room for visible overworld warp tiles (`0x24`, `0x88`, `0x70-0x73`)
- group contiguous warp tiles into a doorway/stair trigger
- place the runtime trigger at the decomp-style Y alignment (`tile_y + 13/16`)

That is still an approximation of the full NES movement/object model, but it is
derived from the actual room tiles and `CheckWarps` logic instead of a fake
screen-sized box.

## Current Limits

- Hidden caves and secret stairs are only annotated right now;
secret discovery logic is not implemented yet.- Cave interiors are still placeholder logic rooms,
    not full ROM - authored cave content.- The zoos are test content,
    not canonical campaign placement.-
        The bottom message strip is still a lightweight gameplay / status layer,
    not a final dialogue UI.

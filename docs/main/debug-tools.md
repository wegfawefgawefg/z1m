# Debug Tools

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
- current room warp metadata

Current hotkeys:

- `F1`: show/hide the Dear ImGui debug panel
- `F2`: show/hide the world debug overlay
- `F3`: show/hide hitboxes
- `F4`: show/hide collision tiles
- `F5`: show/hide interactables
- `F6`: show/hide labels

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
- pickup labels
- hidden cave room attributes when a room has cave metadata but no visible
  entrance tiles yet

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

- Hidden caves and secret stairs are only annotated right now; secret discovery
  logic is not implemented yet.
- Cave interiors are still placeholder logic rooms, not full ROM-authored cave
  content.
- Overlay annotations currently focus on the active room only.

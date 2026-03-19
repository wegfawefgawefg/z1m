# Game Scope

## Content Target

The target is a feature-complete Zelda 1 style campaign, even if individual
details are reimplemented in a practical way.

That means shipping all of the following:

- Full overworld map and all named or distinct locations
- Full dungeon set, room layouts, room links, bosses, transport behavior, and
  dungeon-specific progression logic
- Core combat items and utility items
- Shops, caves, secrets, transport points, doors, push blocks, and stairways
- Enemy rosters, spawn placements, and broad encounter behavior
- Save/load and session rejoin support
- Enough HUD and menu support to make multiplayer playable

## Accuracy Target

The game should feel broadly like Zelda 1, but accuracy is subordinate to
clarity and completeness.

Allowed deviations:

- Different rendering style or asset pipeline
- Stand-in art, replacement tilesheets, or individual animation sprites
- Modern camera and zoom behavior
- Internal data structures that differ completely from NES-era layouts
- Practical cleanup of awkward original behavior if it improves maintainability

Avoid unless there is a good reason:

- Removing original locations or progression beats
- Flattening dungeon logic into something recognizably different from the source
- Replacing original mechanics with unrelated modern systems

## Working Decisions

- World geography, dungeon topology, encounter placement, and progression rules
  are part of the must-keep game identity.
- Art fidelity is optional. Content fidelity is not.
- The project should support multiple presentation modes over time:
  - single-room focus
  - nearby-room context
  - wide-area or full-map zoomed-out rendering
- Players are allowed to occupy different rooms, screens, or regions at the
  same time. The game should not force the party to stay together.

## Progression Defaults

The initial working model is:

- Shared world progression lives with the host session.
- Per-player state includes position, health, combat state, and inventory.
- Durable world changes are shared when they affect exploration for everyone.

Examples of shared world changes:

- a boss defeated
- a dungeon room cleared permanently
- a secret stair revealed
- a door or puzzle state that should stay solved for the session

Examples of per-player state:

- current health
- active weapon use
- rupees, bombs, arrows, and keys
- item ownership

This is the right default for now because it supports join-in-progress and
separate exploration without forcing every pickup to be globally unique. The
hard edge cases are tracked in `open-questions.md`.

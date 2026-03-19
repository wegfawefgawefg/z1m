# Open Questions

These are intentionally unresolved. Everything else in the docs should be
treated as the current working decision.

## Late-Join Progression Catch-Up

We currently assume:

- shared world progression
- per-player inventory

Open problem:

- when a new player joins an advanced host world, which key items should be
  granted automatically, copied conditionally, or left for the player to earn
  manually?

## Save Model

We need a concrete save layout for:

- host world progression
- per-player inventory and stats
- reconnecting a player to the same campaign later

## Friendly Interaction Rules

We want player-to-player interaction, but we still need exact defaults for:

- whether push can interrupt movement
- whether bombs can launch teammates
- whether specific items should damage teammates in optional modes

## Default Camera and Presentation Mode

We want multiple view modes eventually, but we have not picked the default
presentation for the first real playable version:

- room-only
- room plus neighbors
- continuously scrolling world view

## Remote Simulation Budget

The docs assume permissive divergence, but we still need real thresholds for:

- update rate by distance
- room activity rules when no local player is present
- what remote events are worth reliable delivery

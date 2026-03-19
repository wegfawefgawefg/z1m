# Architecture

## Guiding Rules

- Keep files small and responsibility-focused.
- Keep the module tree mostly flat.
- Prefer explicit game-specific modules over vague shared utility buckets.
- Avoid mandatory ECS or framework-heavy architecture unless a concrete problem
  appears that simpler structures cannot solve.
- Prefer plain structs plus free functions over heavy OO layering.
- Avoid wrapper classes whose only job is to hide direct state updates.

## Initial Tech Stack

- SDL3 for window creation, input, and the first renderer path
- GLM for vector and camera math
- Fixed `60 Hz` simulation with rendering decoupled from simulation rate

## Recommended Top-Level Slices

- `app/`: startup, main loop, high-level mode switching
- `platform/`: windowing, input, timing, file paths, sockets
- `render/`: camera, tile drawing, sprites, debug overlays, presentation modes
- `game/`: player control, combat rules, room logic, entity behaviors
- `world/`: map graph, dungeon graph, room data, spawn tables, progression flags
- `net/`: session state, event transport, remote player state, serialization
- `content/`: normalized data loaded from our own runtime formats
- `tools/`: extraction or conversion utilities used during development

Not every slice needs to exist immediately, but the ownership boundaries should
stay clear from the beginning.

## Data Direction

Preferred flow:

1. References and decomps are consulted offline
2. We extract or hand-author normalized runtime data for `z1m`
3. The game runtime consumes only `z1m` data formats

That keeps reference code separate from the shipped runtime and makes behavior
easier to inspect.

## Simulation Model

- Fixed-step simulation at `60 Hz`
- Rendering decoupled from simulation rate
- World simulation divided by room, screen, or region so distant activity can be
  updated with different cost and fidelity
- Durable world state stored separately from transient actor state

## Multiplayer Shape

The networking code should be event-first, not frame-sync-first.

Recommended split:

- durable world events
- per-player session state
- approximate remote actor state
- local prediction and presentation state

Do not mix all of that into one giant replication blob.

## UI and Menus

The original game did not need multiplayer session flow, so `z1m` should treat
that as first-class game code rather than an afterthought.

Expected additions:

- host or join flow
- player/session settings
- reconnect or rejoin handling
- simple diagnostics for latency and connection state

## Rendering Direction

Rendering should be able to present the same world data in multiple ways.

Initial target:

- straightforward scrollable tilemap view
- debug overlays for collision, rooms, and replicated state

Later target:

- room-focused presentation
- nearby-room stacked views
- zoomed-out whole-area rendering

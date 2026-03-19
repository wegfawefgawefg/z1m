# Roadmap

## Phase 0: Project Bootstrap

- establish docs and working decisions
- establish build, run, and formatting setup
- create a tiny runnable prototype with a clean module layout

## Phase 1: Local Gameplay Slice

- scrollable tilemap rendering
- player movement
- sword attack and hit detection
- at least one enemy type
- basic collision and room transitions
- debug overlays for coordinates, rooms, and state

Success condition: one player can move through a small playable area and fight
enemies with behavior that is recognizably Zelda-like.

## Phase 2: World and Content Backbone

- runtime formats for overworld and dungeon data
- room graph and screen graph loading
- item and pickup framework
- interactable world features
- save model prototype

Success condition: the full game world can be represented even if all content is
not polished yet.

## Phase 3: Multiplayer Vertical Slice

- host and join flow
- join-in-progress
- remote player visibility
- event replication for kills, doors, and room state
- lag-tolerant local movement and combat

Success condition: multiple players can roam, fight, and progress together
without obvious local lag.

## Phase 4: Full Content Pass

- all overworld screens
- all dungeons
- bosses and item progression
- shops, caves, secrets, transport, and save flow

Success condition: the entire campaign is completable in coop.

## Phase 5: Quality and Scale

- better session management
- improved catch-up behavior for late joiners
- more rendering modes
- better tooling and content validation
- performance work for larger player counts

Success condition: the project is stable, debuggable, and comfortable to host
for long coop sessions.

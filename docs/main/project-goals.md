# Project Goals

## Core Goal

`z1m` is a modern C++ implementation of Zelda 1 with cooperative multiplayer.

It is not an emulator and it is not trying to be cycle-accurate. The target is
complete game coverage with gameplay that feels recognizably like Zelda 1 while
being practical to build, easy to debug, and friendly to high-latency online
play.

## What Matters Most

- Complete world coverage: all overworld locations, dungeons, encounters,
  progression gates, and major item behaviors should exist.
- Cooperative play first: the project should feel like a good shared campaign,
  not a single-player remake with multiplayer stapled on.
- Responsiveness over strict sync: local play should stay immediate even when
  peers are very far apart.
- Clear code over clever code: small files, explicit data flow, low-magic
  structure, and easy-to-trace logic.
- Flexible presentation: original art is not required, and rendering does not
  need to mimic NES limits.

## Explicit Non-Goals

- Exact hardware accuracy
- Mandatory reuse of original assets
- Perfectly non-divergent simulation across clients
- Competitive multiplayer correctness
- A hardcoded small player cap

## Working Product Principles

- Default implementation language is modern C++.
- Default coding style target is C+ or C+-:
  - direct
  - explicit
  - small, isolated modules
  - modest abstraction
  - debugger-friendly control flow
- Runtime data types should use modern practical types. We do not need to mimic
  original 8-bit storage unless that choice materially helps behavior.
- The game should be able to support at least `16` players as an initial target.
  This should be represented as a configurable constant, not a deep design
  assumption.

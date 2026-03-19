# Reference Policy

## What We Can Use References For

The surrounding workspace includes useful references:

- original ROM
- disassembly and decomp projects
- clean architecture examples
- platform and rendering examples

These are valid sources for:

- room layouts
- tile placements
- spawn positions
- dungeon links
- secret triggers
- item behavior
- enemy behavior
- menu flow and progression rules

## What `z1m` Should Own Directly

`z1m` should keep its own:

- runtime code
- runtime data formats
- replacement art or stand-in assets
- tooling for extraction or conversion
- multiplayer-specific behavior

Do not make the runtime depend directly on external repos in this workspace.

## Practical Rule

Use references to understand and extract. Do not entangle the new codebase with
their internal structure.

That means:

- no copying large chunks of decomp architecture into the runtime by default
- no assuming external directory layouts inside the game
- no tying the main build to neighboring repos

## Asset Policy

Original artwork is not required.

Acceptable approaches:

- temporary placeholder art
- hand-made replacement tilesheets
- per-animation sprite PNG folders
- mixed temporary and final art during development

The key constraint is that the content coverage remains complete even when the
art pipeline is still rough.

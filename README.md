# z1m

`z1m` is a C++ implementation of Zelda 1 with cooperative multiplayer.

The project target is full Zelda 1 content coverage with pragmatic modern
implementation choices, flexible rendering, and latency-tolerant coop play.
Behavior does not need to be cycle-accurate, but the locations, dungeons,
progression, and overall feel should stay recognizably Zelda 1.

Current bootstrap stack:

- SDL3 for the window and renderer
- GLM for math
- fixed `60 Hz` simulation with uncapped rendering
- default window size `960x540` which is half of `1920x1080`

## Docs

Start with [docs/README.md](docs/README.md).

Important docs:

- [docs/project-goals.md](docs/project-goals.md)
- [docs/game-scope.md](docs/game-scope.md)
- [docs/multiplayer-model.md](docs/multiplayer-model.md)
- [docs/architecture.md](docs/architecture.md)
- [docs/roadmap.md](docs/roadmap.md)

## Local Build

Configure and build a debug binary:

```bash
cmake -S . -B build_debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build_debug -j
```

Run it:

```bash
./build_debug/z1m
```

If SDL3 is not already installed on the machine, CMake fetches the official
`SDL3 3.2.22` release tarball and builds it as part of configure.

Controls:

- `w/a/s/d` or arrow keys: move
- `space` or `f`: swing sword
- mouse wheel / `+` / `-`: zoom
- `0`: reset zoom
- `esc`: quit

## VS Code

The repo includes:

- `.vscode/tasks.json` for configure/build
- `.vscode/launch.json` so `F5` builds and runs `z1m`
- `.clang-format` and `.vscode/settings.json` for formatting integration

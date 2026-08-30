# helios-examples

Runnable examples for the [helios](https://helios.garagecraft.games) C++23 game-engine ecosystem.

---

**Project website:** [helios.garagecraft.games](https://helios.garagecraft.games) / [thorsten.suckow-homberg.de](https://thorsten.suckow-homberg.de)  

---

## Overview

This repository collects self-contained example programs that demonstrate how to wire up the helios module stack — ECS, rendering, physics, windowing, and UI — for real use-cases.

Each example lives in its own subdirectory under `src/` and is built into a separate executable by CMake.

---

## Examples

### `ecs_rendering`

**Source:** [`src/ecs_rendering/`](src/ecs_rendering/)

Demonstrates the full ECS-driven rendering pipeline:

- World setup with typed entity handles
- Viewport, camera, and scene binding
- `SceneMemberVisibilitySystem` + `SceneRenderSystem` pipeline (instanced submission)
- OpenGL draw dispatch via command buffers
- ImGui debug overlay
- Physics integration

Modules used: `helios.ecs`, `helios.engine`, `helios.math`, `helios.opengl`, `helios.glfw`, `helios.imgui`, `helios.physics`

---

### `game_of_life`

**Source:** [`src/game_of_life/`](src/game_of_life/)

Conway's Game of Life implemented on top of the helios ECS and rendered via OpenGL:

- Per-cell ECS entities with `CellAliveComponent` / `CellDeadComponent`
- Instanced rendering of a large cell grid
- Rule-based system update each frame
- ImGui controls for stepping and resetting the simulation

Modules used: `helios.ecs`, `helios.engine`, `helios.math`, `helios.opengl`, `helios.glfw`, `helios.imgui`, `helios.physics`

---

## Building

All examples are built together via a single CMake project. Dependencies are resolved automatically — either from local sibling directories or fetched from GitHub.

```bash
cmake -S . -B build
cmake --build build
```

Executables are placed under `build/examples/<example-name>/`.

### Local dependency overrides

By default the build looks for sibling repositories at `../helios-*`. You can override each path explicitly:

```bash
cmake -S . -B build \
  -DHELIOS_ENGINE_LOCAL_PATH=/path/to/helios-engine \
  -DHELIOS_OPENGL_LOCAL_PATH=/path/to/helios-opengl \
  -DHELIOS_GLFW_LOCAL_PATH=/path/to/helios-glfw \
  -DHELIOS_IMGUI_LOCAL_PATH=/path/to/helios-imgui \
  -DHELIOS_PHYSICS_LOCAL_PATH=/path/to/helios-physics
```

### Requirements

- CMake ≥ 4.0
- C++23-capable compiler (Clang ≥ 17 or GCC ≥ 14 recommended)
- OpenGL driver with at least OpenGL 4.1 support

---

## Development checks

Quick devtools entrypoint from this repository:

```bash
sh ./run-devtools.sh format
sh ./run-devtools.sh format-fix
sh ./run-devtools.sh tidy
sh ./run-devtools.sh tidy-fix
```

`run-devtools.sh` is a thin wrapper around CMake targets and expects an already configured build directory (default: `cmake-build-debug`).

Run formatting checks:

```bash
cmake --build cmake-build-debug --target format
```

Run formatting with in-place fixes:

```bash
cmake --build cmake-build-debug --target format-fix
```

Run clang-tidy checks:

```bash
cmake --build cmake-build-debug --target tidy
```

Run clang-tidy with autofix:

```bash
cmake --build cmake-build-debug --target tidy-fix
```

Target-specific variants are also available:

```bash
cmake --build cmake-build-debug --target format-helios_examples_bootstrap
cmake --build cmake-build-debug --target tidy-helios_examples_bootstrap
```

Formatting and clang-tidy checks are sourced from shared `helios-devtools` config.

---

## Related repositories

| Repository | Description |
|---|---|
| [`helios-engine`](https://github.com/thorstensuckow/helios-engine) | Runtime, world orchestration, rendering abstractions, scene systems |
| [`helios-ecs`](https://github.com/thorstensuckow/helios-ecs) | Generic ECS primitives (handles, sparse-set storage, views) |
| [`helios-math`](https://github.com/thorstensuckow/helios-math) | Math primitives (vectors, matrices, frustum planes) |
| [`helios-opengl`](https://github.com/thorstensuckow/helios-opengl) | OpenGL backend (buffers, shaders, textures, draw dispatch) |
| [`helios-glfw`](https://github.com/thorstensuckow/helios-glfw) | GLFW window and input integration |
| [`helios-imgui`](https://github.com/thorstensuckow/helios-imgui) | Dear ImGui integration layer |
| [`helios-physics`](https://github.com/thorstensuckow/helios-physics) | Physics simulation integration |


# Embervale / GameEngine

A C++20 and SDL3 tile-based RPG project, with an existing playable development world and a planned content creation toolkit called **Embervale Studio**.

## Start here

| I want to… | Read |
|---|---|
| Understand the project in five minutes | [Project overview](docs/embervale/00-overview/PROJECT_OVERVIEW.md) |
| See what is implemented | [Implemented systems](docs/embervale/01-current-state/IMPLEMENTED_SYSTEMS.md) |
| See what happens next | [Master roadmap](docs/embervale/02-roadmap/MASTER_ROADMAP.md) |
| Build and run the existing game | [Getting started](docs/embervale/06-development/GETTING_STARTED.md) |
| Work on a specific task | [Implementation backlog](docs/embervale/02-roadmap/BACKLOG.md) |
| Understand the editor design | [Embervale Studio](docs/embervale/04-studio/STUDIO_DESIGN.md) |
| Design the starter region | [River Crossing](docs/embervale/05-game/RIVER_CROSSING.md) |
| Find every document | [Documentation index](docs/embervale/README.md) |

## Project status

The source includes authoritative world updates, movement and running, gathering, crafting, melee combat, NPCs, quests, dialogue, shops, banking, progression, development-player persistence and an SDL3 client. The current development map is flat and uses placeholder visuals. Studio, external authored region loading and elevation are planned work; their design documents are specifications rather than instructions for tools that already exist.

The documentation baseline is commit [e9d32932e8f9b27e96283313cca320058792a81f](https://github.com/Adzffs/GameEngine/commit/e9d32932e8f9b27e96283313cca320058792a81f), checked on **18 November 2026**. The latest inspected push workflow at that commit is [marked failed](https://github.com/Adzffs/GameEngine/actions/runs/44018694678). Its available job metadata does not establish the cause. PR #2's earlier report of 81 passing tests is historical evidence; this documentation update did not execute a fresh game build. See the [evidence ledger](docs/embervale/01-current-state/EVIDENCE_LEDGER.md).

## Quick headless build

From the repository root, with a C++20 compiler, CMake 3.20 or newer and Ninja available:

```bash
cmake -S . -B build-headless -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DGAMEENGINE_BUILD_CLIENT=OFF -DGAMEENGINE_BUILD_GRAPHICS_TESTS=OFF
cmake --build build-headless --parallel
ctest --test-dir build-headless --output-on-failure -L headless
```

These commands match the checked-in build configuration. They are reproduction instructions, not a claim that the current CI failure has been resolved. SDL3 is needed for the client configuration; see [Getting started](docs/embervale/06-development/GETTING_STARTED.md).

## Development direction

First establish a reproducible baseline. Then extract the current world into validated, versioned content; prove equivalent behaviour; and deliver one complete editor workflow: **move a real tree, validate it, preview it, undo it, save it and play the edited map**. Expand into a flat River Crossing starter area, coherent artwork and additional content editors after that foundation works.

The longer-term direction includes richer regions, elevation, encounters, companions and multiplayer investigation. These are separately gated milestones, not claims of implemented MMO infrastructure.

## Contributing

Read the [contribution workflow](docs/embervale/06-development/CONTRIBUTING.md), [testing strategy](docs/embervale/06-development/TESTING.md), [save compatibility contract](docs/embervale/03-engine/SAVE_COMPATIBILITY.md) and the relevant task before changing runtime behaviour. Changes to a documented feature should update its specification, implementation status and evidence in the same pull request.

No project licence is asserted by this README. Confirm ownership and choose an appropriate repository licence before distributing third-party contributions or assets; record asset provenance as described in [Art and audio](docs/embervale/05-game/ART_AND_AUDIO.md).

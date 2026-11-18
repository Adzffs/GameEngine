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

The source includes authoritative world updates, movement and running, gathering, crafting, melee combat, NPCs, quests, dialogue, shops, banking, progression, development-player persistence and an SDL3 client. The current development map is flat and uses placeholder visuals. The first Studio foundation is a read-only [starter-world exporter](docs/embervale/04-studio/STARTER_WORLD_EXPORT.md). Map editing, external region loading and elevation remain planned; their design documents describe future capabilities.

On **18 November 2026**, the unchanged baseline at `c697386382845f17ce02027e6bade24591a9eb36` built locally and all **81 existing tests passed** on Linux with SDL’s dummy display driver. The exporter adds two CTest checks. See the [verification record](docs/embervale/06-development/BASELINE_VERIFICATION_2026-11-18.md) for changed-source results, exact environments and remaining Windows/visual-playtest limits.

## Quick headless build

From the repository root, with a C++20 compiler, CMake 3.20 or newer and Ninja available:

```bash
cmake -S . -B build-headless -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DGAMEENGINE_BUILD_CLIENT=OFF -DGAMEENGINE_BUILD_GRAPHICS_TESTS=OFF
cmake --build build-headless --parallel
ctest --test-dir build-headless --output-on-failure -L headless
```

Export the current compiled starter area with `./build-headless/EmbervaleWorldExport > development-starter.snapshot.json`. The output is a complete baseline snapshot; editing it does not yet alter the game. SDL3 is needed for the client configuration; see [Getting started](docs/embervale/06-development/GETTING_STARTED.md).

## Development direction

The local baseline and read-only export are the first implemented foundation. Next add bounded input validation and prove equivalent runtime loading, then deliver one complete editor workflow: **move a real tree, validate it, preview it, undo it, save it and play the edited map**. Expand into a flat River Crossing starter area, coherent artwork and additional content editors after that foundation works.

The longer-term direction includes richer regions, elevation, encounters, companions and multiplayer investigation. These are separately gated milestones, not claims of implemented MMO infrastructure.

## Contributing

Read the [contribution workflow](docs/embervale/06-development/CONTRIBUTING.md), [testing strategy](docs/embervale/06-development/TESTING.md), [save compatibility contract](docs/embervale/03-engine/SAVE_COMPATIBILITY.md) and the relevant task before changing runtime behaviour. Changes to a documented feature should update its specification, implementation status and evidence in the same pull request.

No project licence is asserted by this README. Confirm ownership and choose an appropriate repository licence before distributing third-party contributions or assets; record asset provenance as described in [Art and audio](docs/embervale/05-game/ART_AND_AUDIO.md).

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
| See the first in-game art | [Forest rendering and captures](docs/embervale/06-development/FOREST_RENDERING_2026-11-19.md) |
| See player, Mara and furnace art | [Cast art and walking](docs/embervale/06-development/CAST_RENDERING_2026-11-19.md) |
| See river, stone and mining art | [Environment art and captures](docs/embervale/06-development/ENVIRONMENT_RENDERING_2026-11-19.md) |
| See bank and crafting table art | [Service art and captures](docs/embervale/06-development/SERVICE_RENDERING_2026-11-19.md) |
| Find every document | [Documentation index](docs/embervale/README.md) |

## Project status

The source includes authoritative world updates, movement and running, gathering, crafting, melee combat, NPCs, quests, dialogue, shops, banking, progression, development-player persistence and an SDL3 client. The current development map is flat and uses a first grass/path/tree art kit alongside placeholder visuals. The Studio foundation now includes [starter-world export](docs/embervale/04-studio/STARTER_WORLD_EXPORT.md), bounded validation and [explicit runtime loading](docs/embervale/04-studio/STARTER_WORLD_IMPORT.md) of the existing starter region. The game can consume supported terrain and coordinate changes while preserving placement identities and creation stages. Shared editing, guarded saving, undo, a visual editor and elevation remain planned.

On **18 November 2026**, the runtime-import slice passed **77/77 headless tests** and **89/89 tests** in the Linux SDL client configuration. A real game process also loaded an edited tree placement, ran with temporary progress and stopped cleanly without creating files. This used SDL’s dummy video/software rendering, not a visible manual playthrough. See the [runtime verification record](docs/embervale/06-development/RUNTIME_IMPORT_VERIFICATION_2026-11-18.md) for exact source/environment evidence and platform limits; [earlier baseline/export results](docs/embervale/06-development/BASELINE_VERIFICATION_2026-11-18.md) remain separately recorded.

## Quick headless build

From the repository root, with a C++20 compiler, CMake 3.20 or newer and Ninja available:

```bash
cmake -S . -B build-headless -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DGAMEENGINE_BUILD_CLIENT=OFF -DGAMEENGINE_BUILD_GRAPHICS_TESTS=OFF
cmake --build build-headless --parallel
ctest --test-dir build-headless --output-on-failure -L headless
```

Export the compiled starter area and validate the selected document:

```bash
./build-headless/EmbervaleWorldExport > development-starter.playtest.json
./build-headless/EmbervaleWorldValidate development-starter.playtest.json
```

With an SDL3 client build, launch it using `./build-client/GameEngine --world development-starter.playtest.json` (`GameEngine.exe` on Windows). Imported-world playtests use temporary player progress unless an explicit `--save` path is supplied. See [runtime import](docs/embervale/04-studio/STARTER_WORLD_IMPORT.md) for supported edits, limits and all launch options.

## Development direction

The export, bounded input and runtime adapter establish the content-loading foundation. Next implement a shared move operation and guarded saving, then complete the editor workflow: **move a real tree, validate it, preview it, undo it, save it and play the edited map**. Expand into a flat River Crossing starter area, coherent artwork and additional content editors after that foundation works.

The longer-term direction includes richer regions, elevation, encounters, companions and multiplayer investigation. These are separately gated milestones, not claims of implemented MMO infrastructure.

## Contributing

Read the [contribution workflow](docs/embervale/06-development/CONTRIBUTING.md), [testing strategy](docs/embervale/06-development/TESTING.md), [save compatibility contract](docs/embervale/03-engine/SAVE_COMPATIBILITY.md) and the relevant task before changing runtime behaviour. Changes to a documented feature should update its specification, implementation status and evidence in the same pull request.

No project licence is asserted by this README. Confirm ownership and choose an appropriate repository licence before distributing third-party contributions or assets; record asset provenance as described in [Art and audio](docs/embervale/05-game/ART_AND_AUDIO.md).

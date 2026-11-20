# Embervale / GameEngine

A C++20 and SDL3 tile-based RPG project, with an existing playable development world and a planned content creation toolkit called **Embervale Studio**.

## Start here

| I want to… | Read |
|---|---|
| Understand the project in five minutes | [Project overview](docs/embervale/00-overview/PROJECT_OVERVIEW.md) |
| See the current reviewed state | [Current status](docs/embervale/01-current-state/CURRENT_STATUS.md) |
| Read the full project review | [Findings, fixes and remaining work](docs/embervale/06-development/PROJECT_REVIEW_2026-11-20.md) |
| Read the original engine inventory | [Dated implemented systems](docs/embervale/01-current-state/IMPLEMENTED_SYSTEMS.md) |
| See what happens next | [Master roadmap](docs/embervale/02-roadmap/MASTER_ROADMAP.md) |
| Build and run the existing game | [Getting started](docs/embervale/06-development/GETTING_STARTED.md) |
| Work on a specific task | [Implementation backlog](docs/embervale/02-roadmap/BACKLOG.md) |
| Understand the editor design | [Embervale Studio](docs/embervale/04-studio/STUDIO_DESIGN.md) |
| Design the starter region | [River Crossing](docs/embervale/05-game/RIVER_CROSSING.md) |
| See the first in-game art | [Forest rendering and captures](docs/embervale/06-development/FOREST_RENDERING_2026-11-19.md) |
| See player, Mara and furnace art | [Cast art and walking](docs/embervale/06-development/CAST_RENDERING_2026-11-19.md) |
| See river, stone and mining art | [Environment art and captures](docs/embervale/06-development/ENVIRONMENT_RENDERING_2026-11-19.md) |
| See bank and crafting table art | [Service art and captures](docs/embervale/06-development/SERVICE_RENDERING_2026-11-19.md) |
| See the workshop and building kit | [Building art, collision and roof visibility](docs/embervale/06-development/BUILDING_RENDERING_2026-11-19.md) |
| Animate characters, weapons or actions | [Animation guide](docs/embervale/05-game/ANIMATION_GUIDE.md) |
| Reuse animation research and measured findings | [Animation research, 20 November](docs/embervale/06-development/ANIMATION_RESEARCH_2026-11-20.md) |
| Check combat, food and recovery verification | [Starter replay and corrected tutorial checkpoints](docs/embervale/06-development/STARTER_COMBAT_RECOVERY_2026-11-20.md) |
| Check when progress is saved | [Autosave policy, retries and interruption/reload evidence](docs/embervale/06-development/PLAYER_AUTOSAVE_2026-11-20.md) |
| Recover unfinished map edits | [Private recovery, source conflicts and interrupted-write results](docs/embervale/06-development/EDITOR_RECOVERY_2026-11-20.md) |
| Publish or roll back a map version | [Studio validation, fixed versions and selected launch](docs/embervale/04-studio/WORLD_PUBLICATION.md) |
| Check sprite resources and memory budgets | [Read-only art validation, costs and runtime limits](docs/embervale/05-game/ASSET_RESOURCE_LIMITS.md) |
| Use sound effects and volume controls | [Action audio and packaging results](docs/embervale/06-development/ACTION_AUDIO_2026-11-20.md) |
| Find every document | [Documentation index](docs/embervale/README.md) |

## Project status

The project has an authoritative local RPG simulation, movement/running, gathering/crafting, melee/AI/loot, quests/dialogue, shops/banking and versioned saves. River Crossing is an authored flat starter candidate. The bounded Python/Tk editor supports shared edits/history, guarded new drafts, reopen, isolated playtests and overlays. Eight art batches and eight-way walking/chopping/mining use the shared cache.

The [20 November review](docs/embervale/06-development/PROJECT_REVIEW_2026-11-20.md) fixed three player-data risks and verified 102/102 Windows SDL CTest entries, 87/87 standalone headless and 42/42 Python editor tests. The later [action-audio follow-up](docs/embervale/06-development/ACTION_AUDIO_2026-11-20.md) implements 15 original effects and effects volume/mute, fixes shared asset copying and passes 107/107 Linux SDL entries, 89/89 standalone headless and 42/42 Python units. The later [starter continuation](docs/embervale/06-development/STARTER_COMBAT_RECOVERY_2026-11-20.md) fixes skipped tutorial checkpoints and verifies combat, food, death, persisted recovery and six save/reload checkpoints: 109/109 Linux SDL and 91/91 standalone headless entries. The latest [periodic autosave](docs/embervale/06-development/PLAYER_AUTOSAVE_2026-11-20.md) completes GAME-010 and passes 112/112 Linux SDL, 93/93 standalone headless and 42/42 Python units. Audible/Windows/uncoached starter acceptance remains separate. The latest [editor recovery](docs/embervale/06-development/EDITOR_RECOVERY_2026-11-20.md) completes bounded STUDIO-025: 113/113 Linux SDL, 94/94 headless and 53/53 Python units per interpreter, plus a native restore. The latest [fixed world publication](docs/embervale/04-studio/WORLD_PUBLICATION.md) completes bounded STUDIO-026: exact-byte validation receipts, named manifest-last versions and Studio-selected fresh launch/rollback, with 116/116 Linux SDL, 96/96 headless and 64/64 Python tests per interpreter. The latest [asset validation](docs/embervale/06-development/ASSET_RESOURCE_VALIDATION_2026-11-20.md) completes current-art STUDIO-063: frame/PNG/resource budgets, owned decoder bytes, guarded copying and memory accounting, with 120/120 Linux SDL, 98/98 headless and 79/79 Python tests per interpreter. Current builds also require zlib development headers/library; see [setup](docs/embervale/06-development/GETTING_STARTED.md). In-place replacement, general importing and measured starter acceptance remain open.

Local melee/equipment still needs reconciliation with published Mining. Its visual quality is a first pass, with polish owner-deferred. See [current status](docs/embervale/01-current-state/CURRENT_STATUS.md) for the source distinction.

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

Reconcile and publish the working Mining/melee/equipment version, observe the fresh-player journey and resolve blocking navigation/combat/recovery issues, then finish audible acceptance of the implemented sound pass. The [roadmap](docs/embervale/02-roadmap/MASTER_ROADMAP.md) and [backlog](docs/embervale/02-roadmap/BACKLOG.md) retain reliability, presentation and authoring work.

Elevation, connected regions, companions, bosses and multiplayer remain separate milestones. No complete Studio release or online MMO infrastructure is claimed.

## Contributing

Read the [contribution workflow](docs/embervale/06-development/CONTRIBUTING.md), [testing strategy](docs/embervale/06-development/TESTING.md), [save compatibility contract](docs/embervale/03-engine/SAVE_COMPATIBILITY.md) and the relevant task before changing runtime behaviour. Changes to a documented feature should update its specification, implementation status and evidence in the same pull request.

No project licence is asserted by this README. Confirm ownership and choose an appropriate repository licence before distributing third-party contributions or assets; record asset provenance as described in [Art and audio](docs/embervale/05-game/ART_AND_AUDIO.md).

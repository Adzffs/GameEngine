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
| Plan and improve custom player characters | [Custom character design and delivery plan](docs/embervale/05-game/CUSTOM_CHARACTERS.md) |
| Inspect the first reusable base and its limitations | [Pilot review and improvement register](docs/embervale/06-development/CUSTOM_CHARACTER_REVIEW_2026-11-22.md) |
| Produce compatible character artwork | [PixelLab character workflow](docs/embervale/05-game/CUSTOM_CHARACTER_WORKFLOW.md) |
| Open the retained body, clothing layers and preview | [Custom character pilot](assets/embervale/custom-character-pilot/README.md) |
| Reuse animation research and measured findings | [Animation research, 20 November](docs/embervale/06-development/ANIMATION_RESEARCH_2026-11-20.md) |
| Check combat, food and recovery verification | [Starter replay and corrected tutorial checkpoints](docs/embervale/06-development/STARTER_COMBAT_RECOVERY_2026-11-20.md) |
| Check when progress is saved | [Autosave policy, retries and interruption/reload evidence](docs/embervale/06-development/PLAYER_AUTOSAVE_2026-11-20.md) |
| Recover unfinished map edits | [Private recovery, source conflicts and interrupted-write results](docs/embervale/06-development/EDITOR_RECOVERY_2026-11-20.md) |
| Publish or roll back a map version | [Studio validation, fixed versions and selected launch](docs/embervale/04-studio/WORLD_PUBLICATION.md) |
| Check sprite resources and memory budgets | [Read-only art validation, costs and runtime limits](docs/embervale/05-game/ASSET_RESOURCE_LIMITS.md) |
| Use sound effects and volume controls | [Action audio and packaging results](docs/embervale/06-development/ACTION_AUDIO_2026-11-20.md) |
| Find every document | [Documentation index](docs/embervale/README.md) |

## Project status

**Custom characters, 22 November:** The [first PixelLab base/layer proof](assets/embervale/custom-character-pilot/README.md) is preserved with eight standing views, south idle/walk and separate tunic/trousers. The owner requests improvements. Its [plan](docs/embervale/05-game/CUSTOM_CHARACTERS.md) covers body quality/scale, compatible layers, equipment, renderer integration, safe appearance saves and creator controls. This is reference art outside the game catalogue; no finished creator or runtime replacement is claimed.

The project has an authoritative local RPG simulation, movement/running, gathering/crafting, melee/AI/loot, quests/dialogue, shops/banking and versioned saves. River Crossing is an authored flat starter candidate. The bounded Python/Tk editor supports shared edits/history, guarded new drafts, reopen, isolated playtests and overlays. Nine art batches and eight-way walking/chopping/mining/melee with visible equipment use the shared cache.

The [project review](docs/embervale/06-development/PROJECT_REVIEW_2026-11-20.md) records the verified player-data corrections. Periodic player autosave, private editor recovery, fixed world publication and bounded art resources are implemented. Current builds require zlib development headers/library alongside the selected toolchain; see [setup](docs/embervale/06-development/GETTING_STARTED.md). In-place world replacement, general importing and broader installation acceptance remain open.

The [player action integration](docs/embervale/06-development/PLAYER_ACTION_INTEGRATION_2026-11-20.md) combines recovered melee/equipment with current Mining/audio: 124/124 Windows SDL checks, 99/99 standalone headless, and 81 Python tests (79 passed/two unavailable-symlink skips). All 32 editable masters are preserved; 3,063 frames/32 textures fit the unchanged budgets. Animation polish remains owner-deferred; physical-device audio and uncoached starter acceptance stay open.

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

The owner-selected custom-character work begins with ART-006: compare the improved base at world scale and prove one layered character in the existing renderer. Follow the [staged character plan](docs/embervale/05-game/CUSTOM_CHARACTERS.md) and preserve the separate walking/equipment candidate. Further actions, wardrobe coverage, persistence and player controls have explicit backlog entries. Existing starter acceptance remains open below.

Use the verified combined Mining/melee/equipment version to observe the fresh-player journey and resolve blocking navigation/combat/recovery issues, then finish audible acceptance of the implemented sound pass. The [roadmap](docs/embervale/02-roadmap/MASTER_ROADMAP.md) and [backlog](docs/embervale/02-roadmap/BACKLOG.md) retain reliability, presentation and authoring work.

Elevation, connected regions, companions, bosses and multiplayer remain separate milestones. No complete Studio release or online MMO infrastructure is claimed.

## Contributing

Read the [contribution workflow](docs/embervale/06-development/CONTRIBUTING.md), [testing strategy](docs/embervale/06-development/TESTING.md), [save compatibility contract](docs/embervale/03-engine/SAVE_COMPATIBILITY.md) and the relevant task before changing runtime behaviour. Changes to a documented feature should update its specification, implementation status and evidence in the same pull request.

No project licence is asserted by this README. Confirm ownership and choose an appropriate repository licence before distributing third-party contributions or assets; record asset provenance as described in [Art and audio](docs/embervale/05-game/ART_AND_AUDIO.md).

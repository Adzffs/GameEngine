# Repository guide and developer orientation

**Status:** SOURCE VERIFIED orientation to `e9d32932e8f9b27e96283313cca320058792a81f`; proposed integration notes are labelled separately.\
**Reviewed:** 18 November 2026.

## 1. Start here

The current repository is a C++20 game prototype with an SDL3 client and a reusable headless core. Start with the existing build rather than creating a second build system for Studio. Read this guide alongside [Implemented Systems](IMPLEMENTED_SYSTEMS.md), which explains behaviour and limitations, and [Evidence Ledger](EVIDENCE_LEDGER.md), which explains how claims were verified.

The complete reviewed tree contains 363 tracked files: 272 below `src/`, 81 test `.cpp` files plus their support header, three existing Markdown documents, and build/editor configuration. This is a source inventory, not a quality or completion score. There were no tracked binaries or generated build directories in that tree. [Pinned tree][RG-TREE]

### Recommended first reading order

1. `CMakeLists.txt`: target boundaries, dependencies, and test groups.
2. `src/main.cpp`: startup validation and development save-path policy.
3. `src/Core/Engine.cpp`: client lifecycle, frame input, timing, and rendering integration.
4. `src/World/World.h` and selected portions of `World.cpp`: authoritative state and orchestration.
5. `src/Command/ServerCommand.h`: the intended command surface and session/revision data.
6. `src/World/Development/DevelopmentWorldContent.*` and `src/NPC/NpcSpawnDatabase.cpp`: the actual map baseline.
7. `src/Persistence/PlayerSaveData.h`, codec, and file store: saved state and historical compatibility.
8. The specific system and tests involved in the task; do not use this reading list as a reason to refactor unrelated code.

These paths are present at the pinned commit. [Build][RG-CMAKE] · [entry point][RG-MAIN] · [World][RG-WORLD] · [commands][RG-COMMAND]

## 2. Code ownership map

“Ownership” here means architectural responsibility, not an assigned person.

| Folder | Responsibility | Typical reason to open it |
|---|---|---|
| `src/Core/` | Engine lifecycle, development configuration, startup validation, random-source abstractions, logging | Startup behaviour, frame loop, configuration. |
| `src/Time/`, `src/Scheduling/` | Fixed tick timing, performance timing, scheduled events | Tick pacing, event deadlines, overrun diagnosis. |
| `src/World/` | World orchestration, map, tile rules, development placements, ground items and recovery piles | World construction, collision, authoritative integration. |
| `src/Entity/`, `src/Player/`, `src/NPC/` | Entity identity, managers, players, monsters, friendly NPCs, definitions and spawns | Actor lifecycle, spawn rules, entity lookup. |
| `src/Command/` | Command variants, queue, processing result | User intent and authoritative rejection/success. |
| `src/Movement/`, `src/Pathfinding/` | Movement intent/execution and route search | Travel, running, blocked targets, path safety. |
| `src/Interaction/` | Approaching and using world targets | Resource/station/NPC interaction handoff. |
| `src/Action/` | Timed action lifecycle and cancellation | Gathering, recipe production, melee timing. |
| `src/Combat/`, `src/AI/` | Combat resolution, engagement, feedback data, monster decisions | Attacks, aggression, retaliation, enemy behaviour. |
| `src/Stats/`, `src/StatusEffect/`, `src/Equipment/` | Derived ratings, health, effects, equipped items | Bonuses, survivability, effects, equipment changes. |
| `src/Gathering/`, `src/Skills/` | Gathering completion and skill progression | Resource rewards, XP, requirements. |
| `src/Inventory/`, `src/Item/`, `src/Bank/` | Slot/stack operations, item definitions, bank storage/session data | Transfers, quantities, item use and storage. |
| `src/Recipe/` | Recipes, matching, crafting interaction/session model, discovery, production pacing | Grid crafting and material processing. |
| `src/Reward/`, `src/Requirement/`, `src/Economy/` | Rewards, shared requirements, balance calculations | Loot, unlock checks, economic analysis. |
| `src/Dialogue/`, `src/Shop/` | Dialogue graphs/sessions/presentation and shop definitions/sessions | Conversation choices and trade. |
| `src/Quest/`, `src/Tutorial/`, `src/Progression/` | Quest definitions/state, tutorial steps, presentation state | Goals, journal guidance, unlock and completion feedback. |
| `src/Persistence/` | Player capture/reconstruction, text codec, file store, lifecycle results | Save formats, migration, failure recovery. |
| `src/Content/` | Catalogue validation and structured reports | Definition correctness and cross-reference checks. |
| `src/Graphics/`, `src/Input/` | SDL presentation, input-facing client state | Camera, hitboxes, panels, procedural visuals. |
| `tests/` | Behaviour and integration test executables | Existing regression coverage and new focused evidence. |
| `.github/` | Contribution templates | Bug reports, feature requests and pull request context. |

The map is based on the complete source tree and the CMake source list. Some folders hold headers while orchestration still lives in `World.cpp`; folder names should not be interpreted as perfectly isolated services. [Tree][RG-TREE] · [CMake][RG-CMAKE]

## 3. Build and run reference

These configurations have subsequently been reproduced on Linux, including all 81 original tests with SDL’s dummy driver. The exporter adds optional content-tool targets and two CTest checks; see the [verification record](../06-development/BASELINE_VERIFICATION_2026-11-18.md). Compiler, CMake, Ninja and SDL remain environment prerequisites.

### Headless build

From the repository root:

```bash
cmake -S . -B build-headless -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON \
  -DGAMEENGINE_BUILD_CLIENT=OFF \
  -DGAMEENGINE_BUILD_GRAPHICS_TESTS=OFF
cmake --build build-headless --parallel
ctest --test-dir build-headless --output-on-failure -L headless
```

This deliberately disables both SDL-dependent targets. It is the useful baseline for engine changes and future headless Studio validation. A headless build alone cannot establish graphics/input correctness.

### SDL client and complete configured test groups

For a Windows build, use an MSYS2 **UCRT64** shell and matching UCRT64 GCC, CMake, Ninja, and SDL3 packages. Keep the compiler, libraries, and build directory in the same toolchain family. See [Getting started](../06-development/GETTING_STARTED.md).

```bash
cmake -S . -B build-client -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON \
  -DGAMEENGINE_BUILD_CLIENT=ON \
  -DGAMEENGINE_BUILD_GRAPHICS_TESTS=ON
cmake --build build-client --parallel
ctest --test-dir build-client --output-on-failure -L headless
SDL_VIDEODRIVER=dummy ctest --test-dir build-client --output-on-failure -L client
SDL_VIDEODRIVER=dummy ctest --test-dir build-client --output-on-failure -L graphics
```

The client target is `GameEngine`; with this single-configuration Ninja setup, its Windows executable is normally `build-client/GameEngine.exe`. Launch the game without the dummy video driver for a manual visual playtest. Run from a deliberate working directory because the development save path is relative to it. [CMake][RG-CMAKE] · [Entry point][RG-MAIN]

Use `ctest --test-dir build-client -N` to inspect configured tests, and `ctest --test-dir build-client --output-on-failure -R '^ExactTestTarget$'` for an existing targeted suite. Record the exact commit, options, command, and result in evidence. Listing tests is not running them.

### Existing save policy

`src/main.cpp` opts into `saves/development-player.save`. An `EngineConfiguration` with no save path retains the nonpersistent path. Controlled shutdown saves; ticks do not periodically autosave. Preserve test save fixtures separately from a player's ordinary save. Changing the map requires testing player-position fallback and recovery data, not merely reopening a save successfully. [Persistence policy][RG-PERSISTENCE] · [World][RG-WORLD]

## 4. Important integration seams

| Change | Read before implementation | Preserve |
|---|---|---|
| Export or import the starter map | Development world content, Map constructor, World constructor, spawn database | Terrain order, `(10, 10)` water, base ground, object blocking, lifecycle order. |
| Move an NPC or enemy | Spawn definitions, `World::CreateNpc/CreateMonster`, spawn manager | Canonical-definition validation, active counts, wandering/respawn settings. |
| Add an item or recipe | Enum, database, requirements, rewards, save token codec, UI | Stable meanings of existing IDs, known-recipe state, authoritative transactions. |
| Change quest logic | Quest definitions/system, dialogue, World events, save versions | Prerequisites, reward-once behaviour, old save interpretation. |
| Improve art | Graphics drawing, world transforms, hit testing, camera tests | Visual/collision agreement, click accuracy, fallback readability. |
| Change world topology | Map/pathfinding/movement, interaction, AI, saves | Traversability, approach rules, spawn/recovery validity, deterministic travel. |

At the baseline, `World.cpp` is approximately 5,568 lines and `Graphics.cpp` approximately 6,237 lines. These are maintainability hotspots, not a mandate for a wholesale rewrite. Extract the specific reusable seam required by each milestone and prove behaviour remains consistent. [World][RG-WORLD] · [Graphics][RG-GRAPHICS]

## 5. Existing documentation and reconciliation

Three documents are present on the reviewed default branch:

| Existing document | Material to preserve | Maintenance needed |
|---|---|---|
| `docs/DevelopmentPlayerPersistence.md` | Working-directory save policy, opt-in configuration, controlled shutdown, structured failures, limits of durability | Link it to the expanded compatibility and recovery specification; retain its guarantee boundaries. |
| `docs/EconomyLedger.md` | Item sources/sinks, first-upgrade target, travel and inventory costs, anti-arbitrage rule, executable balance model | Reconcile stale rows with current recipes/shop/quests; label targets as targets. |
| `docs/StarterCharacters.md` | Six character profiles, examine lines, voice and relationship loop | Keep narrative intentions separate from implemented artwork and service behaviour. |

Sources: [Persistence][RG-PERSISTENCE] · [Economy Ledger][RG-ECONOMY] · [Starter Characters][RG-CHARACTERS]

At the 18 November baseline review, an earlier documentation foundation was open as [PR #1][RG-PR1]. It contains eighteen proposed files, including game direction, design, roadmap, workflow, templates, and a September repository snapshot. Its direction and templates are useful inputs, while its current-status page and implementation assumptions are older than the November gameplay work. The new documentation pack should reconcile these explicitly. It should not silently merge a stale snapshot or imply that PR #1 was already merged.

### Proposed cleanup policy

The reviewed `.gitignore` already excludes `.vscode/`, but three editor configuration files are tracked. Their launch settings contain an absolute `c:/GameEngine` path and an `outDebug` executable path that differs from the current CMake target. These are machine-specific development settings rather than a portable project contract. A clean integration can remove those tracked copies after the CMake build/debug guidance is supplied; the ignore rule then allows individual local settings. [Editor launch settings][RG-LAUNCH] · [Ignore rules][RG-IGNORE]

The ignore rules cover `build_client/`, `build_headless/`, `build-client/`, `build-headless/` and local saves. Keep intended fixture files visible. There is no basis in the reviewed tree for deleting source, tests, or the three useful existing documents as generated clutter. [Ignore rules][RG-IGNORE] · [Tree][RG-TREE]

## 6. When returning after a break

First read the current roadmap and evidence ledger, then inspect the actual branch tip. Match a work item to a bounded feature or content brief. Establish whether its prerequisites and baseline tests are satisfied. Make the implementation branch from the intended current base, keep the change focused, and record the resulting commit and evidence. A historical date or this pinned snapshot should never override a newer verified repository state.

[RG-TREE]: https://github.com/Adzffs/GameEngine/tree/e9d32932e8f9b27e96283313cca320058792a81f
[RG-CMAKE]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/CMakeLists.txt
[RG-MAIN]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/main.cpp
[RG-WORLD]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/World.cpp
[RG-COMMAND]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Command/ServerCommand.h
[RG-PERSISTENCE]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/docs/DevelopmentPlayerPersistence.md
[RG-GRAPHICS]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Graphics/Graphics.cpp
[RG-ECONOMY]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/docs/EconomyLedger.md
[RG-CHARACTERS]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/docs/StarterCharacters.md
[RG-PR1]: https://github.com/Adzffs/GameEngine/pull/1
[RG-LAUNCH]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/.vscode/launch.json
[RG-IGNORE]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/.gitignore

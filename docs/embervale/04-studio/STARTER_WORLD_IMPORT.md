# Validate and play a starter-world snapshot

**Status:** Implemented starter-content runtime bridge; shared editing, guarded publication, undo and the visual editor remain planned.\
**Updated:** 18 November 2026\
**Format:** `embervale.starter-world-snapshot`, schema version `1`, region `embervale.development_starter`.\
**Related:** [Export guide](STARTER_WORLD_EXPORT.md), [runtime import decision](../07-decisions/ADR_017_BOUNDED_STARTER_WORLD_IMPORT.md), [save compatibility](../03-engine/SAVE_COMPATIBILITY.md).

## 1. What is available

The game can load an explicitly selected JSON snapshot of its existing 100×100 starter region. Terrain, placement coordinates and the player-spawn coordinate can change. The document must retain the current 20 placement identities: seven resources, three stations and ten NPC/monster spawns. Their definition bindings, spawn settings and creation stages remain fixed.

`EmbervaleWorldValidate` reads a candidate and applies the same parser and runtime validation used by game startup. `GameEngine --world <path>` then constructs the world from the validated snapshot. A missing, malformed or unsafe file stops startup with a diagnostic; it does not silently select a different world.

The game and validator do not write the selected world file. There is no implemented map-move command, draft-save command, undo history, publication manifest or graphical editor. Coordinate edits can be supplied in a candidate JSON document, but the complete safe authoring workflow remains the next milestone.

## 2. Build and validate without SDL

From the repository root, use a C++20 compiler, CMake 3.20 or newer and Ninja:

```bash
cmake -S . -B build-headless -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DGAMEENGINE_BUILD_CLIENT=OFF -DGAMEENGINE_BUILD_GRAPHICS_TESTS=OFF -DGAMEENGINE_BUILD_CONTENT_TOOLS=ON
cmake --build build-headless --parallel 4
ctest --test-dir build-headless --output-on-failure -R 'StarterWorld|GameLaunchOptions'
./build-headless/EmbervaleWorldValidate tests/fixtures/content/development-starter.snapshot.json
```

To create a separate baseline file for inspection, choose a new destination name:

```bash
./build-headless/EmbervaleWorldExport > development-starter.playtest.json
./build-headless/EmbervaleWorldValidate development-starter.playtest.json
```

Shell redirection creates or replaces its destination. The exporter always captures compiled initial content; it does not re-export a running imported world. Keep the reviewed fixture intact when creating a candidate.

`GAMEENGINE_BUILD_CONTENT_TOOLS=OFF` omits the standalone export/validation executables and their optional checks. The parser, runtime validator and compatibility adapter remain in the SDL-free core so the game can still use `--world`.

### Validator command results

| Invocation or outcome | Result |
|---|---|
| `EmbervaleWorldValidate <snapshot.json>` | Read and validate the explicit file |
| `EmbervaleWorldValidate --help` | Usage; exit `0` |
| Valid document | Dimensions, placement count and player-spawn summary; exit `0` |
| Invalid document, unsafe geography or file-read failure | Diagnostic on standard error; exit `1` |
| Missing, extra or unsupported arguments | Usage on standard error; exit `2` |

The current output is plain text. Machine-readable diagnostics and a capabilities command remain part of the [planned CLI](CLI_AND_AI_CONTRACT.md).

## 3. Build and launch the client

Install SDL3 for the selected toolchain; [Getting started](../06-development/GETTING_STARTED.md) includes the Windows UCRT64 packages. Configure a separate client build:

```bash
cmake -S . -B build-client -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DGAMEENGINE_BUILD_CLIENT=ON -DGAMEENGINE_BUILD_GRAPHICS_TESTS=ON -DGAMEENGINE_BUILD_CONTENT_TOOLS=ON
cmake --build build-client --parallel 4
./build-client/EmbervaleWorldValidate development-starter.playtest.json
./build-client/GameEngine --world development-starter.playtest.json
```

If SDL3 is installed in a private prefix, add `-DCMAKE_PREFIX_PATH=/absolute/path/to/sdl-install` to the configure command. In an MSYS2 UCRT64 shell on Windows, use `./build-client/GameEngine.exe` and the corresponding `.exe` tool names. Run visible playtests with the normal display environment; the dummy SDL driver is for automated tests.

### World and save selection

| Game arguments | World | Player persistence |
|---|---|---|
| No arguments | Original compiled starter world | `saves/development-player.save` |
| `--world candidate.json` | Validated selected snapshot | Temporary; no player save is read or written |
| `--world candidate.json --save saves/map-playtest.save` | Validated selected snapshot | Explicit isolated save |
| `--compiled-world` | Original compiled starter world | Normal development save |
| `--compiled-world --no-save` | Original compiled starter world | Temporary |
| `--save saves/alternate.save` | Original compiled starter world | Explicit alternate save |
| `--no-save` | Original compiled starter world | Temporary |
| `--help` | No game startup | Show usage only |

All relative paths resolve from the process working directory. Duplicate world selectors, `--world` with `--compiled-world`, duplicate save selectors, `--save` with `--no-save`, missing values and unknown arguments are rejected before game startup. Invalid arguments return `2`; a selected-content startup failure returns `1`.

For a persistent map playtest, select a dedicated save explicitly:

```bash
./build-client/GameEngine --world development-starter.playtest.json --save saves/map-playtest.save
```

The save target and its `.tmp` and `.bak` recovery paths must be separate from the world input. Startup checks normalized paths and existing file aliases, including symlinks and hard links, to prevent the player-save transaction from replacing the selected content. This is startup path validation, not a general concurrent-filesystem transaction guarantee.

Omitting `--save` during an imported-world playtest avoids reading or changing the normal development save. Normal controlled shutdown remains the persistence boundary when a save is selected. Temporary players still use existing development-player defaults, including skill boosts; they do not establish the planned unboosted starter profile.

## 4. Input contract

The file reader checks for a regular file and enforces **2 MiB** both before reading and during reading. Parsing is strict: unknown fields, duplicate keys, unsupported schema revisions, malformed integers, trailing input and unsupported string encoding fail explicitly.

| Boundary | Current accepted limit |
|---|---|
| Input bytes | At most 2 MiB |
| JSON nesting | Depth at most 12, with the root at depth 0 |
| Total JSON values | At most 65,536 |
| Array elements | At most 16,384 per array |
| Object keys | At most 32 per object |
| Decoded string length | At most 16,384 characters; schema fields have tighter limits |
| Encoding | ASCII strings; JSON Unicode escapes are unsupported |
| Structural model dimensions | 1–1024 per axis; not a runtime-map support claim |
| Runtime map dimensions | Exactly 100×100 |
| Runtime placements | Exactly the current 20 stable IDs and bindings |
| Surface/exits | One `ground` surface at elevation `0`; no region exits |

The terrain rows must match dimensions exactly and use `G` grass, `D` dirt, `S` stone or `W` water. Positions must be in bounds. The current stable-ID registry binds each placement to its resource/station/NPC type, lifecycle stage and order. Spawn IDs, wander radius/interval, active population limit and respawn setting must match the supported definition. Adding/removing placements or changing those settings requires a deliberate registry/content-system extension.

## 5. Geography validation and runtime construction

Validation checks the complete initial map before runtime construction:

- Every placement has distinct coordinates and walkable ground; placing it on water is rejected.
- Resource and station occupancy blocks movement. Actors retain the existing nonblocking tile semantics.
- The player spawn is walkable and clear of authored placements.
- Every actor spawn is reachable from the selected player spawn.
- Every placement has a reachable cardinal approach tile, preserving service and interaction access.

Reachability uses cardinal connectivity. With the current rule forbidding diagonal corner cutting, this describes the same connected components as legal eight-direction travel. It does not measure route quality, travel time, encounter difficulty or the enjoyment of an edited area.

The adapter installs all underlying ground cells, then applies existing resource/station occupancy. Moving a resource in the input therefore leaves the exact authored terrain at its old coordinate. Spawn definitions belong to the selected `World`, so imported spawn coordinates remain authoritative without mutating the global compiled database. Existing type, capacity and lifecycle checks remain in force.

The `world_start` and `first_player_registration` stages retain their explicit creation order. The first creates the guide, two early monsters, resources and stations; the second creates the advanced monsters and supporting characters only after player registration. Reordering the JSON array does not reorder runtime creation. A fresh compiled `World` remains independent of an imported one.

## 6. Player positions and recovery

The snapshot’s selected player spawn is used for new players, death respawns and incompatible saved-position fallback. Valid saved positions remain valid only when the selected map permits them and they are connected to its safe starting area. Recovery state must also remain accessible; the current starter-map relocation policy preserves the pile’s items, ownership and remaining expiry ticks while choosing a safe position when its old location cannot be used. See [Save compatibility](../03-engine/SAVE_COMPATIBILITY.md) for the bounded policy and remaining migration scope.

The player-save format remains version 13. The imported map is still the same legacy-compatible region on `ground`; multi-region save identity, migration manifests and general content-version migrations remain future work.

## 7. Verification and remaining work

The focused suites cover parser budgets and rejection, runtime geography validation, unchanged-world parity, changed terrain and placement coordinates, creation stages, selected spawn authority, persistence fallback and command/path handling. CLI tests invoke the actual exporter and validator. Reproduce the relevant suites on the actual commit rather than treating a prior exporter result as an importer result.

```bash
ctest --test-dir build-headless --output-on-failure -R 'StarterWorld|GameLaunchOptions'
ctest --test-dir build-headless --output-on-failure -L headless
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDL_AUDIODRIVER=dummy ctest --test-dir build-client --output-on-failure --no-tests=error -L '^(headless|client|graphics)$'
```

On 18 November 2026, this slice passed **77/77 headless tests**, **89/89 tests** in the Linux SDL configuration and **2/2 selected runtime/launch tests** with standalone content tools disabled. A real game process loaded a tree moved to `(40,20)`, ran for two seconds and stopped gracefully with exit `0`, creating no files. The launch used dummy video/software rendering, so it does not establish visible manual playtest quality. See the [runtime verification record](../06-development/RUNTIME_IMPORT_VERIFICATION_2026-11-18.md) for the exact source, commands and environment.

Implementation and tests: [parser](../../../src/Content/Export/StarterWorldSnapshotInput.cpp), [runtime input](../../../src/Content/Import/StarterWorldRuntimeInput.cpp), [World](../../../src/World/World.cpp), [launch options](../../../src/Core/GameLaunchOptions.cpp), [parser tests](../../../tests/StarterWorldSnapshotInputTests.cpp), [input tests](../../../tests/StarterWorldRuntimeInputTests.cpp), [runtime tests](../../../tests/StarterWorldRuntimeTests.cpp), [launch tests](../../../tests/GameLaunchOptionsTests.cpp), [validator CLI tests](../../../tests/StarterWorldValidateCliTests.cmake), [real application startup tests](../../../tests/GameEngineStartupCliTests.cmake).

The next bounded step is a shared operation that moves an existing placement by stable ID, produces a reviewable candidate, and uses this same validator. Guarded saving with conflict detection, undo/redo, preview and a minimal visual editor then complete the authoring path. This importer alone does not complete R1, R2 or Studio Alpha 0.1.

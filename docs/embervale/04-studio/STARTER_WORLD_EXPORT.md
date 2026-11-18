# Export the current starter world

**Scope:** First read-only Studio foundation. The executable exports compiled initial content; it is not a map editor or a runtime map loader.\
**Format:** `embervale.starter-world-snapshot`, schema version `1`.\
**Implementation:** [snapshot model and writer](../../../src/Content/Export/StarterWorldSnapshot.h), [command entry point](../../../src/Tools/WorldExportMain.cpp).

## What this makes possible

The current starter area's geography can now be inspected as one deterministic JSON document. It includes the ground beneath objects, every resource and workstation, and all ten NPC/monster spawn definitions, including actors created only after the first player is registered. The checked-in [baseline snapshot](../../../tests/fixtures/content/development-starter.snapshot.json) is a reviewable reference for the next importer and editing work.

The game continues to construct its world from the existing compiled definitions. Editing the exported JSON does not change the game. The next integration step must prove that loading a document reproduces the current world before it supports moving a tree.

## Build and run

Use the normal headless build from the repository root:

```bash
cmake -S . -B build-headless -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DGAMEENGINE_BUILD_CLIENT=OFF -DGAMEENGINE_BUILD_GRAPHICS_TESTS=OFF
cmake --build build-headless --parallel
ctest --test-dir build-headless --output-on-failure -L headless
./build-headless/EmbervaleWorldExport > development-starter.snapshot.json
```

On Windows, use the corresponding executable in your build directory, for example `build-client/EmbervaleWorldExport.exe`. Use an MSYS2 shell or another shell that preserves redirected UTF-8 bytes; avoid text transformations when comparing canonical files. The program itself emits LF on Windows and Linux.

`GAMEENGINE_BUILD_CONTENT_TOOLS` defaults to `ON`. Set it to `OFF` to omit the snapshot library, executable and its two tests. The tool requires no SDL when both client and graphics tests are disabled. It adds no JSON-library or Python requirement to the C++ build.

| Invocation | Result |
|---|---|
| `EmbervaleWorldExport` | Canonical JSON on standard output; no progress messages mixed into JSON |
| `EmbervaleWorldExport --help` | Usage and scope |
| `EmbervaleWorldExport --version` | Exporter snapshot schema version |
| Any other arguments | Exit `2`, diagnostic on standard error, empty standard output |
| Capture, validation or output failure | Exit `1` with a diagnostic |
| Successful operation | Exit `0` |

The exporter accepts no input filename and has no `--output`, edit or save option. Shell redirection chooses the destination and can overwrite that destination; use a new output filename when keeping a previous snapshot. The executable does not inspect player saves, enter the game lifecycle, or create files in its working directory.

## What the document contains

| Section | Meaning |
|---|---|
| `format`, `schema_version` | Identifies this export contract separately from a future loadable authoring format |
| `region_id`, `revision` | `embervale.development_starter`, initial snapshot revision `1` |
| `size` | Actual 100×100 development-map dimensions |
| `surfaces` | One `ground` surface with elevation `0`; this introduces no elevation capability |
| `terrain` | Restricted ASCII legend and 100 rows of 100 underlying ground cells |
| `player_spawn` | Default initial position `(0,0)` on `ground` |
| `placements` | Seven resources, three stations and ten NPC/monster spawn records |
| Placement initialization | Explicit lifecycle stage and order, independent of JSON array order |
| Spawn properties | Existing spawn binding, wander radius/interval, active limit and respawn flag |
| `region_exits` | Empty: the current development map has no authored region-transition system |

Coordinates are integer tiles: origin at the top-left, X rightward and Y downward. The terrain legend is `G` grass, `D` dirt, `S` stone and `W` water. The single water tile at `(10,10)` survives the ordered terrain overlays.

Ground and placements are separate in the snapshot. The legacy runtime still marks resources—including rocks—as `TREE`, and stations as `STATION`. Export tests reconstruct those occupied tiles before comparing all 10,000 cells and passability with an actual `World`. This retains the old behaviour while preserving the ground needed when an object is eventually moved.

Live state is excluded: player inventory, current health, wandering positions, resource depletion, pending commands and temporary runtime IDs do not belong in this initial-content document. Fixed renderer labels and reusable gameplay catalogues are inventoried separately in the [starter baseline](../03-engine/STARTER_WORLD_BASELINE.md); their externalisation remains future work.

### Bounds of the current writer

The in-memory writer accepts dimensions from 1 through 1024, at most 4096 placements, and identity strings of 1–128 characters from lowercase ASCII letters, digits, `.`, `_` and `-`. Rows must exactly match the dimensions and use only `G`, `D`, `S` and `W`. Positions must be within bounds; placement IDs and stage/order pairs must be unique. Typed definition bindings and spawn fields are checked for consistency.

The actual exporter captures only the current compiled 100×100 baseline and its explicit registry. These writer limits do not declare larger-map gameplay support or define a hostile-file parser policy. Overlap rules, blocked-spawn checks, full catalogue compatibility and edited-world validation remain separate requirements.

## Identity and ordering

Placement IDs are explicit literals mapped to the current legacy definitions. They are not generated from coordinates, display names, array indices or temporary runtime IDs. The existing starter map has one placement of each resource and station type, so that uniqueness is checked. Adding another placement requires allocating its own identity and updating the registry deliberately.

The JSON array is sorted by placement ID. A separate initialization stage/order records actual creation semantics:

- `world_start`: guide, two early monsters, seven resources and three stations.
- `first_player_registration`: two advanced monsters and five supporting characters, after the player itself is registered.

All ten spawn definitions are registered before initial actors are created. A future importer must reproduce that distinction. Sorting the document must never reorder runtime creation. Moving a placement later must retain its identity and initialization order.

Canonical output has a fixed field order, deterministic integer formatting, LF and a final newline. It includes no timestamp, machine-specific path or local runtime state. Repeated exports of the same compiled content produce identical bytes.

## Verification and fixture updates

[StarterWorldSnapshotTests](../../../tests/StarterWorldSnapshotTests.cpp) compare the model with the actual runtime and verify canonical output. [StarterWorldExportCliTests](../../../tests/StarterWorldExportCliTests.cmake) invoke the executable, parse its JSON with CMake's independent JSON reader, check complete dimensions/counts, compare the reviewed fixture, verify repeatability and argument handling, and check that no files appear in the isolated working directory.

Run only the new focused checks with:

```bash
ctest --test-dir build-headless --output-on-failure -R StarterWorld
```

When intentional starter content changes make the fixture fail, inspect the semantic differences first. Review terrain, identities, spawn definitions and lifecycle keys; then regenerate the fixture and explain the intended changes in the same PR. Do not update it merely to hide an unexpected regression.

See the [verification record](../06-development/BASELINE_VERIFICATION_2026-11-18.md) for exact tested environments and the GitHub CI blocker. A passing export test does not establish that an edited world is safe to load.

## What comes next

The next bounded task is the input contract: choose a bounded parser, validate a candidate without unsafe map access, and connect one canonical definition provider to the runtime adapter. Loading the unchanged baseline must preserve terrain, occupancy, lifecycle and saved-location behaviour. After that, add one shared move operation, guarded persistence, preview and undo.

The [broader CLI contract](CLI_AND_AI_CONTRACT.md) and [content model](../03-engine/CONTENT_MODEL.md) remain proposals for those capabilities. This exporter does not complete their parser, import, validation, publication, migration or editor requirements. [ADR-016](../07-decisions/ADR_016_READ_ONLY_WORLD_EXPORT.md) records why this first step is deliberately bounded.

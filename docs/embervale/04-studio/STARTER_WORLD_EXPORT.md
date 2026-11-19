# Export the current starter world

**Scope:** Read-only compiled baseline export, plus explicit authoring of the separate River Crossing candidate. [Runtime validation/loading](STARTER_WORLD_IMPORT.md) and [shared visual editing](VISUAL_WORLD_EDITOR.md) use the same bounded model.\
**Format:** `embervale.starter-world-snapshot`; default baseline schema `1`, River Crossing candidate schema `2`.\
**Implementation:** [snapshot model and writer](../../../src/Content/Export/StarterWorldSnapshot.h), [command entry point](../../../src/Tools/WorldExportMain.cpp).

## What this makes possible

The current starter area's geography can now be inspected as one deterministic JSON document. It includes the ground beneath objects, every resource and workstation, and all ten NPC/monster spawn definitions, including actors created only after the first player is registered. The checked-in [baseline snapshot](../../../tests/fixtures/content/development-starter.snapshot.json) is a reviewable reference for runtime parity and editing work.

Normal startup constructs the compiled world. `GameEngine --world <snapshot.json>` explicitly selects a validated snapshot, including supported terrain and placement-coordinate changes. The no-argument exporter captures the compiled baseline, irrespective of which content a game instance is using. `--river-crossing` explicitly builds the separate runtime-validated [River Crossing candidate](../../../content/worlds/README.md); it does not alter that baseline. See [Starter-world import](STARTER_WORLD_IMPORT.md) for the exact input contract and isolated playtest behaviour.

## Build and run

Use the normal headless build from the repository root:

```bash
cmake -S . -B build-headless -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DGAMEENGINE_BUILD_CLIENT=OFF -DGAMEENGINE_BUILD_GRAPHICS_TESTS=OFF
cmake --build build-headless --parallel
ctest --test-dir build-headless --output-on-failure -L headless
./build-headless/EmbervaleWorldExport > development-starter.snapshot.json
```

On Windows, use the corresponding executable in your build directory, for example `build-client/EmbervaleWorldExport.exe`. Use an MSYS2 shell or another shell that preserves redirected UTF-8 bytes; avoid text transformations when comparing canonical files. The program itself emits LF on Windows and Linux.

`GAMEENGINE_BUILD_CONTENT_TOOLS` defaults to `ON`. Set it to `OFF` to omit the standalone export/validation executables and their optional tests. Snapshot parsing and runtime loading remain available in the core. The tool requires no SDL when both client and graphics tests are disabled. It adds no JSON-library or Python requirement to the C++ build.

| Invocation | Result |
|---|---|
| `EmbervaleWorldExport` | Canonical JSON on standard output; no progress messages mixed into JSON |
| `EmbervaleWorldExport --river-crossing` | Canonical schema-2 River Crossing candidate, with full production content validation before output |
| `EmbervaleWorldExport --help` | Usage and scope |
| `EmbervaleWorldExport --version` | Default compiled-baseline schema version (1) |
| Any other arguments | Exit `2`, diagnostic on standard error, empty standard output |
| Capture, validation or output failure | Exit `1` with a diagnostic |
| Successful operation | Exit `0` |

The exporter accepts no input filename and has no `--output`, edit or save option. Shell redirection chooses the destination and can overwrite that destination; use a new output filename when keeping a previous snapshot. The executable does not inspect player saves, enter the game lifecycle, or create files in its working directory.

## What the document contains

The table below describes the default compiled-baseline export. The named River
Crossing candidate uses the same terrain/placement contract with arrival `(5,28)`,
31 placements (18 resources, 3 stations, 10 actor records), schema 2 and allocation
counter 12. Its authoring source is [RiverCrossingBlockout.cpp](../../../src/Tools/RiverCrossingBlockout.cpp).

| Section | Meaning |
|---|---|
| `format`, `schema_version` | Identifies the supported starter snapshot contract; a broader authoring schema remains planned |
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

The default exporter captures the current compiled 100×100 baseline and its explicit registry. The explicit `--river-crossing` builder authors another 100×100 candidate with bounded allocated resources and retained actor bindings/lifecycle. These writer limits do not declare larger-map gameplay support or define a hostile-file parser policy. The [runtime validator](STARTER_WORLD_IMPORT.md) additionally enforces the exact 100×100 starter registry, overlap/terrain rules, player-spawn clearance and reachable approaches. Larger or general authoring formats remain separate work.

## Identity and ordering

Placement IDs are explicit literals mapped to the current legacy definitions. They are not generated from coordinates, display names, array indices or temporary runtime IDs. The existing starter map has one placement of each resource and station type, so that uniqueness is checked. Adding another placement requires allocating its own identity and updating the registry deliberately.

The JSON array is sorted by placement ID. A separate initialization stage/order records actual creation semantics:

- `world_start`: guide, two early monsters, seven resources and three stations.
- `first_player_registration`: two advanced monsters and five supporting characters, after the player itself is registered.

All ten spawn definitions are registered before initial actors are created. The runtime importer reproduces that distinction. Sorting the document must never reorder runtime creation. Changing a placement coordinate retains its identity and initialization order.

Canonical output has a fixed field order, deterministic integer formatting, LF and a final newline. It includes no timestamp, machine-specific path or local runtime state. Repeated exports of the same compiled content produce identical bytes.

## Verification and fixture updates

[StarterWorldSnapshotTests](../../../tests/StarterWorldSnapshotTests.cpp) compare the model with the actual runtime and verify canonical output. [StarterWorldExportCliTests](../../../tests/StarterWorldExportCliTests.cmake) invoke the executable, parse its JSON with CMake's independent JSON reader, check complete dimensions/counts, compare the reviewed fixture, verify repeatability and argument handling, and check that no files appear in the isolated working directory.

Run only the new focused checks with:

```bash
ctest --test-dir build-headless --output-on-failure -R StarterWorld
```

When intentional starter content changes make the fixture fail, inspect the semantic differences first. Review terrain, identities, spawn definitions and lifecycle keys; then regenerate the fixture and explain the intended changes in the same PR. Do not update it merely to hide an unexpected regression.

See the [verification record](../06-development/BASELINE_VERIFICATION_2026-11-18.md) for exact tested environments and remaining platform limits. A passing export test does not establish that an edited world is safe to load.

## Input and runtime use

The [bounded parser and runtime importer](STARTER_WORLD_IMPORT.md) accept schema 1 with the complete baseline registry, schema 2 with bounded object allocations/removals, and schema 3 with supported spawn settings. They validate supported geography before runtime construction and preserve ground, spawn authority and creation stages. Parser round-trip/rejection tests and imported-world tests supplement the original exporter checks.

The current input path has a 2 MiB byte budget and independent limits for nesting, total values, strings and containers. It accepts ASCII schema strings and rejects unknown fields, identities, changed spawn settings and unsupported features. Runtime use remains restricted to the existing 100×100 region. The import guide documents all limits, commands and save isolation.

## What comes next

Implement one shared operation that moves an existing placement by stable ID, then add guarded saving, undo and preview. The [broader CLI contract](CLI_AND_AI_CONTRACT.md) and [content model](../03-engine/CONTENT_MODEL.md) describe the remaining authoring capabilities. Export and runtime import do not complete their publication, general migration or graphical-editor requirements. [ADR-016](../07-decisions/ADR_016_READ_ONLY_WORLD_EXPORT.md) records the original export boundary; [ADR-017](../07-decisions/ADR_017_BOUNDED_STARTER_WORLD_IMPORT.md) extends it with explicit runtime loading.

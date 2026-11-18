# Testing, validation and evidence

**Status date:** 18 November 2026\
**Baseline:** `e9d32932e8f9b27e96283313cca320058792a81f`\
**Fresh evidence:** [18 November verification](BASELINE_VERIFICATION_2026-11-18.md) records a passing local Linux baseline and the exporter change. Windows remains unverified.

## 1. Current test counts and executed results

The original engine baseline registers 70 headless, 7 graphics and 4 client executables: 81 total. All 81 passed in a fresh local Linux run at documentation merge `c697386382845f17ce02027e6bade24591a9eb36`, with SDL’s dummy display driver. This is independent of the older PR report.

With `GAMEENGINE_BUILD_CONTENT_TOOLS=ON`, the exporter adds one C++ snapshot test and one CMake CLI integration test: **72 headless + 7 graphics + 4 client = 83 CTest entries**. Both changed configurations passed: 72/72 headless and 83/83 in the SDL client build, with no failures or skips. Disabling content tools omits those two checks. See [current CMake](../../../CMakeLists.txt) and the [machine-readable results](verification/2026-11-18/results.json).

Local Linux success does not establish a Windows result. Platform claims must match the configurations actually exercised.

## 2. Existing verification layers

| Layer | Current examples | What it tells us |
|---|---|---|
| Headless unit/system tests | Combat formulas, inventory transactions, save codec/store, movement, quests, commands and scheduling | Behaviour exercised without SDL graphics |
| Headless integration tests | World/player persistence, quest/world interactions, gathering, crafting and shops | Interactions across authoritative systems |
| Client tests | Engine persistence, dialogue, shop and crafting integration | Selected application/input/presentation integration |
| Graphics tests | Camera, combat feedback, quest and crafting presentation | Selected rendering/presentation behaviour |
| Startup content validation | Compiled catalogue validation before running the engine | Invalid supported content is rejected at startup |
| Manual playtests | Actual navigation, comprehension and gameplay observations | Experience not demonstrated by automated assertions |

These categories describe existing registrations and entry points, not proof that each feature is fully covered. Inspect individual assertions when deciding which risk a test protects. [Sources: CMakeLists.txt](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/CMakeLists.txt), [main.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/main.cpp).

## 3. Commands for the current build

Configure the intended environment using [Getting started](GETTING_STARTED.md). Then use the following commands from the repository root:

```bash
ctest --test-dir build-headless -N
ctest --test-dir build-headless --output-on-failure -L headless
ctest --test-dir build-client --output-on-failure -L headless
SDL_VIDEODRIVER=dummy ctest --test-dir build-client --output-on-failure -L client
SDL_VIDEODRIVER=dummy ctest --test-dir build-client --output-on-failure -L graphics
```

The `SDL_VIDEODRIVER` syntax shown is Bash/MSYS2 Bash. These are current CTest commands, unlike the proposed Studio commands elsewhere in the pack. `-N` lists tests without running them. To focus on a known test, use an exact anchored expression such as `-R '^MovementPathSafetyTests$'`; inspect the available names first.

Preserve the test command’s exit status when piping output to a log. In Bash, use `set -o pipefail` or inspect `PIPESTATUS[0]` after `tee` so logging does not hide a failing `ctest` command.

## 4. Studio coverage already present and still required

The [snapshot test](../../../tests/StarterWorldSnapshotTests.cpp) compares all 10,000 effective cells and collision results with a real World, checks ground under occupied tiles and staged actors, uses actual pathfinder endpoints, performs a furnace interaction, and verifies deterministic output and structural rejection. The [CLI test](../../../tests/StarterWorldExportCliTests.cmake) independently parses executable JSON, compares the reviewed fixture and exercises read-only invocation/argument behaviour.

The following broader input/editing coverage remains planned and should be introduced with its corresponding implementation:

| Contract | Meaningful test |
|---|---|
| Input bounds | Reject negative/zero/overflowing dimensions and oversized/deep input before unsafe allocation/indexing |
| Canonical codec | Parse/write/parse preserves semantics; normalisation is deterministic |
| Durable identity | Moving/renaming a placement preserves its ID; copying allocates a new ID |
| References | Unknown definitions/surfaces and duplicate IDs produce precise errors |
| Terrain/occupancy | Moving/deleting a resource restores exact underlying terrain and leaves no phantom blocker |
| Spawn authority | Selected catalogue governs allowed fields without bypassing capacity/lifecycle checks |
| Concurrent writes | Two writers from one base cannot silently overwrite each other; commit recheck catches a changed hash |
| Write interruption | Supported interruption points retain a recoverable draft or previous valid publication |
| Publication | A report for an old hash or incompatible validator cannot publish changed content |
| Undo/redo | Complete command groups restore identity, terrain and references; stale inverses conflict safely |
| Path scope | Traversal/symlink policy and output scope are enforced |
| CLI/GUI equivalence | Equivalent commands produce equivalent canonical content and diagnostics |

Negative tests should assert unchanged committed content and a useful diagnostic, not only that the process exits. Test the actual operation boundary; a unit test of a helper does not alone prove that every CLI entry point uses it correctly.

## 5. Semantic baseline comparisons

The exporter now covers initial construction, first and subsequent player registration, and a representative action/tick sequence. Before enabling the importer, extend that evidence to loaded-content construction and saved-player/recovery compatibility. Delayed NPC/monster creation makes a single startup snapshot insufficient.

Compare terrain, effective collision, durable placement identity, definition references, spawn population/lifecycle, important interactions and persistent state. Preserve creation order during initial migration where compatibility requires it. Keep temporary runtime IDs as a separate observation when relevant; do not define the entire golden master as “all integers match forever.”

The expected baseline must come from independently inspected existing behaviour. A test that computes both expected and actual values using the new adapter can reproduce the same bug twice. Updating a golden snapshot requires explaining the intended change, not merely accepting current output.

## 6. Reachability and gameplay checks

Use authoritative movement and interaction semantics. A target may be close in straight-line distance yet unreachable from the correct side. Validate legal approach tiles, diagonal corner restrictions, occupied footprints and any interaction-specific side/range rules. Flood-fill connectivity can help locate a problem but must not override the actual game's path/interaction result.

Test required paths from spawn to guide, bank, relevant workstation, quest target and region exits. Optional scenic space need not be reachable unless gameplay requires it. Add fixtures for a blocked spawn, a blocked saved position, an obstructed recovery pile and an NPC whose visible location is reachable but legal interaction approach is not.

Run an end-to-end scenario that gathers, processes or crafts, interacts with a named NPC, completes controlled combat/recovery and saves/reloads. Use a deliberately unboosted profile for starter pacing; current development XP settings are unsuitable evidence for a new player's first upgrade time. [Source: DevelopmentConfig.h](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Core/DevelopmentConfig.h).

## 7. Visual and performance verification

Test tile picking across camera pan, zoom, viewport edges and out-of-bounds pointer positions. A preview should record content hash, renderer/tool version and overlays. Pixel snapshots require a pinned rendering environment or documented tolerance; cross-backend pixel equality is not implied by deterministic content.

Measure parse, validation, world construction, pathfinding, tick/frame work, preview and asset memory separately. Start with the actual 100×100 region and measured larger fixtures when the feature requires them. Set budgets after recording hardware/build conditions. Do not claim an improvement from a single noisy run or introduce chunking without a demonstrated bottleneck.

## 8. Evidence record and stopping rule

Every test report should record commit, content hash where relevant, platform/tool versions, configuration, commands, discovered/executed counts, outcomes, logs and unresolved limitations. Use synthetic saves and scrub personal paths/data from public attachments.

For a small change, run the affected meaningful tests plus required project gates. Broaden testing when a concrete risk remains or a release gate requires it. Documentation fixes need structural/link/factual checks, not an invented game test result. Once the scoped change is adequately verified, update the evidence and continue delivery.

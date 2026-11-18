# Testing, validation and evidence

**Status date:** 18 November 2026  
**Baseline:** `e9d32932e8f9b27e96283313cca320058792a81f`  
**Evidence limitation:** No fresh local game build/test run was performed for this documentation pack.

## 1. What the existing test count means

The inspected CMake file registers **70 headless**, **7 graphics** and **4 client** test targets when the required options are enabled: **81 in total**. This is a source registration count, not a result count from a fresh executed suite. A headless-only configuration does not build all 81. [Source: CMakeLists.txt](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/CMakeLists.txt).

The historical game-feel pull request reports that all 81 tests passed. The inspected push workflow for the baseline commit instead has a failed outcome, with no usable step execution/log evidence returned in this review. Its cause is unknown; neither “tests failed” nor “tests currently pass” is established by that workflow observation. [Sources: PR #2](https://github.com/Adzffs/GameEngine/pull/2), [workflow run](https://github.com/Adzffs/GameEngine/actions/runs/44018694678).

Recovering a reproducible baseline is the first engineering milestone. Record the test inventory discovered by the actual configured build, the execution results and any platform limitation.

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

Preserve the exit status when piping output to a log. The existing CI uses Bash's `PIPESTATUS[0]` after `tee` so logging does not hide a failing `ctest` command. [Source: CI workflow](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/.github/workflows/ci.yml).

## 4. Understanding the current CI workflow

The workflow has Linux headless and Windows UCRT64 client jobs. Windows runs the three test labels separately with `continue-on-error` so it can collect results from each group. An always-run enforcement step then fails the job if any group's outcome is not success. The presence of `continue-on-error` is therefore not evidence that failing tests are ignored. [Source: CI workflow](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/.github/workflows/ci.yml).

When diagnosing a failed run, distinguish runner/setup failure, configure failure, compilation failure, test assertion failure, timeout and missing/skipped execution. If logs are unavailable, record that limit rather than assigning a cause. Workflow configuration, branch rules, commit statuses and Actions check results are separate evidence sources.

## 5. Studio foundation coverage to add

The following coverage is planned and should be introduced with the corresponding implementation, not claimed as present today.

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

## 6. Semantic baseline comparisons

Before enabling the importer, capture the existing world using deterministic seeds and explicit observation stages: immediately after construction, after player registration, after relevant actions/ticks, and after save/reload. Delayed NPC/monster creation makes a single startup snapshot insufficient.

Compare terrain, effective collision, durable placement identity, definition references, spawn population/lifecycle, important interactions and persistent state. Preserve creation order during initial migration where compatibility requires it. Keep temporary runtime IDs as a separate observation when relevant; do not define the entire golden master as “all integers match forever.”

The expected baseline must come from independently inspected existing behaviour. A test that computes both expected and actual values using the new adapter can reproduce the same bug twice. Updating a golden snapshot requires explaining the intended change, not merely accepting current output.

## 7. Reachability and gameplay checks

Use authoritative movement and interaction semantics. A target may be close in straight-line distance yet unreachable from the correct side. Validate legal approach tiles, diagonal corner restrictions, occupied footprints and any interaction-specific side/range rules. Flood-fill connectivity can help locate a problem but must not override the actual game's path/interaction result.

Test required paths from spawn to guide, bank, relevant workstation, quest target and region exits. Optional scenic space need not be reachable unless gameplay requires it. Add fixtures for a blocked spawn, a blocked saved position, an obstructed recovery pile and an NPC whose visible location is reachable but legal interaction approach is not.

Run an end-to-end scenario that gathers, processes or crafts, interacts with a named NPC, completes controlled combat/recovery and saves/reloads. Use a deliberately unboosted profile for starter pacing; current development XP settings are unsuitable evidence for a new player's first upgrade time. [Source: DevelopmentConfig.h](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Core/DevelopmentConfig.h).

## 8. Visual and performance verification

Test tile picking across camera pan, zoom, viewport edges and out-of-bounds pointer positions. A preview should record content hash, renderer/tool version and overlays. Pixel snapshots require a pinned rendering environment or documented tolerance; cross-backend pixel equality is not implied by deterministic content.

Measure parse, validation, world construction, pathfinding, tick/frame work, preview and asset memory separately. Start with the actual 100×100 region and measured larger fixtures when the feature requires them. Set budgets after recording hardware/build conditions. Do not claim an improvement from a single noisy run or introduce chunking without a demonstrated bottleneck.

## 9. Evidence record and stopping rule

Every test report should record commit, content hash where relevant, platform/tool versions, configuration, commands, discovered/executed counts, outcomes, logs and unresolved limitations. Use synthetic saves and scrub personal paths/data from public attachments.

For a small change, run the affected meaningful tests plus required project gates. Broaden testing when a concrete risk remains or a release gate requires it. Documentation fixes need structural/link/factual checks, not an invented game test result. Once the scoped change is adequately verified, update the evidence and continue delivery.

# Full project and roadmap review

**Date:** 20 November 2026.\
**Requested outcome:** Review the project before audio, repair critical verified defects, and record the remaining findings in the roadmap.\
**GitHub review baseline:** [`ffd1899`](https://github.com/Adzffs/GameEngine/commit/ffd1899f9a57800153c4e94430c282339e582477).\
**Published critical fixes:** [`38552c2`](https://github.com/Adzffs/GameEngine/commit/38552c255649c61060bf42931f17f3e524229d4b).\
**Local animation baseline:** `688221b`; same safety changes applied at local `196a279`.

## Overall assessment

The project has a useful gameplay foundation and a working bounded editor. The largest immediate risks were losing player state and confusing separate source versions or historical evidence with current acceptance. Three data-integrity defects were reproduced and fixed. The roadmap also needed factual reconciliation: working editor/art features were still described as unavailable, completed bounded work remained awaiting review, and several tests still expected behavior from before later features were introduced.

The next useful step is to reconcile the working builds, then observe the complete fresh-player loop. Sound remains a reasonable subsequent presentation slice. More animation polish is explicitly deferred; the owner has not accepted its current visual quality.

## Review method and coverage

The review compared the recursive GitHub tree with the local animation checkout, examined the code at gameplay/persistence/editor boundaries, read the roadmap and its Markdown/JSON backlog, checked current-state and specialist documentation, built both client and headless configurations, ran the full relevant test sets, and replayed the actual economy/starter commands.

The baseline tree contained 745 files, including 105 C++ implementation files, 203 headers and 106 files under tests. Inventory counts identify the review surface; they are not a claim that every function received exhaustive formal analysis. Source, content, tools and runtime textures were materialized from verified GitHub bytes in an isolated review checkout. Unneeded historical screenshots/references were checked against the GitHub tree rather than downloaded; documentation validation reports that limit.

| Area reviewed | Evidence examined | Outcome |
|---|---|---|
| Timing and movement | Clock, pathfinding, movement authority, navigation tests | No new blocking defect identified in the exercised scenarios |
| Inventory/equipment/bank | Direct item addition, transaction helpers, stack operations, command/session validation | Overflow and unknown-item admission corrected; equipment/session regressions pass |
| Gathering/crafting | Attempt/reward ownership, crafting cursor/grid conservation, station commands and save boundary | Crafting escrow persistence corrected; committed rewards remain authoritative |
| Combat/AI/loot/recovery | Combat results, targeting/leash/respawn, death/recovery and feedback tests | Relevant tests pass; full visible encounter/food/recovery journey remains open |
| Quests/dialogue/progression | Definition consistency, event progress, reward claims and starter guidance | Stale dialogue test corrected; narrative/quest suites pass; human comprehension unmeasured |
| Persistence | Versions 1-13, bounded file loading, sidecars, world snapshots, lifecycle and shutdown | Failed-load artifact destruction and transient-item omission corrected |
| Content/editor | Schemas 1-4, geography/approaches, shared edits/history, draft publication and Python bridge | Current bounded features verified; crash recovery/in-place publication remain open |
| Presentation/assets | Current batches, PNG/binding packaging, animation records, local/upstream difference | Animation integration and quality gaps recorded separately; audio remains planned |
| Build/tests | Windows headless/client builds, real bridge and Python unit suite | Current matrix passes after stale expectations and interpreter discovery were addressed |
| Planning/docs | Active guides, old baseline inventories, roadmap, 87 original task records and link checker | Current entry points and task records reconciled; historical reports retained |

## Findings and dispositions

| Finding | Priority | Disposition | Roadmap owner |
|---|---|---|---|
| REVIEW-001: Failed save loading deleted recovery files before validation | P0 | Fixed and regression verified | ENG-003 |
| REVIEW-002: Saving omitted crafting cursor/grid items | P0 | Fixed; full snapshots reject safely when items cannot fit | GAME-009 |
| REVIEW-003: Direct stack addition and bank withdrawal could overflow | P0 | Fixed; unknown direct additions also reject | ENG-004 |
| REVIEW-004: Local melee/polish and published Mining code diverge | P1 | Explicit integration task; neither version overwritten | REL-004 |
| REVIEW-005: Roadmap and JSON/status notes disagree with delivered work | P1 | Current records reconciled; old chronology labeled | DOC-003 / QA-004 |
| REVIEW-006: Root/current-state/testing pages still imply the old baseline is current | P1 | New current status and corrected entry points | DOC-003 |
| REVIEW-007: Stale tests and an inaccessible Python shortcut hid current editor coverage | P1 | Expectations corrected; explicit interpreter used; actual matrix recorded | QA-004 / QA-001 |
| REVIEW-008: Fresh-player replay has not established human pacing or combat pressure | P1 | Keep starter acceptance open; use current 454-tick measurement honestly | GAME-004 / GAME-005 / GAME-006 |
| REVIEW-009: Saves rely on controlled shutdown; no periodic autosave | P1 | Add an explicit scoped autosave task | GAME-010 |
| REVIEW-010: Interrupted editor recovery and validated publication remain unfinished | P1 | Preserve existing tasks, including stale lock/draft handling | STUDIO-025 / STUDIO-026 |
| REVIEW-011: Several build targets copy the same runtime textures concurrently | P1 | Track packaging ownership and parallel-build verification | STUDIO-063 |
| REVIEW-012: Health/XP additions still need integer-limit policy | P2 | Track boundary hardening separately | ENG-005 |
| REVIEW-013: World and Graphics concentrate many unrelated responsibilities | P2 | Extract only when a concrete change benefits, preserving tests | ENG-006 |
| REVIEW-014: Animation quality is a first pass and not owner-accepted | P2 | Deferred explicitly; preserve sources/research and reopen later | ART-005 |
| REVIEW-015: Remaining art, audio rights/settings and accessibility acceptance are incomplete | P1/P2 | Keep scoped art/audio tasks open; bring usability checks into R4 | ART-002 / ART-004 / QA-003 |
| REVIEW-016: Playtest dependencies required acceptance that playtests must supply | P1 | Corrected measurement-task dependencies; regression/docs checks start before R6 | GAME-005 / QA-001 / QA-003 |

Planning priorities are not a security severity score. Fixed code does not imply that the entire milestone, all platforms or visible acceptance are complete.

## Critical fix details

### REVIEW-001 - Failed loads must preserve recovery evidence

`PlayerSaveFileStore::Reconcile` previously discarded `.tmp` and `.bak` as soon as the primary path existed. The decoder could then reject a damaged primary after the useful backup had already been deleted. The prior test explicitly expected that destructive policy.

The corrected load path waits until the bounded read and decoder succeed before cleaning stale sidecars. Invalid primary content still produces a failure rather than silently selecting another save, while its primary, backup and temporary files remain byte-identical for recovery. Valid-primary precedence, missing-primary recovery and cleanup warnings remain covered.

Reproduction covered malformed, truncated and oversized primaries beside valid artifacts. The new expectations failed **7 of 110 assertions** before the fix. All **110 assertions** now pass.

Sources: [file store](../../../src/Persistence/PlayerSaveFileStore.cpp), [regression](../../../tests/PlayerSaveFileStoreTests.cpp). No save schema change.

### REVIEW-002 - Crafting ownership must cross the save boundary

A live crafting session owns its cursor and grid items outside `Player::Inventory`. `World::SavePlayerToFile` previously captured only the player inventory; a successful save/reload could therefore lose items while the crafting panel remained open.

The save now projects the session's items into a temporary inventory used only for the persisted snapshot. It neither consumes nor duplicates live session ownership. Repeated saves produce the same owned quantities; a reload receives the items in inventory rather than restoring a station-bound session.

If the combined items cannot fit, the save returns the structured `CRAFTING_ITEMS_NOT_SAVED` issue before changing the existing save. The game now handles failed shutdown saving by offering **Return to game** or an explicit **Exit without saving**, with return as the default. It no longer silently ends the process after reporting a failed save to the log.

The actual World/session round trip reproduced the item omission before the fix. Regression coverage now checks cursor + grid + inventory quantities, live nonmutation, repeated saves and capacity-failure preservation. `WorldPlayerPersistenceTests` passes **152 assertions**. Existing engine lifecycle tests also verify that failed finalization remains retryable.

Sources: [world snapshot](../../../src/World/World.cpp), [structured issue](../../../src/Persistence/WorldPlayerPersistenceResult.h), [engine exit handling](../../../src/Core/Engine.cpp), [world tests](../../../tests/WorldPlayerPersistenceTests.cpp), [engine lifecycle tests](../../../tests/EnginePlayerPersistenceLifecycleTests.cpp). Save version remains **13**. The native dialog was compiled; its visible button interaction was not manually exercised.

### REVIEW-003 - Capacity queries and mutations must agree

`Inventory::CanAddItem` returned true for an existing stack without checking its remaining integer capacity. `AddItem` then incremented it unchecked. `Bank::TryWithdraw` trusted those results, so an overflowing coin withdrawal could corrupt inventory and debit the bank.

Both query and mutation now reject amounts exceeding `INT_MAX` before changing state. Unknown item IDs also reject instead of using the fallback definition. Exact-capacity additions still work. Overflowing withdrawals preserve both bank and inventory.

The boundary checks failed **4 of 101 inventory assertions** before the fix. All **101 assertions** now pass, alongside the existing atomic transaction and stack-operation suites.

Sources: [inventory](../../../src/Inventory/Inventory.cpp), [bank](../../../src/Bank/Bank.h), [regression](../../../tests/InventoryTransactionTests.cpp). Stack limits and the save format are unchanged.

## Source integration finding

At the start of review, GitHub had the Mining slice and shared gathering presentation, while the local animation branch had melee, visible equipment and newer posing/timing helpers. The local checkout also lacked parts of the published editor/documentation snapshot. Its synthetic snapshot history was not suitable for a force push over `master`.

The safety fixes were applied independently to the published source and local animation line. The original animation work is preserved, and both code variants were tested. The review did not present the local prototype as an integrated published feature.

REL-004 must preserve the shared Mining/Woodcutting state, reconcile Graphics/atlas bindings, build scripts and preview fixtures, publish editable sources and evidence, and run the combined tests. Only then can its local/published status converge. This is the first remaining implementation task before audio.

## Planning and documentation corrections

- Replace old 'what next' instructions about first implementing move/undo/editor basics with the actual remaining integration and starter acceptance work.
- Add a [current status page](../01-current-state/CURRENT_STATUS.md); keep the old source inventory as a dated baseline.
- Synchronize Markdown and JSON task fields, notes, evidence and dependencies. Keep stable IDs and distinguish implemented scope from broader acceptance.
- Remove obsolete 'unbuilt/Windows unverified' notes where current executed evidence supersedes them. Preserve the older dated records rather than rewriting their original results.
- Keep the current 1,175-frame/30-texture published art count separate from the local 2,871-frame prototype count.
- Record animation quality as deferred by the owner; an automated test pass does not approve its appearance.
- Make ongoing regression/documentation checks available before R6 packaging; accessibility/usable feedback belongs in the starter slice.
- Keep autosave, editor interruption recovery, asset copy ownership and integer-boundary hardening as explicit tasks with observable acceptance.

## Executed verification

| Check | Result | Qualification |
|---|---:|---|
| Full Windows SDL build and CTest | 102/102 pass | 87 headless + 5 client + 10 graphics entries; dummy/software renderer |
| Separate Windows headless configure/build/CTest | 87/87 pass | Client/graphics disabled; no SDL dependency required for the changed core |
| Python editor unit suite | 42/42 pass | Python 3.14.6 selected explicitly |
| Real editor/CLI bridge | 12/12 cases pass | Included in one CTest entry above |
| Local animation line after applying the fixes | 101/101 pass | A separate snapshot; its older configuration omits the Python bridge entry |
| Focused persistence/inventory checks | 110 / 101 / 152 assertions pass | Save-file store / inventory transactions / World persistence |
| Current timber production replay | 60 -> 54 -> 48 coins | Two complete cycles, net -6 each; no outstanding confirmed timber arbitrage |
| Current fresh starter replay, run twice | Shield 245; sword 376; prepared return 454 ticks | Zero starting coins/material/XP injection; 20 quest coins, 16 food cost, 4 remaining |
| Documentation consistency | Checked by repository script | Final totals are stored with the review evidence; baseline-only historic media are explicitly reported |

The initial local animation build passed 99/101 entries; actor-movement and dialogue wording checks were stale. Enabling the actual Python interpreter exposed stale command-whitelist and fresh-profile expectations in editor tests. Those checks now validate the current supported behavior, while retaining rejection, no-write and isolated-save assertions.

Toolchain: **Windows, UCRT64 GCC 16.1.0, CMake 4.4.3, Ninja 1.13.2, SDL 3.4.12, Python 3.14.6**. Windows Store execution aliases were inaccessible in this session; the installed interpreter was used directly. A missing Python integration entry must not be described as a full editor pass.

The older 451-tick starter route remains historical evidence for its source revision. This review observed **454 ticks twice** on the current source, while shield/sword acquisition stayed at 245/376. Neither value measures the player's reading, navigation or decision time; the 10-15 minute design goal remains unverified.

See [machine-readable results](review/2026-11-20/results.json), [current timber replay](review/2026-11-20/timber-replay.json) and [current starter replay](review/2026-11-20/starter-replay.json).

### Reproduction

From the repository root, substitute a working installed Python executable for the example path:

```powershell
cmake -S . -B build-review -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DGAMEENGINE_BUILD_CLIENT=ON -DGAMEENGINE_BUILD_GRAPHICS_TESTS=ON -DPython3_EXECUTABLE=C:/Path/To/Python/python.exe
cmake --build build-review --parallel 6
$env:SDL_VIDEODRIVER = "dummy"
$env:SDL_AUDIODRIVER = "dummy"
ctest --test-dir build-review --output-on-failure --parallel 4

cmake -S . -B build-review-headless -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DGAMEENGINE_BUILD_CLIENT=OFF -DGAMEENGINE_BUILD_GRAPHICS_TESTS=OFF -DPython3_EXECUTABLE=C:/Path/To/Python/python.exe
cmake --build build-review-headless --parallel 6
ctest --test-dir build-review-headless --output-on-failure --parallel 4

python -m unittest discover -s tests -p "test_*.py"
python tools/check_documentation.py
.\build-review\EmbervaleEconomyReport.exe --replay-timber-cycle 60 2 --world content/worlds/river-crossing.blockout.json
.\build-review\EmbervaleEconomyReport.exe --replay-starter-upgrade --world content/worlds/river-crossing.blockout.json
```

The documentation checker can use `--baseline-tree <recursive-tree.json>` for an explicitly partial staging checkout. The executed staging check used that option for historical media. A normal complete checkout can omit it.

## What remains open

The source differences, real starter pacing, visible save-failure choices, full quest/combat/recovery experience, remaining artwork, general importing, periodic autosave, interrupted editor recovery, publication manifests and accessibility still need their own acceptance. This review did not run a visible GPU playthrough, exercise the new native dialog by hand, retest Linux after the fixes, or establish online service readiness.

Keep the scope of the existing flat game. Elevation, multiplayer, companions, bosses and specialized authoring workspaces remain separately planned investments. None is a prerequisite for fixing present player-facing problems.

## Recommended next sequence

1. Finish REL-004: reconcile and publish the working melee/equipment version with the current Mining code.
2. Run GAME-005 on the exact integrated build: fresh profile, gathering/crafting, bank/shop, starter combat, death/recovery and save/reload. Record confusion and actual elapsed upgrade time.
3. Resolve any blocking journey failures, then implement ART-004's small sound pass with event identity, volume/mute, repetition limits and provenance.
4. Schedule the remaining P1 recovery/packaging/usability tasks. Return to ART-005 when the owner is ready to assess animation quality again.

The [roadmap](../02-roadmap/MASTER_ROADMAP.md) and [backlog](../02-roadmap/BACKLOG.md) contain the maintained work order and acceptance results.

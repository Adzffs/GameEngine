# Current project status

**Reviewed:** 20 November 2026.\
**Published code:** [`38552c2`](https://github.com/Adzffs/GameEngine/commit/38552c255649c61060bf42931f17f3e524229d4b).\
**Source of truth:** Current code and the [project review](../06-development/PROJECT_REVIEW_2026-11-20.md). The older [engine inventory](IMPLEMENTED_SYSTEMS.md) retains its explicitly dated baseline.

Embervale is a playable local C++20/SDL3 RPG prototype with a bounded world editor and an authored River Crossing starter region. It has substantial gameplay systems. It is not yet a completed starter vertical slice, a released Studio product or an implemented online MMO.

## What is implemented

| Area | Current bounded implementation | Remaining acceptance or scope |
|---|---|---|
| Simulation | Authoritative world updates, 600-ms ticks, catch-up limits, separate render timing and deterministic gameplay RNG streams | Multiplayer command/identity/transport work remains a later milestone |
| Movement | Eight-direction pathfinding, corner safety, queued destinations, walking/running and camera presentation | Uncoached route quality and map pacing |
| Gameplay | Gathering, processing/crafting, inventory/equipment, shops, banking, melee, monster AI/leash/respawn, loot, food and recovery | Full starter journey, combat/food pressure and economy balance acceptance |
| Progression | Five persisted skills, six starter quests, tutorial guidance, dialogue choices and recipe discovery | Observe first-time players completing the whole chain without coaching |
| Persistence | Save version 13, historical migration, bounded loading, transactional file handling and controlled shutdown saving | Periodic autosave and stronger interruption/durability policy |
| Data-safety corrections | Failed loads preserve backups; saves include crafting escrow; overflowing inventory additions reject; failed shutdown saves offer return to game or explicit exit without saving | Native save-failure dialog interaction still needs visible Windows acceptance |
| Authored world | Flat 100x100 River Crossing candidate, distributed services, existing actor homes and static workshop buildings | Visible encounter/escape/recovery and human pacing acceptance; more building assemblies |
| World format | Schemas 1-4, bounded parser/runtime validation, stable placements, retained allocation IDs and staged spawn semantics | General region/definition authoring and versioned publication manifests |
| Studio | Shared move/paint/place/remove/spawn-setting commands, history, guarded new sibling drafts, dirty-state protection, reopen, isolated playtests and navigation/validation overlays | Interruption recovery, in-place publication, richer diagnostics, brushes and accessibility acceptance |
| Published art | Eight bounded batches, 1,175 frames in 30 textures, eight-way walking/chopping/mining, editable masters, cache and fallback | Remaining cast/resource tiers, equipment/melee integration, general importer, UI/accessibility and visible quality acceptance |
| Audio | Planned; no completed gameplay sound pass is claimed | Event routing, restrained assets, rights, volume/mute, repetition and missing-asset behavior |

## Published and local animation work

GitHub contains the Mining integration and shared `GatheringPresentationState`. The separate local animation line contains melee, visible equipment and a first polish pass. It started this review at `688221b` and now also contains the critical safety fixes at local `196a279`. The locally tested animation line still lacks GitHub's newer Mining/editor/documentation work.

These histories must be reconciled without replacing one snapshot with the other. **REL-004** owns that integration, publication of editable sources/previews and combined verification. The local animation branch's 101 passing CTest entries are a separate result from the published source's 102 entries.

The owner is not satisfied with the animation quality and explicitly deferred further polish. Treat it as a working first pass, with **ART-005** still planned and visual acceptance outstanding. See the [animation guide](../05-game/ANIMATION_GUIDE.md) and [dated research](../06-development/ANIMATION_RESEARCH_2026-11-20.md).

**Integration update:** REL-004 has started in the separate local GameEngine-next worktree. The Windows execution environment became unavailable before its combined build/export results could be retrieved. The implementation remains unpublished and unverified; the baseline results below have not changed. See the [integration handoff](../06-development/INTEGRATION_HANDOFF_2026-11-20.md) for recovery details, Mining/woodcutting compatibility findings and pending starter/audio checks.

## Executed checks

| Source/configuration | Executed result | Scope |
|---|---:|---|
| Published source plus critical fixes, Windows SDL client configuration | 102/102 CTest entries | Includes 87 headless, 5 client and 10 graphics labels; SDL dummy/software presentation |
| Same source, separate Windows build with client and graphics disabled | 87/87 CTest entries | Confirms the changed core builds without SDL |
| Python editor unit tests | 42/42 tests | Run directly with Python 3.14.6 |
| Real executable-backed editor bridge | 12/12 cases | Included in the CTest bridge entry above, not 12 additional CTest entries |
| Local animation line after the same safety fixes | 101/101 CTest entries | Different source snapshot; does not establish combined Mining/melee acceptance |

Toolchain: UCRT64 GCC 16.1.0, CMake 4.4.3, Ninja 1.13.2 and SDL 3.4.12. The source blobs published for the fixes were checked against the tested local commit. Earlier Linux results remain historical; this review did not rerun Linux or perform an uncoached human/visible GPU playthrough.

The current production replay equips the shield at tick 245, equips the sword at tick 376 and prepares the return with food at tick 454. These are simulation measurements, not human elapsed time. The older 451-tick record belongs to its earlier source. The timber replay still loses six coins per complete cycle.

## Current work order

1. **Completed in this review:** ENG-003, ENG-004 and GAME-009 protect saves/items; QA-004 reconciles current automated evidence.
2. **REL-004:** combine the published Mining work and local melee/equipment first pass; publish the matching sources, outputs and previews.
3. **GAME-005 / GAME-004:** run the fresh-player route visibly; record navigation, food, death/recovery, confusion and real upgrade time.
4. **ART-004:** add the bounded sound pass with independent effects volume/mute and event deduplication.
5. Continue the separately prioritized recovery, asset packaging, autosave and accessibility tasks. Animation quality remains ART-005, deferred by the owner.

R0-R4 remain partial milestones. Optional editor expansion, elevation, multiple regions, companions, bosses and multiplayer do not need to be completed before the existing flat starter experience improves. Use the [roadmap](../02-roadmap/MASTER_ROADMAP.md) and synchronized [backlog](../02-roadmap/BACKLOG.md) for exact tasks and dependencies.

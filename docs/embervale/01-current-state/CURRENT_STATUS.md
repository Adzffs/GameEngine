# Current project status

**Reviewed:** 20 November 2026.\
**Published code:** [fa5e0e9](https://github.com/Adzffs/GameEngine/commit/fa5e0e93efa71ac60519287cfd9a35b328ec6270).\
**Source of truth:** Current code and the [project review](../06-development/PROJECT_REVIEW_2026-11-20.md). The older [engine inventory](IMPLEMENTED_SYSTEMS.md) retains its explicitly dated baseline.

Embervale is a playable local C++20/SDL3 RPG prototype with a bounded world editor and an authored River Crossing starter region. It has substantial gameplay systems. It is not yet a completed starter vertical slice, a released Studio product or an implemented online MMO.

## What is implemented

| Area | Current bounded implementation | Remaining acceptance or scope |
|---|---|---|
| Simulation | Authoritative world updates, 600-ms ticks, catch-up limits, separate render timing and deterministic gameplay RNG streams | Multiplayer command/identity/transport work remains a later milestone |
| Movement | Eight-direction pathfinding, corner safety, queued destinations, walking/running and camera presentation | Uncoached route quality and map pacing |
| Gameplay | Gathering, processing/crafting, inventory/equipment, shops, banking, melee, monster AI/leash/respawn, loot, food and recovery | Visible/full story journey, combat/food feel and economy balance acceptance |
| Progression | Five persisted skills, six starter quests, tutorial guidance, dialogue choices and recipe discovery | Observe first-time players completing the whole chain without coaching |
| Persistence | Save version 13, historical migration, bounded loading, transactional replacement, 60-second autosave and controlled shutdown | Windows/human notice review and stronger physical-media durability policy |
| Data-safety corrections | Failed loads preserve backups; saves include crafting escrow; overflowing inventory additions reject; failed shutdown saves offer return to game or explicit exit without saving | Native save-failure dialog interaction still needs visible Windows acceptance |
| Authored world | Flat 100x100 River Crossing candidate, distributed services, existing actor homes and static workshop buildings | Visible encounter/escape/recovery and human pacing acceptance; more building assemblies |
| World format | Schemas 1-4, bounded parser/runtime validation, stable placements, retained allocation IDs and staged spawn semantics | General region/definition authoring and versioned publication manifests |
| Studio | Shared move/paint/place/remove/spawn-setting commands, history, guarded new sibling drafts, dirty-state protection, reopen, isolated playtests and navigation/validation overlays | Interruption recovery, in-place publication, richer diagnostics, brushes and accessibility acceptance |
| Published art | Eight bounded batches, 1,175 frames in 30 textures, eight-way walking/chopping/mining, editable masters, cache and fallback | Remaining cast/resource tiers, equipment/melee integration, general importer, UI/accessibility and visible quality acceptance |
| Audio | First sound pass: 15 original effects, committed-result routing, bounded playback, effects volume/mute and keyboard controls | Audible/device/Windows review, original melee integration and full starter acceptance |

## Published and local animation work

GitHub contains the Mining integration and shared `GatheringPresentationState`. The separate local animation line contains melee, visible equipment and a first polish pass. It started this review at `688221b` and now also contains the critical safety fixes at local `196a279`. The locally tested animation line still lacks GitHub's newer Mining/editor/documentation work.

These histories must be reconciled without replacing one snapshot with the other. **REL-004** owns that integration, publication of editable sources/previews and combined verification. The local animation branch's 101 passing CTest entries are a separate result from the published source's 102 entries.

The owner is not satisfied with the animation quality and explicitly deferred further polish. Treat it as a working first pass, with **ART-005** still planned and visual acceptance outstanding. See the [animation guide](../05-game/ANIMATION_GUIDE.md) and [dated research](../06-development/ANIMATION_RESEARCH_2026-11-20.md).

**Integration update:** The original GameEngine-next worktree remains unavailable. Its melee/polish merge is unpublished and unverified. An independent Linux follow-up restored the current published source, fixed shared runtime copying and delivered the first sound pass. See the [action-audio results](../06-development/ACTION_AUDIO_2026-11-20.md) and retain the [original integration handoff](../06-development/INTEGRATION_HANDOFF_2026-11-20.md) for recovery. ART-004 is in progress; REL-004, full starter and owner visual acceptance remain open.

The [starter combat/recovery follow-up](../06-development/STARTER_COMBAT_RECOVERY_2026-11-20.md) completes the automated tutorial and deliberate recovery exercise. It fixes skipped post-combat bank/Mara steps and misleading loot/reward hints. Six save/reload checkpoints and single persisted recovery pass; full human/visible acceptance remains open.

The [autosave policy and verification](../06-development/PLAYER_AUTOSAVE_2026-11-20.md) completes GAME-010: persistent players checkpoint after committed ticks every 60 seconds, retry failures after 10 seconds and defer due saves through respawn. Crafting ownership and temporary-play isolation remain intact; a killed process reloads its completed checkpoint. This does not guarantee power-loss durability.

## Executed checks

| Source/configuration | Executed result | Scope |
|---|---:|---|
| Current autosave source, Linux SDL configuration | 112/112 CTest entries | 93 headless, 7 client, 12 graphics; includes killed-process reload and status rendering |
| Same current source, Linux with client/graphics disabled | 93/93 CTest entries | SDL-free core, starter replay and periodic-save/interruption checks |
| Current Python editor unit suite | 42/42 tests | Python 3.12.14; real bridge still 12 cases within one CTest entry |
| Earlier published source plus critical fixes, Windows SDL client configuration | 102/102 CTest entries | Includes 87 headless, 5 client and 10 graphics labels; SDL dummy/software presentation |
| Same source, separate Windows build with client and graphics disabled | 87/87 CTest entries | Confirms the changed core builds without SDL |
| Python editor unit tests | 42/42 tests | Run directly with Python 3.14.6 |
| Real executable-backed editor bridge | 12/12 cases | Included in the CTest bridge entry above, not 12 additional CTest entries |
| Local animation line after the same safety fixes | 101/101 CTest entries | Different source snapshot; does not establish combined Mining/melee acceptance |

The current follow-up uses Linux GCC 14.2.0, CMake 4.4.5, Ninja 1.13.2, audio-enabled SDL 3.4.12 and Python 3.12.14. Its published source blobs match the tested files. Earlier Windows review results use UCRT64 GCC 16.1.0/CMake 4.4.3 and their own recorded source. The new sound pass has not been accepted on a physical device or Windows, and no uncoached human/full visible starter playthrough is claimed.

The unchanged preparation replay equips the shield at tick 245, equips the sword at 376 and buys food by 454. The extended route defeats Mossling at 677 and completes the tutorial by returning to Mara at 741. A separate intentional death/recovery segment ends at cumulative tick 2370 across two primary sessions, with six save/reload checks. These simulation measurements do not establish human elapsed time; the older 451-tick preparation belongs to its historical source. The timber replay still loses six coins per complete cycle.

## Current work order

1. **Completed in this review:** ENG-003, ENG-004 and GAME-009 protect saves/items; QA-004 reconciles current automated evidence; GAME-010 now adds bounded periodic saves.
2. **REL-004:** combine the published Mining work and local melee/equipment first pass; publish the matching sources, outputs and previews.
3. **GAME-005 / GAME-004:** automated combat, food, death/recovery and save/reload now pass; finish the visible/uncoached route, input/confusion, balance and real elapsed-time acceptance.
4. **ART-004:** the bounded sound pass is implemented against published source; finish original animation synchronization and audible/device/Windows acceptance after recovery.
5. Continue the separately prioritized editor recovery, asset packaging and accessibility tasks. GAME-010 autosave is complete for its stated scope. Animation quality remains ART-005, deferred by the owner.

R0-R4 remain partial milestones. Optional editor expansion, elevation, multiple regions, companions, bosses and multiplayer do not need to be completed before the existing flat starter experience improves. Use the [roadmap](../02-roadmap/MASTER_ROADMAP.md) and synchronized [backlog](../02-roadmap/BACKLOG.md) for exact tasks and dependencies.

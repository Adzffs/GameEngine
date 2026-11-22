# Current project status

**Reviewed:** 21 November 2026.\
**Published code:** [d40b491](https://github.com/Adzffs/GameEngine/commit/d40b491d8b8082edc65ec4be0efc26ed882f2c54).\
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
| World format | Schemas 1-4, bounded runtime validation, stable placements/allocation IDs, staged spawns and fixed version manifests | General region/definition authoring, content/save migrations and asset packages |
| Studio | Shared edits/history, guarded drafts, private recovery, reopen, overlays, exact-byte validation, create-only version publishing and selected-version play/rollback | In-place replacement, richer diagnostics, brushes and broader accessibility/platform acceptance |
| Art candidate | Ten batches, 3,135 frames/33 textures, eight-way walking/chopping/mining/melee and visible equipment, editable masters, guarded cache/fallback, bounded PNG streams and explicit resource budgets | Remaining cast/resource tiers, general importer, UI/accessibility and visible quality acceptance |
| Audio | First sound pass: 15 original effects, committed-result routing, bounded playback, effects volume/mute and keyboard controls | Physical-device audible quality/balance and full starter acceptance |

## Modular equipment authoring proof

The [east-facing axe prototype](../06-development/MODULAR_AXE_PROTOTYPE_2026-11-22.md) now assembles one existing body sequence and explicit hand track with a separate axe master, semantic metal mask and bronze/iron/steel mappings. The owner rejected the initial masking-only attempt to fix handle penetration. The [documented pose revision](../06-development/MODULAR_AXE_OCCLUSION_FIX_2026-11-22.md) now reposes both arms and hand/aim tracks so the shaft lifts beside the face and remains forward of the torso. Twelve focused tests pass, including actual LibreSprite export, unmasked clearance in all 24 cells and refusal of the saved rejected pose. The pilot head/feet and axe/mask sources are preserved; its arm source intentionally changes. The pilot is outside runtime packaging: all 33 existing runtime masters and textures remain exact, and the live catalogue stays at 3,135 frames. The existing body is an engineering fixture; clean motion quality, all eight directions and actual-game migration remain open.

## Integrated player actions

The [player action integration](../06-development/PLAYER_ACTION_INTEGRATION_2026-11-20.md) completes REL-004. The original Windows work was recovered from its verified checkpoint and reconciled against current master without replacing the published snapshot history. Mining, weighted chopping/walking, visible weapons/shields, melee and current audio/editor/safety changes now share one verified build.

Normal exports preserve all 32 editable masters. Lossless runtime packing fits 3,063 frames in 32 textures within the unchanged resource limits. Committed melee snapshots survive death cleanup and catch-up ticks; swing sound follows the visible wind-up once. All 47 packaged images/sounds match their sources.

ART-005 has resumed with [PixelLab eight-way empty-handed walking](../06-development/PIXELLAB_WALKING_2026-11-21.md). The owner liked the east-only trial; the complete in-game set remains a review candidate. Equipped walking, Mining, Woodcutting and melee retain their existing art until matching AI passes are ready. The new master is separate, taking the candidate to 33 masters/textures, 3,135 bound frames and 48 packaged images/sounds. ART-004 remains in progress for physical-device audible quality/balance. GAME-005 retains the uncoached starter journey and actual elapsed-time acceptance.

The [equipped-walking handoff](../06-development/PIXELLAB_EQUIPPED_HANDOFF_2026-11-21.md) records a subsequent local sword-and-shield east-facing pilot. A diagonal handedness defect, insufficient generation credits and a disconnected local session block its correction and final publication. The owner intends to add credits. Its raw art, editable master and authoring preview are retained locally; this documentation checkpoint adds no runtime art or final verification claim. Recover the recorded files before generating replacements.

The [starter combat/recovery follow-up](../06-development/STARTER_COMBAT_RECOVERY_2026-11-20.md) completes the automated tutorial and deliberate recovery exercise. It fixes skipped post-combat bank/Mara steps and misleading loot/reward hints. Six save/reload checkpoints and single persisted recovery pass; full human/visible acceptance remains open.

The [autosave policy and verification](../06-development/PLAYER_AUTOSAVE_2026-11-20.md) completes GAME-010: persistent players checkpoint after committed ticks every 60 seconds, retry failures after 10 seconds and defer due saves through respawn. Crafting ownership and temporary-play isolation remain intact; a killed process reloads its completed checkpoint. This does not guarantee power-loss durability.

The [editor recovery follow-up](../06-development/EDITOR_RECOVERY_2026-11-20.md) completes STUDIO-025: private accepted-edit journals, separate validated restore, active-owner exclusion, safe legacy-lock handling and staged world writes. Forced termination at journal/C++ write boundaries passes. One real Linux/Tk restore through a changed-original warning and new-file picker also passed; broader Windows/accessibility acceptance remains open.

The [publication verification](../06-development/WORLD_PUBLICATION_2026-11-20.md) completes bounded STUDIO-026: exact-source/validator receipts, canonical create-only versions, manifest-last commit, current-runtime revalidation and explicit fresh-session selection/rollback. Studio validates, publishes and lists earlier versions. Linux process-kill, stale-content, tamper and v1/v2/v1 game-start tests pass. Native Tk publishing/selection passed; its game launch reports the missing desktop SDL driver. Dummy-SDL game launches pass. See [fixed world publication](../04-studio/WORLD_PUBLICATION.md).

The [asset validation results](../06-development/ASSET_RESOURCE_VALIDATION_2026-11-20.md) completes bounded STUDIO-063: shared numeric budgets, frame/binding validation, CRC and bounded PNG inflation before SDL decode, owned decoder bytes, cache accounting and read-only pre-copy validation. That original eight-batch run measured 11,681,792 logical RGBA bytes plus 2,920,448 transparency bytes; the current combined set uses 23,162,880 plus 5,790,720 bytes, as recorded in the player action integration. Private corrupt-source, preserved-output, no-relink repair and spaced multi-config Release checks pass. Art/animation sources and generated geometry are unchanged. See [asset resource limits](../05-game/ASSET_RESOURCE_LIMITS.md); core builds now require zlib development headers/library.

## Usability verification

QA-003 remains **in progress** for broader acceptance. The [Windows verification](../06-development/WINDOWS_USABILITY_2026-11-20.md) completes the recovered candidate's build/full regression checks and fixes queued station closure, bank transfer replacement, CRLF art bindings and Windows path/test portability. Dialogue keyboard/acknowledgement, help/Escape priority, focus-loss cancellation and readable save recovery are implemented. Bank deposit/withdraw is verified through real SDL, Engine and World paths. Wider keyboard operation, scalable UI text, ordinary focus/high-DPI, manual native Windows save choices and an uncoached full starter journey remain open.

## Executed checks

| Source/configuration | Executed result | Scope |
|---|---:|---|
| Integrated player actions, Windows Debug SDL | 124/124 CTest entries | 99 headless, 10 client, 15 graphics; dummy/software/audio; guarded packaging |
| Same integrated source, separate Windows SDL-free | 99/99 CTest entries | No SDL reference in its Ninja graph; full starter/safety/editor coverage |
| Same integrated source, Python 3.14.6 | 81 run: 79 pass, two skips | Symbolic links unavailable; mixed pose-count guards included |
| Earlier Windows usability source, Debug SDL configuration | 122/122 CTest entries | 98 headless, 10 client, 14 graphics; dummy/software/audio and private packaging checks |
| Same earlier usability source, separate Windows SDL-free build | 98/98 CTest entries | Core, replay, saves, recovery, publication and resource checks |
| Earlier usability Python 3.14.6 suite | 80 run: 78 pass, two skips | Symlink creation unavailable; no other skipped checks |
| Asset-validation source, Linux SDL configuration | 120/120 CTest entries | 98 headless, 9 client, 13 graphics; dummy video/software/audio and real private packaging checks |
| Same current source, Linux with client/graphics disabled | 98/98 CTest entries | SDL-free/zlib-linked core, starter replay, autosave, editor recovery, publication and asset checks |
| Asset-validation Python authoring/editor unit suite | 79/79 tests per interpreter | Python 3.12.14 and 3.13.5; includes 15 new resource cases; separate integration suites remain included in CTest totals |
| Earlier published source plus critical fixes, Windows SDL client configuration | 102/102 CTest entries | Includes 87 headless, 5 client and 10 graphics labels; SDL dummy/software presentation |
| Same source, separate Windows build with client and graphics disabled | 87/87 CTest entries | Confirms the changed core builds without SDL |
| Python editor unit tests | 42/42 tests | Run directly with Python 3.14.6 |
| Real executable-backed editor bridge | 12/12 cases | Included in the CTest bridge entry above, not 12 additional CTest entries |
| Local animation line after the same safety fixes | 101/101 CTest entries | Different source snapshot; does not establish combined Mining/melee acceptance |

The current Windows follow-up uses UCRT64 GCC 16.1.0, CMake 4.4.3, Ninja 1.13.2, SDL 3.4.12, zlib 1.3.2 and Python 3.14.6. The preceding Linux asset-validation line used GCC 14.2.0, CMake 4.4.5, SDL 3.4.12, zlib 1.3.1 and Python 3.12.14. Its published source blobs match the tested files. Earlier Windows review results use UCRT64 GCC 16.1.0/CMake 4.4.3 and their own recorded source. The integrated sound plumbing passes Windows automated checks; physical-device audible quality remains unaccepted, and no uncoached human/full visible starter playthrough is claimed.

The unchanged preparation replay equips the shield at tick 245, equips the sword at 376 and buys food by 454. The extended route defeats Mossling at 677 and completes the tutorial by returning to Mara at 741. A separate intentional death/recovery segment ends at cumulative tick 2370 across two primary sessions, with six save/reload checks. These simulation measurements do not establish human elapsed time; the older 451-tick preparation belongs to its historical source. The timber replay still loses six coins per complete cycle.

## Current work order

1. **Completed in this review:** ENG-003, ENG-004 and GAME-009 protect saves/items; QA-004 reconciles current automated evidence; GAME-010 and STUDIO-025 protect player progress/editor work; bounded STUDIO-026 publishes/selects fixed maps and STUDIO-063 now validates current art resources and memory budgets.
2. **Current owner priority, ART-005:** review the PixelLab eight-way walking candidate, then produce matching equipped/action art. REL-004 remains complete for the earlier published combined player-action build.
3. **GAME-005 / GAME-004:** automated combat, food, death/recovery and save/reload now pass; finish the visible/uncoached route, input/confusion, balance and real elapsed-time acceptance.
4. **ART-004:** the bounded sound pass is implemented against published source; finish physical-device audible quality, repetition and balance acceptance; original animation synchronization now passes Windows checks.
5. Continue QA-003 UI readability/keyboard/manual acceptance and REL-002 distribution/installation work after the verified usability fixes. GAME-010, bounded STUDIO-025 recovery, STUDIO-026 fixed publishing and STUDIO-063 current-art resource validation are complete for their stated scopes. Animation quality remains ART-005, now in progress with the PixelLab walking candidate.

R0-R4 remain partial milestones. Optional editor expansion, elevation, multiple regions, companions, bosses and multiplayer do not need to be completed before the existing flat starter experience improves. Use the [roadmap](../02-roadmap/MASTER_ROADMAP.md) and synchronized [backlog](../02-roadmap/BACKLOG.md) for exact tasks and dependencies.

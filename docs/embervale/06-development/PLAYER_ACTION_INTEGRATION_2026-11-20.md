# Recovered player action integration

**Date:** 20 November 2026.\
**Verified implementation:** [d40b491](https://github.com/Adzffs/GameEngine/commit/d40b491d8b8082edc65ec4be0efc26ed882f2c54).\
**Published baseline:** [4686c53](https://github.com/Adzffs/GameEngine/commit/4686c539281ce7e6621c73f2b33d6b5fab712847).\
**Task:** REL-004; related ART-004 synchronization and GAME-005 verification.\
**Status:** The original Windows melee/equipment integration is recovered, reconciled with current Mining/audio/editor code, verified and published. Owner animation polish, audible quality and uncoached starter acceptance remain separate.

## Recovery and scope

The create-only checkpoint described in the [Windows recovery record](WINDOWS_USABILITY_2026-11-20.md) supplied all 79 working paths. Their saved SHA-256 values were checked before reconstruction. The original unresolved checkout, its conflict stages and the unrelated unfinished help/dialogue edits remain untouched.

The recovered delta was applied to current master through a new integration branch. Its synthetic snapshot ancestry was not used to replace master. The resulting GitHub implementation tree is exactly `a6de44c7efe21f96d4c6353be361112ff0162110`, matching the local verified implementation tree. The [source-blob record](player-actions/2026-11-20/results.json) identifies all 656 tested source, tool, test and asset blobs.

This delivers the existing first-pass artwork and timing. It does not claim a new owner-approved polish pass. The [original handoff](INTEGRATION_HANDOFF_2026-11-20.md), [research](ANIMATION_RESEARCH_2026-11-20.md) and older captures remain historical records of their own source revisions.

## Reconciled boundaries

| Boundary | Combined behavior |
|---|---|
| Walking/equipment | Eight distance-driven walking poses in eight directions; equipped sword, godsword, axe/pickaxe tiers and shield use matching attachments |
| Woodcutting | Eight authored weighted poses; contact at 660-740 ms; the existing 1,200-ms cosmetic cycle remains |
| Mining | The original six-pose, 64x80 masters and lower rock strike remain; six 200-ms holds and existing pickaxe orientation are preserved |
| Shared gathering | One eligibility/interruption/reward implementation; skill-specific durations and independent array bounds |
| Melee | Guard, final wind-up, committed contact and weapon-specific recovery for unarmed, sword, heavy and tool profiles |
| Fatal/catch-up results | Engine captures committed result snapshots after every World update; a bounded latest-result slot survives feedback cleanup and the next catch-up tick |
| Swing sound | One consumable edge at visible wind-up; immediate contacts supply a swing if none played. Action creation no longer sounds a premature or duplicate swing |
| Outcome sound | Hit, block and miss remain driven by committed per-tick results; rejected commands remain silent |
| Runtime resources | Both Python and SDL-free C++ binding tables validate the new eight-pose walking/chopping and sixteen equipped-frame tables; Mining stays six |
| Cache/fallback | Reinitialization clears melee bindings; missing required body/weapon/shield layers reject drawing and retain fallback |
| Packaging | New melee images join the existing guarded single copy owner; current sounds and all other batches remain packaged |

No collision, reach, roll, damage, recipe, item/XP grant, quest predicate, save schema or authoritative action interval changes. Player format remains 13 and simulation ticks remain 600 ms. Autosave, crafting ownership, editor recovery/publication and the latest bank/modal fixes remain included.

## Lossless runtime packing

The recovered editable melee canvases were 1536x3840 and 1536x6912. They exceed the newer individual edge/pixel and aggregate budgets. Those limits were retained.

The LibreSprite export now shelf-packs each occupied cell rectangle, including its ground pivot, into runtime sheets of **1024x462** and **1024x949**. Frame rectangles and pivots are generated together. The original 96x96 editable cells and masters are unchanged. After writing each PNG, the exporter reopens it and compares alpha and visible RGBA against all **1,792 original cells / 16,515,072 source pixels**.

| Current combined resource | Result |
|---|---:|
| Batches / runtime PNGs / frames | 9 / 32 / 3,063 |
| Encoded PNGs | 467,679 bytes |
| Logical RGBA8 texture estimate | 23,162,880 bytes |
| Retained alpha cache | 5,790,720 bytes |
| Texture plus alpha | 28,953,600 bytes |
| Largest image staging estimate | 7,112,398 bytes |

These are logical accounting estimates, excluding driver/allocator/decoder overhead. See the [complete memory report](player-actions/2026-11-20/memory-report.json) and unchanged [resource policy](../05-game/ASSET_RESOURCE_LIMITS.md).

Normal cast, woodcutting, Mining and melee exports were run sequentially. All **32 editable masters** retained their hashes; all nine active action/cast masters were included. The [export record](player-actions/2026-11-20/exports.json) retains the commands' results and master hashes. The [rig audit](player-actions/2026-11-20/rig-audit.json) retains the 9.5-pixel shoulder/wrist bound and 40 matching idle transitions.

## Executed verification

Windows UCRT64 GCC 16.1.0, CMake 4.4.3, Ninja 1.13.2, SDL 3.4.12, zlib 1.3.2 and Python 3.14.6. Repeatable SDL checks use dummy video/software rendering and dummy audio.

| Check | Result |
|---|---:|
| Complete Debug SDL configuration | 124/124 CTest entries: 99 headless, 10 client, 15 graphics |
| Separate client/graphics-disabled configuration | 99/99; no SDL reference in its Ninja build graph |
| Python editor/authoring suite | 81 run: 79 passed, two unavailable-symlink skips |
| Melee presentation | 206 assertions |
| Melee rendering | 1,891 assertions |
| Existing Mining / woodcutting rendering | 425 / 521 assertions |
| Walking and movement presentation | 366 assertions |
| Audio routing / resource metadata | 32 / 45 assertions |
| Current packaged files | 47/47: 32 PNGs and 15 WAVs |

Tests exercise real killing blows retained through a catch-up tick, eight-way facing, hits/misses/blocks, weapon changes, cancellation, death, timing at 30/60 render updates, once-only swing edges, mixed pose-count boundaries and real renderer selection. The complete suites retain the production starter, persistence, autosave, recovery, publication, input and private packaging checks.

Deleting the copied melee equipment texture and rebuilding GameEngine repaired its exact bytes. All C++ object and executable timestamps stayed unchanged. Source refusal, previous-output preservation, spaced paths and multi-config packaging remain covered by the passing packaging integration test. See [packaging hashes and repair](player-actions/2026-11-20/packaging.json).

## Current game-renderer captures

All seven fixtures use the same integrated River Crossing renderer and load **32/32** textures. [Arguments, output and hashes](player-actions/2026-11-20/captures.json) accompany the images. These are software-rendered game captures, not a manually observed GPU playthrough.

| Melee | Gathering and movement |
|---|---|
| [Sword contact](player-actions/2026-11-20/sword-contact.png) | [Equipped walking](player-actions/2026-11-20/equipped-walk.png) |
| [Heavy contact](player-actions/2026-11-20/heavy-contact.png) | [Woodcutting contact](player-actions/2026-11-20/woodcutting-contact.png) |
| [Unarmed contact](player-actions/2026-11-20/unarmed-contact.png) | [Mining contact](player-actions/2026-11-20/mining-contact.png), [ore/reward](player-actions/2026-11-20/mining-reward.png) |

The inspected contact captures retain body/tool attachment and the rock/tree contact positions. The sword/shield contact renders alongside the authoritative block label. Existing prototype posing and remaining primitive monster art are retained; these captures do not establish owner visual approval.

## Starter replay on the combined source

The [fresh production journey](player-actions/2026-11-20/starter-journey.json) again equips the shield at tick **245**, sword at **376**, finishes preparation at **454**, defeats Mossling at **677** and returns to Mara at **741**. Deliberate death, respawn, persisted single-use recovery and safe return finish at cumulative tick **2370**. All six save/reload checkpoints preserve canonical bytes and owned items.

No material/XP/coin injection is used. The bank and live-Mara-dialogue predicates remain enforced. Simulation ticks measure this deterministic driver, not a human's navigation or elapsed play time.

## Reproduce and remaining acceptance

Build both configurations and run their full CTest sets as described in [getting started](GETTING_STARTED.md). Run `python -m unittest discover -s tests -p 'test_*.py'` and the read-only `python tools/art/check_world_art.py --check`.

Normal art export uses `python tools/art/build_<batch>_pilot.py --editor PATH` for cast, woodcutting, mining and melee; omit regeneration flags to preserve masters. Run exports before builds. If a restricted Windows session cannot write LibreSprite preferences, point the batch process's APPDATA at a disposable writable profile; do not change the user's real editor settings.

**REL-004 is complete for source integration/publication and automated gameplay/render/packaging verification.** GAME-005 / GAME-004 still need uncoached navigation, combat/food/recovery clarity and real elapsed-time observation. ART-004 still needs physical-device audible quality, repetition and balance review. QA-003 retains broader keyboard/text-scale/native Windows acceptance. ART-005 remains owner-deferred and unaccepted. None of these human/hardware acceptance fields is closed by the automated results above.

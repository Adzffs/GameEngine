# Animation integration, starter checks and sound handoff

**Date:** 20 November 2026.  
**Tasks:** REL-004, GAME-005 / GAME-004, ART-004; related packaging work in STUDIO-063.  
**Published baseline:** [3827ce8](https://github.com/Adzffs/GameEngine/commit/3827ce8dfb313bdb69a579dabc36853325648bf4).  
**Status:** Integration started in a separate local worktree. The Windows execution environment became unavailable before the combined build/export results could be retrieved. This handoff publishes the decisions and unfinished work; it does not publish the implementation or establish a passing combined build.

The [project review](PROJECT_REVIEW_2026-11-20.md) remains the source for completed safety fixes and executed baseline results. Do not attribute those results to this unfinished candidate. Animation quality remains owner-deferred under ART-005.

**Subsequent follow-up:** The [action-audio and packaging report](ACTION_AUDIO_2026-11-20.md) records independently published sound/copy changes and newer Linux checks. It supersedes the planned-only audio/copy status in this historical record. The original Windows animation merge and full starter acceptance remain open.

**Completed follow-up:** The [player action integration](PLAYER_ACTION_INTEGRATION_2026-11-20.md) recovers and verifies this work against current master. The interrupted status and proposed counts below are historical. REL-004 is now complete; human/audio/owner quality acceptance remains separate.

## Source and recovery position

| Source | Last recorded position | Meaning |
|---|---|---|
| GitHub master at interruption | 3827ce8 | Published Mining/shared gathering, critical save/item fixes and review documentation |
| Original local animation branch | polish/player-animation at 196a279 | Melee/equipment/polish plus the same safety fixes; lacks newer published Mining/editor work |
| Local review branch | review/project-audit-2026-11-20 at ce3160a | Tested published source reconstructed locally, with review documentation; synthetic ancestry |
| Integration worktree | GameEngine-next, branch feat/integrated-combat-audio | Based on ce3160a; integration edits were uncommitted at interruption |
| Original integration delta | Local 6bde02b to 688221b | Melee and polish changes, applied together to avoid reconciling the same conflicts twice |

The last recorded integration patch is animation-integration.patch in the sibling audit-2026-11-20 directory. Recover the existing worktree and inspect its status before applying any patch again. Some resolved working files may still have unmerged index entries. Preserve the original animation and review checkouts. This record is not a backup of their uncommitted files or editable binaries.

Do not replace master with either synthetic local snapshot history. Publish a reviewed diff against the current default branch while preserving unrelated upstream files. The default branch may have advanced since the baseline above.

## Integration findings and local edits to recheck

These describe the last recorded working tree, not verified published behavior.

| Boundary | Finding | Local resolution to verify |
|---|---|---|
| Gathering presentation | The separate animation line predates the shared Mining/Woodcutting state | Keep shared eligibility, interruption and committed-reward handling; choose authored pose durations per skill |
| Body/tool arrays | Polished woodcutting has eight poses; existing Mining masters have six | Keep separate array bounds and action bindings; never index the six-pose Mining atlas with an eight-pose loop |
| Authoring geometry | Mining previously reused the older chopping geometry; the polished chopping rig has different dimensions | Extract the original six-pose 64x80 geometry into mining_geometry.js; keep current Mining masters and default exports intact |
| Neutral character input | The new character sheet can be wider than the older sheet | Accept the supported 128/256-pixel layouts while sampling the intended neutral frame |
| Graphics and previews | Both histories modify player drawing, fixtures and atlas setup | Retain Mining and melee updates/draw paths, equipped walking, all action fixtures and conflicting-fixture rejection |
| Atlas generation | The older animation delta can drop Mining from batch aggregation | Retain all nine batches and regenerate bindings from all manifests |
| Runtime packaging | Several executables copy the same textures concurrently | Give copying one GameEngineWorldArt target, with each SDL consumer depending on it; verify output paths for every supported configuration |
| Generated headers | Art exporters and compilation both touch/read generated timing/layout headers | Finish exports before building; do not run those mutations concurrently |

Local code also retained the newer review documents, critical fixes and the shared gathering wrappers rather than accepting whole-file replacements from the animation patch.

### Authored timing and expected inventory

| Action | Pose timing in the local candidate | Required check |
|---|---|---|
| Walking | 110, 90, 100, 150, 110, 90, 100, 150 ms | Preview and distance-driven runtime agree; total preview cycle 900 ms |
| Woodcutting | 260, 200, 200, 80, 80, 80, 100, 200 ms | Eight poses, 1,200-ms cycle; contact pose 3 occupies 660-740 ms |
| Mining | Six poses of 200 ms | Preserve the existing 1,200-ms cycle, six-pose masters and lower rock strike |
| Melee | Weapon-specific windup/recovery from the local timing data | Preserve unarmed, sword, heavy and tool profiles; confirm attachments and interruption |

The calculated combined inventory is **3,063 frames in 32 textures across nine batches**: the published 1,175 frames, 1,792 melee/equipment frames, 32 additional walking frames and 64 additional woodcutting frames. This is an expected count for reconciliation, not an executed export/cache result.

## Resume REL-004

1. Inspect the recovered worktree, diff, index and any still-running jobs. Retrieve the mining export and full build logs if they survive. Do not infer success from a successful configure.
2. Review the resolved CMake, Graphics, ForestSprites, RenderPreviewMain and shared GatheringPresentationState changes together. Confirm both Mining and melee paths are present and preserve all recent data-safety fixes.
3. Run default art exports sequentially. Verify editable-master hashes are preserved, Mining retains six poses, authored timing reaches previews/runtime and all nine batches appear in the generated atlas. Use explicit regeneration only when intentionally changing art.
4. Build the combined Windows SDL configuration with an explicit working Python interpreter. Run the complete discovered CTest set and the Python editor unit suite. The anticipated pre-audio CTest inventory is 104 entries; discovery and actual results must confirm it.
5. Configure/build/test with client and graphics disabled. Confirm new presentation helpers do not introduce an SDL dependency into core.
6. Exercise clean and incremental parallel builds. Change or remove only a copied build-output texture, rebuild, and verify it is repaired byte-for-byte from source without requiring a C++ edit. Check all packaged texture hashes. The broader memory/resource-limit acceptance of STUDIO-063 remains separate.
7. Capture walking with equipment, chopping, Mining and melee in the same River Crossing renderer. Inspect pivots, all eight facings, tool direction, overlap, action cancellation and resource depletion. Automated captures do not establish owner visual acceptance.
8. Publish source, editable masters, timing, manifests, outputs and matching evidence as one reviewed integration. Update current counts and REL-004 only after the exact combined source is tested and published.

The configure command completed before interruption. A full build and a default Mining export had been started, but their completion/output was not recovered. No combined pass total, new gameplay result or visible acceptance is claimed here.

## Starter route to execute on that candidate

GAME-005 remains planned until the integrated candidate is available. Use a fresh isolated profile and the authored River Crossing world. Record the source/content revision, toolchain, launch parameters and start/end time.

| Segment | Observe and record |
|---|---|
| Arrival and gathering | Can the player identify the first goal, resource and valid tool without coaching? Check movement, interruption, depletion and reward feedback |
| Inventory, bank and services | Route clarity, capacity, shop prices, bank transfers and crafting cursor/grid ownership |
| First upgrades | Materials, attempts, travel, currency sources/sinks and actual elapsed shield/sword acquisition time |
| Combat and food | Threat recognition, attack/cancel behavior, food cost/use, escape and whether equipment feedback matches outcomes |
| Death and recovery | Death, respawn, recovery path/items, failed approaches and any confusing state |
| Save/reload and exit failure | Reload owned items/progression once; visibly exercise return-to-game and explicit unsaved exit after a deliberately induced save failure in a disposable profile |

The previous production replay measured shield at tick 245, sword at 376 and prepared return at 454, twice on its tested baseline. Re-run on the combined source. Simulation ticks and agent-driven fixtures do not measure uncoached human time. Leave those acceptance fields open until observed; record concrete blockers, fix them and retest the same route.

## Bounded sound pass after integration and route checks

ART-004 remains planned. The proposed first pass covers chopping/Mining contact, melee swing and hit/miss/block, committed gathering/crafting success, equipment and simple inventory/food confirmation. No music or ambient system is required for this pass.

| Event | Routing requirement |
|---|---|
| Tool contact | Use the authored presentation contact boundary; stop on cancellation; it must never grant items or XP |
| Gathering reward | Consume actual committed reward events once, including catch-up updates and depletion |
| Melee outcome | Identify attacker/defender/completion tick; retain weapon and position snapshots so fatal hits can still produce feedback; zero damage must be distinguishable from a miss |
| Crafting success | Emit only after output/inventory/XP transaction success; timer completion alone does not prove production |
| Equipment/inventory/food | Confirm an accepted command once; rejected or replayed commands must not sound successful |

Keep routing independent from the SDL backend so core still builds without audio. Initialize playback only for an actual client run, bound pending cues and simultaneous voices, and consume events on every simulation update before transient events are replaced. Cosmetic variants must not use gameplay RNG.

Provide working effects volume and mute controls, including immediate silence and no stale replay after unmuting. Do not show controls for music that does not exist. Missing optional clips or an unavailable device must leave gameplay usable, with restrained diagnostics.

Prefer a small original synthesized PCM set with its generator, manifest, format/duration/peak checks and provenance. Record who created it and its inputs. Do not invent a separate licence grant on the owner's behalf. If another source is used, record its actual distribution permission and attribution.

Verification must include duplicate/catch-up routing, interrupted actions, fatal/zero-damage/missed combat, rejected crafting, mute/volume, bounded overlap, missing clips/device, a silent headless build, and audible review in the real client. A dummy backend verifies plumbing only; it cannot establish sound quality.

## Publication and remaining acceptance

This handoff is documentation of interrupted work. REL-004 is in progress. GAME-005 and ART-004 remain planned. STUDIO-063 is not complete merely because the copy-owner change was drafted. ART-005 remains deferred by the owner.

When execution returns, recover and finish the integration first, then record the actual starter observations and sound implementation in their own evidence sections or dated reports. Keep [current status](../01-current-state/CURRENT_STATUS.md), the [roadmap](../02-roadmap/MASTER_ROADMAP.md) and both backlog formats synchronized.

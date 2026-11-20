# Player animation research and polish findings

**Research and prototype date:** 20 November 2026.  
**Scope:** existing player walking, equipped movement, melee attacks, visible weapons and chopping.  
**Local comparison baseline:** `16f9697` — melee attacks and equipped weapons.  
**Local polished revision:** `688221b` — player posing, timing and movement.  
**GitHub source inspected at publication:** [`a6d7d44a934212cef052f0a4ee7e06a9ea7d91a7`](https://github.com/Adzffs/GameEngine/commit/a6d7d44a934212cef052f0a4ee7e06a9ea7d91a7).

## Read this status first

This report preserves the animation research, measured problems, implementation decisions and results from the local polish session. Its local revision identifiers are not published GitHub commit links. The inspected GitHub revision already contains newer Mining work and a shared gathering presentation state, but does not contain the separate local melee/polish implementation or helper files described below. This documentation publication integrates the findings, not that runtime prototype. Use the [animation guide](../05-game/ANIMATION_GUIDE.md) for reusable practice and inspect the current source before reusing commands or implementation assumptions.

The earlier animation worked mechanically, but extended arms too far, kept the torso stiff, gave different weapon types similar motion, spaced poses too evenly and paired walking with a fast-start/slow-finish glide. The local polish rebuilt bounded joints, coordinated body motion, weapon-specific timing, eight-pose walking and steady accepted-step traversal while retaining the original character design, palette, eight facings and authoritative outcomes.

## Research sources and how they informed the work

These are primary sources by the animator, developer or tool maintainer. Findings below are paraphrased. The application column records our interpretation and implementation choice, rather than a requirement from the source. Links were checked again on 20 November 2026. No tutorial artwork was imported.

| Source | Relevant finding | Application to Embervale |
|---|---|---|
| [Mariel Cartwright — Fluid and Powerful Animation within Frame Restrictions, GDC 2014](https://media.gdcvault.com/GDC2014/Presentations/Cartwright_Muriel_Animation_Bootcamp_Fluid.pdf) | Strong silhouettes, anticipation, follow-through, selective smears, overshoot and deliberately held keys carry limited-frame animation. Gameplay constrains timing. See PDF pages 6–13 and 20–25. | Prioritize loaded/contact/settle poses, reserve sword accents for the strike, and fit anticipation around the existing result schedule. |
| [Raymond Schlitter / SLYNYRD — Pixelblog 55: Top Down Character Animation](https://www.slynyrd.com/blog/2025/3/24/pixelblog-55-top-down-character-animation) | Rotation and synchronized directional playback expose inconsistent anatomy. Limb thickness, shoulder pivots, bounce, asymmetric details and separate costume layers need deliberate review. | Review eight directions at the same phase; keep body proportions and gear handedness consistent; articulate walking instead of duplicating a few legs. |
| [Raymond Schlitter / SLYNYRD — Pixelblog 56: Top Down Character Attack Animation](https://www.slynyrd.com/blog/2025/5/23/pixelblog-56-top-down-character-attack-animation) | Weapon identity comes from trajectory and phase duration. A sword sweep, spear thrust and heavy hammer use different anticipation/follow-through. Smears belong to fast forward motion. | Separate punch, sword, two-handed and tool motions. Make the heavy recovery longer and keep recovery clear of attack effects. |
| [Pedro Medeiros / Saint11 — Top Down Attack](https://www.patreon.com/saint11/posts/top-down-attack-21959820) | Layered legs, torso and head can shift together in the attack direction. | Use a coordinated lunge with a stable foot reference, instead of rotating an isolated arm on a motionless torso. |
| [Thomas Vasseur / Motion Twin — Using a 3D pipeline for 2D animation in Dead Cells](https://www.gamedeveloper.com/production/art-design-deep-dive-using-a-3d-pipeline-for-2d-animation-in-i-dead-cells-i-) | A reusable production pipeline makes pose/timing retakes and equipment reuse affordable; low-resolution rendering has its own cleanup costs. | Keep the existing native 2D pipeline, but share joints, attachment definitions and timing so retakes do not require separate fixes to each overlay. This was not a migration to 3D. |
| [Roman Gurbanov — Anticipation, smearing, easing and secondary animation](https://pixelart.school/learn/pixel-art-anticipation) | Uneven holds and rapid transitions communicate force; secondary elements lag and settle after the main action. | Use brief final preparation, fast contact and weighted settle; keep secondary pixels restrained. |
| [Official Aseprite documentation — Frame Duration](https://aseprite.org/docs/frame-duration/) | Individual frames and selected frame ranges have explicit durations. | Treat durations as authored data, and make runtime selection and exported previews agree. |

The displayed publication dates for Pixelblog 55 and 56 are 31 March and 2 June 2025, respectively; the older date segments in their URLs differ. Source examples are teaching references, not a specification for this game's action speed.

### Reference timing examples

Pixelblog 56 supplies six-frame examples with the following durations. These illustrate how distribution changes perceived weight; they are not the timings applied to Embervale.

| Source example | Frame durations, ms | Total |
|---|---|---:|
| Sword | `[100, 50, 50, 50, 100, 50]` | 400 ms |
| Spear | `[200, 50, 50, 50, 150, 50]` | 550 ms |
| Hammer | `[250, 50, 50, 50, 300, 100]` | 800 ms |

## Audit: what looked wrong and what changed locally

| Observed problem | Local correction | Review lesson |
|---|---|---|
| Shoulder-to-wrist distance reached about 20.32 native pixels | Shared bounded upper/lower-arm geometry limits that distance to 9.5 pixels before rasterization | A wrist target needs anatomical constraints; direction math alone is insufficient |
| Torso and legs contributed too little | Coordinated body load, lunge, head/torso movement and stable feet | Sell weight with the whole body |
| Weapons felt too similar | Five body styles: unarmed, sword, two-handed, axe and pickaxe | Reuse the rig, while authoring trajectories appropriate to the weapon |
| Waiting and strike phases looked evenly paced | Guard during most of the wait, short final coil/load, fast contact, weighted recovery | Rhythm matters more than adding evenly spaced in-betweens |
| The closing pose popped back to equipped idle | Explicit settle coordinates match equipped idle | Review the loop seam with equipment visible |
| Four walking poses and duplicated leg shapes looked stiff | Eight articulated poses with near/far leg gait, waist retention, slight bob and counter-swing | Review body, feet and hand attachments together |
| Exponential visual traversal accelerated then drifted into each tile | Constant traversal of an accepted 600-ms step; existing run rate retained | Pose quality and movement interpolation are separate causes of jank |
| Rear diagonal axe heads disappeared behind the face | Place the head outside the face silhouette while the cutting edge still faces the tree | Validate visible tool pixels in every direction |
| Walking elbows looked excessively splayed | Restrain elbow bend using available reach/foreshortening | A mathematically valid joint can still create an unnatural silhouette |

The rig audit recorded a maximum shoulder-to-wrist distance of **20.3226474653 px before** and **9.5000000000 px after**. The local audit entry point was `node tools/art/audit_player_rig.js`. The arm limit belongs to this character scale and ordinary action posing.

Elbow bend was reduced to `min(2, sqrt(max(0, 25 - n*n/4)))` in the local rig, where `n` is the shoulder-to-wrist separation in native pixels. This is an implementation detail of the small sprite's two-segment arm, not a generic rig formula to copy into bodies of another size.

The close-to-idle review covered **40 geometric transitions**: five body styles across eight facings. A separate equipped pixel check covered **64 weapon/direction closures**: eight main-hand types across eight facings. Empty unarmed defaults were outside that equipment comparison. All eight original neutral character cells remained pixel-identical.

## Timing decisions

### Melee phases and authority

The local sheet has eight walking columns followed by eight attack columns. Attack keys are **guard, coil, loaded, contact, overshoot, follow-through, return, settle**.

Keep a stable guard through most of the authoritative wait. Use coil/load in the final anticipation window. A new committed world result starts contact/recovery; a paused world must never produce a visually invented attack completion. These durations describe presentation and do not add cooldowns or alter action completion.

| Profile | Final anticipation | Contact exposure | Contact + complete recovery | Per-key durations from contact through settle |
|---|---:|---:|---:|---|
| Unarmed | 150 ms | 50 ms | 250 ms | `[50, 40, 50, 50, 60]` |
| Sword | 220 ms | 70 ms | 300 ms | `[70, 40, 70, 50, 70]` |
| Two-handed heavy | 320 ms | 90 ms | 440 ms | `[90, 60, 110, 80, 100]` |
| Axe / pickaxe | 280 ms | 80 ms | 360 ms | `[80, 50, 90, 60, 80]` |

Contact exposure is the first value of the five-key recovery sequence, and is included in its total.

Unarmed uses a jab, swords sweep across the body, the godsword commits to a heavier two-handed action, and axes/picks use an overhead bite. Short angular sword accents appear only on contact and overshoot. A shield stays in the offhand for one-handed gear and is slung behind the two-handed godsword.

The local main-hand types were bronze sword, developer godsword, bronze/iron/steel axes and bronze/iron/steel pickaxes, with a wooden shield overlay.

The local transient `MeleeCombatFeedback` used the completion tick, actor/defender positions and weapon snapshot. Event/tick identity distinguishes repeated equal-damage results. Misses, blocked/zero-damage results, killing blows and immediate opening completions still play their committed action. Movement, equipment changes, non-melee actions, death, cancellation and invalid range clear stale melee posing.

### Chopping

The eight-key cosmetic loop uses **`[260, 200, 200, 80, 80, 80, 100, 200]` ms**, totaling **1,200 ms**. The contact key is index 3, from 660 ms through 740 ms. The sequence provides preparation, strike/bite, rebound, lowering and settle.

Chips communicate a tool attempt. Logs and XP are shown only from committed gathering events. Tool cadence and rewards remain gameplay-owned. The rear-diagonal blade placement was checked in pixels; the northwest head initially remained obscured and was corrected.

### Walking and interpolation

The eight-pose walk uses **1.5 travelled tiles per full cycle** and phase weights **`[110, 90, 100, 150, 110, 90, 100, 150]`**. They total **900 ms** for the steady-speed preview. Runtime phase follows normalized distance, with the same unequal cumulative phase weights. Stopping selects neutral.

The local cast atlas changed from **128×384** to **256×384**, expanding four walking poses to eight while preserving all eight neutral cells. There are 64 player cells plus Mara and the furnace: 66 cast frames.

The old visual smoothing expression was `1 - exp(-14 * dt)`. Local polish replaced its fast-start/long-tail motion with constant visual traversal over the server's **600-ms accepted step**, preserving the two-tile run rate. Walking and running were checked at 30 and 60 render updates per second and for overshoot. This describes the local branch, not an assertion that the later GitHub movement code has the same implementation.

## Local authoring and integration design

| Responsibility | Local prototype path / rule | Publication status |
|---|---|---|
| Authored timing | `assets/embervale/player-animation.json` | Local prototype file, not present in the inspected GitHub tree |
| Timing validation and GIF retiming | `tools/art/animation_common.py` | Local prototype helper |
| Generated runtime timing | `src/Animation/PlayerAnimationTiming.h` | Local prototype generated header |
| Shared body/tool geometry | `tools/art/player_rig.js` | Local prototype helper |
| Geometry audit | `tools/art/audit_player_rig.js` | Local prototype helper |
| Editable source | LibreSprite `.ase` masters per batch | Preserve saved source during a normal export |
| Runtime | Generated atlas geometry, PNGs and existing SDL renderer | Package exported textures beside the executable |

The timing validator checked expected frame counts, positive delays in 10-ms multiples, the 1,200-ms chopping sum and the fixed profile order. Preview retiming changed GIF Graphic Control Extension delays after LibreSprite image export; Python did not edit sprite pixels.

Default export preserved the saved masters. Intentional regeneration was explicit. This distinction prevents a future routine export from erasing manual art edits.

### Local atlas inventory

| Atlas | Dimensions | Cells | Geometry |
|---|---:|---:|---|
| Cast player | 256×384 | 64 player cells | 32×48 cells |
| Melee bodies | 1,536×3,840 | 640 | 96×96 cells, pivot `(48, 64)` |
| Melee equipment | 1,536×6,912 | 1,152 | Same action cell/pivot |
| Chopping body | 768×768 | 64 | Same action cell/pivot |
| Chopping axes | 768×2,304 | 192 | Same action cell/pivot |

The action pivot corresponds to `(16, 24)` in the original player cell. The local cache reported **2,871 bound frames in 30 textures**. By comparison, the inspected GitHub source documented **1,175 frames in 30 textures** after Mining integration. These counts belong to different source snapshots and must not be combined or used to claim this prototype is on `master`.

The polish did not change collision, reach, damage/RNG, item or XP rewards, requirements, action intervals or save formats.

## Recorded local verification

These are results recorded for local revision `688221b`, not checks rerun against GitHub during documentation publication.

**Environment:** Windows UCRT64 GCC 16.1.0, CMake/Ninja, SDL 3.4.12, SDL dummy video and software rendering. A visible GPU/manual playthrough was not performed.

All **12 focused suites passed**:

1. `MeleeAttackResolverTests`
2. `MeleeCombatActionTests`
3. `MeleeEngagementTests`
4. `EnginePlayerPersistenceLifecycleTests`
5. `WoodcuttingPresentationTests`
6. `MeleePresentationTests` — 179 assertions
7. `PlayerAnimationTests`
8. `WoodcuttingGraphicsTests` — 521 assertions
9. `MeleeGraphicsTests` — 1,891 assertions
10. `MeleeClickIntegrationTests`
11. `CombatFeedbackTests`
12. `CameraPresentationTests`

Additional checks recorded:

- All eight original neutral cells were pixel-identical to baseline.
- The rig stayed within the 9.5-pixel reach bound; all 40 geometric and 64 equipped closures passed.
- Default exports preserved seven `.ase` masters across cast, melee and woodcutting.
- Seven packaged cast/melee/woodcutting textures matched the exported files by SHA-256.
- GIF frame counts and delays matched the authored data: walk 8/900 ms, melee preview 8/820 ms, chop 8/1,200 ms.
- Five actual River Crossing render fixtures loaded all 30/30 textures and showed sword, axe, godsword, equipped walking and chopping.
- Walking and running remained consistent at 30/60 updates per second without visual overshoot.

The final code build used six-way parallel compilation. An asset refresh encountered a copy race because existing post-build targets copied to the same texture destinations concurrently; a serialized refresh succeeded. No build-system source change was included in the polish. If reproduced, treat this as a separate packaging concern rather than an animation defect.

## Comparison and capture fixtures

The local before/after GIF was an **authoring comparison, not a gameplay recording**. It showed before on the left and after on the right, with South/East on top and North/Northeast below. Both variants used the same two-tick attack: start at 0, committed contact at 1,200 ms, complete comparison loop at 1,500 ms. The GIF contained 75 frames at 20 ms each.

The baseline held three anticipation poses for 400 ms each, then contact/follow/recovery at 100 ms each. The polished sword held guard for 980 ms, coil for 132 ms and loaded for 88 ms, then used `[70, 40, 70, 50, 70]` ms through contact and settle. The separate 820-ms melee authoring preview used a shorter 300-ms guard; it was not a world cooldown demonstration.

Actual render fixtures used `content/worlds/river-crossing.blockout.json` with the following local `EmbervaleRenderPreview` options:

| Output | Replay options |
|---|---|
| `sword-contact.png` | `--focus 26 68 --melee 27 68 --weapon bronze-sword --shield --zoom 1.5 --frames 32` |
| `axe-contact.png` | `--focus 26 68 --melee 27 68 --weapon bronze-axe --zoom 1.5 --frames 32` |
| `godsword-contact.png` | `--focus 26 68 --melee 27 68 --weapon godsword --zoom 1.5 --frames 32` |
| `equipped-walk.png` | `--focus 26 68 --walk-to 26 67 --weapon bronze-sword --shield --zoom 1.5 --frames 8` |
| `chopping.png` | `--focus 7 55 --chop 8 55 --zoom 1.5 --frames 18` |

These melee/weapon replay options belong to the local prototype. Verify available options in the current executable before running them.

### Evidence availability and recovery inventory

This publication preserves the textual findings and recorded measurements. The local computer became unavailable during publication, so its binary previews, captures, raw result JSON, `.ase` masters and implementation files could not be attached here. The following are recovery paths, not links to files already published:

```text
docs/embervale/06-development/art/2026-11-20/animation-polish/
  melee-before-after.gif
  before-walking.gif
  before-melee.gif
  before-chopping.gif
  sword-contact.png
  axe-contact.png
  godsword-contact.png
  equipped-walk.png
  chopping.png
  results.json
assets/embervale/cast-pilot/previews/walk-inspection.gif
assets/embervale/melee-pilot/previews/melee-cycle.gif
assets/embervale/woodcutting-pilot/previews/chop-cycle.gif
```

The local `results.json` recorded environment, focused results, checksums, joint audit, GIF metadata, world fixture and the absence of a visible GPU playthrough. Recover that raw record before claiming an independently reviewable checksum archive.

The currently published [Mining report and captures](MINING_ANIMATION_2026-11-20.md) and [Woodcutting report and captures](WOODCUTTING_ANIMATION_2026-11-20.md) provide evidence for their own earlier integrations. They do not substitute for the unpublished polish comparison.

## Reproducing the local prototype after recovery

First recover/reconcile local `688221b` with the current source, preserving the later shared Mining/Woodcutting state. Confirm that the helper files, options and baseline artwork are present. Then the local authoring commands were:

```powershell
python tools/art/build_cast_pilot.py --editor "C:\Program Files\Asprite\libresprite.exe" --rebuild-walk
python tools/art/build_melee_pilot.py --editor "C:\Program Files\Asprite\libresprite.exe" --regenerate
python tools/art/build_woodcutting_pilot.py --editor "C:\Program Files\Asprite\libresprite.exe" --regenerate
node tools/art/audit_player_rig.js
```

`C:\Program Files\Asprite\libresprite.exe` was the local editor location, not a portable prerequisite. Substitute the installed editor path. Normal export omits the rebuild/regeneration flags to preserve manual edits. The optional melee `--compare-with DIRECTORY` fixture expected baseline `melee-player.png` and `melee-equipment.png` in that directory.

Re-run the appropriate focused checks and render fixtures after reconciliation; the recorded results above do not prove a later combined version. Restore before/after evidence with an identical camera, equipment, phase and timeline.

## Decisions to carry into future animation work

1. Fix silhouette, attachment and weight before increasing frame count.
2. Compare every facing at the same phase; inspect rear equipment occlusion in pixels.
3. Reuse geometry and timing infrastructure while preserving weapon-specific motion.
4. Drive walking from travel and validate interpolation separately from pose authoring.
5. Fit presentation around authoritative actions; committed results own damage, items and XP.
6. Treat interruption and loop closure as part of the animation, not cleanup work.
7. Keep masters editable and make preview/runtime timing agree.
8. Record source revisions and distinguish local experiments, published integrations and visible acceptance.

For future work, start from the [animation guide](../05-game/ANIMATION_GUIDE.md), then the current source and the newest action report. Update this dated report only to correct or complete its historical evidence; put later changes in their own dated record.

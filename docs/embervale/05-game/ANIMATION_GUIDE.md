# Animation guide

**Reusable custom characters, 22 November:** Read [custom-character design](CUSTOM_CHARACTERS.md), [workflow](CUSTOM_CHARACTER_WORKFLOW.md) and [v1 findings](../06-development/CUSTOM_CHARACTER_REVIEW_2026-11-22.md) before adopting the new body. Its 80×80/(40,70) packed geometry and south-only idle/walk are not existing runtime bindings. Resolve the body pose once and share it across compatible layers; author hand/ordering tracks for the chosen body, preserve anatomical handedness and validate unmasked tool clearance before compositing. The first proof is not owner visual acceptance. The separate later PixelLab locomotion candidate has a different idle/moving slot contract; reconcile the actual checkout rather than copying old counts or timelines.

**Resource guard follow-up, 20 November:** Validate/export against [asset resource limits](ASSET_RESOURCE_LIMITS.md). The [completed player action integration](../06-development/PLAYER_ACTION_INTEGRATION_2026-11-20.md) now has eight walking/chopping poses, six Mining poses and sixteen equipped walking/attack frames per direction. Both resource validators and runtime arrays agree. A different authored pose count requires matching runtime arrays and both validation tables, followed by complete binding/render checks; retain the lossless packed output instead of loosening budgets for the original oversized editable canvases. The read-only check preserves masters and geometry; further animation polish remains owner-deferred.

**Purpose:** Reusable production reference for player, equipment, gathering and future creature animation.  
**Updated:** 20 November 2026.  
**Status:** Adopted working guidance; exact frame counts and timings remain specific to each authored action.

Read this before animating. The [research and measured findings](../06-development/ANIMATION_RESEARCH_2026-11-20.md) explain the sources, defects, prototype corrections and verification behind these rules. [Art and audio](ART_AND_AUDIO.md) owns the wider asset pipeline and current integration status.

## Start an animation task

**Garment alignment and coverage:** Register complete contours and cuff cross-sections to the actual source pose; verify protected body labels and inter-leg spaces, reject detached scraps and retain every-frame evidence at integer zoom. Check every exported pixel against the editable master. The [22 November alignment correction](../06-development/GARMENT_ALIGNMENT_2026-11-22.md) records why whole-area coverage alone still passed misaligned garments and mislabelled forearms. Owner visual acceptance remains separate from automated checks.

1. Read this guide, the dated research, the current art guide and the relevant action's source and tests.
2. Record the actual checkout commit. Compare current code with the dated report before reusing its helper names or numbers.
3. Write the visible goal, authoritative trigger, interruption rules, facings, equipment and ground contact point.
4. Block the main poses and their timing at gameplay scale. Review one complete direction before expanding to all eight.
5. Validate body and equipment together, then the real renderer and relevant gameplay transitions.
6. Save the editable masters, authored timing, generated outputs and evidence together; update the owning report.

The 20 November polish report describes local prototype `688221b`, following local melee baseline `16f9697`. Those are local revision identifiers, not published GitHub commit links. The later [player action integration](../06-development/PLAYER_ACTION_INTEGRATION_2026-11-20.md) recovers those helpers and reconciles them with current shared Mining/Woodcutting, audio and resource guards. Inspect the current checkout before reusing historical commands or counts.

## Make the action readable before adding frames

Prioritize silhouette, weight, ground contact and rhythm. More evenly spaced frames cannot repair a weak contact pose or a weapon hidden inside the head.

| Element | Working rule | What to inspect |
|---|---|---|
| Preparation | Briefly load the body opposite the main action | Intent is readable without delaying an accepted input |
| Strike | Use a short, decisive change of pose | Hands, weapon and target have a clear relationship |
| Weight | Coordinate feet, pelvis, torso, shoulders and head | The body contributes instead of a detached arm doing all the work |
| Follow-through | Allow a restrained overshoot or rebound | Motion has momentum without appearing to pass through every surface |
| Recovery | Return through a purposeful intermediate pose | Equipment and body settle into the correct equipped idle |
| Secondary motion | Add small delayed hair, cuff or cloth motion only where useful | No noisy pixels or perpetual wobble |
| Effects | Reserve short smears or arcs for the fast strike | The return does not look like another attack |

Use longer readable telegraphs for a future enemy attack when its gameplay design requires reaction time. Player preparation must fit the existing input/result contract. Hitstop, camera shake, displacement and new attack delays require their own concrete design and verification; they are not automatic polish.

## Preserve the character across eight directions

The existing player row order is **South, West, East, North, Southwest, Southeast, Northwest, Northeast**. Preserve the order unless the complete authoring and runtime mapping is deliberately migrated.

Review a stationary rotation and all directions at the same action phase. Keep head size, limb thickness, pelvis height and foot contact consistent. Diagonals need authored silhouettes; a rotated or mirrored cardinal pose is not sufficient when sword, shield, hair or clothing is asymmetric.

Keep the main-hand weapon in the same anatomical hand. Select rear/front body and equipment layers by pose and facing. A rear diagonal can hide the tool head behind the face even when its mathematical angle is correct; inspect the actual pixels. For axes, the blade's cutting edge faces the tree. For picks, the leading point faces the rock. Place the head outside the face silhouette where necessary without detaching the grip.

Treat equipment as attached to a joint. Body and overlay must consume the same wrist/grip coordinates. Use bounded shoulder/elbow/wrist geometry for ordinary poses; intentional stylized deformation needs a visible reason and a clear recovery. The prototype's 9.5-native-pixel shoulder-to-wrist limit is specific to this character, not a universal anatomical constant.

## Keep feet, movement and equipment coherent

Use the established ground pivot through every frame. The player base uses 32×48 cells; the editable action masters use 96×96 cells with pivot `(48, 64)`, corresponding to base ground point `(16, 24)`. Melee runtime output is losslessly cropped/packed with adjusted per-frame pivots. Check the actual manifest/binding before changing geometry; editable cell size is not the packed rectangle size.

Walking phase should follow visual distance travelled, stop on a neutral pose and remain consistent across render rates. Coordinate the two legs, preserve the waist, use restrained torso/head motion and counter-swing the arms. Test walking with a sword, shield and two-handed equipment as well as an empty body.

Review movement interpolation separately from drawn poses. Fast starts followed by long asymptotic drift can look like skating even with good artwork. The local polish used constant traversal of each accepted 600-ms step and retained the existing two-tile run rate. That is a documented prototype choice; verify the current movement implementation before applying it.

Attack posing may move the torso within the sprite, while planted feet retain a stable reference. Sprite motion must not silently change reach, collision, navigation or authoritative position.

## Author timing deliberately

Use unequal holds: stable preparation, fast strike, readable follow-through and a controlled settle. Review a loop at its actual speed, frame by frame, and at normal camera scale.

Keep one authored timing definition wherever the pipeline supports it. Generate runtime constants and preview timing from it; avoid independently maintained GIF and C++ clocks. An eight-frame sheet does not require equal eighths of the cycle.

These are **local prototype starting points**, not weapon cooldowns or new gameplay delays:

| Style | Final anticipation | Contact exposure | Contact plus recovery |
|---|---:|---:|---:|
| Unarmed | 150 ms | 50 ms | 250 ms |
| Sword | 220 ms | 70 ms | 300 ms |
| Two-handed heavy | 320 ms | 90 ms | 440 ms |
| Axe / pickaxe | 280 ms | 80 ms | 360 ms |

Contact exposure is included in the last column. These windows fit around existing authoritative outcomes. Preserve the current world action schedule.

The local walk cycle used 1.5 tiles per complete cycle with eight phase weights `[110, 90, 100, 150, 110, 90, 100, 150]`; the 900-ms preview represents steady walking. The local chopping loop used `[260, 200, 200, 80, 80, 80, 100, 200]` ms, totaling 1,200 ms. See the research report for per-style recovery timing and comparison fixtures.

## Keep gameplay authority explicit

Animation describes an action or committed result; it does not award that result.

| Situation | Presentation requirement |
|---|---|
| Approach | Continue travel until the accepted action is eligible and visual movement has settled |
| Waiting for melee completion | Hold guard; show the final anticipation without inventing a contact |
| New committed melee result | Start the appropriate contact/recovery once, including misses, blocks and killing blows |
| Repeated identical damage | Deduplicate by event/tick identity, not the damage value |
| Gathering attempt | A cosmetic tool contact/chip can repeat; it grants no item or XP |
| Committed gathering reward | Show the actual resource/quantity/XP for the actor and skill, once |
| Depletion on the final use | Retain the committed reward notice after the action ends |
| Movement, cancellation or invalid range | Stop the invalid action pose and return through the correct presentation state |
| Wrong tool, gear change, target loss or death | Revalidate or clear the pose; never retain detached equipment or a stale hit |
| Paused simulation / render-rate change | No invented outcomes; timing remains consistent with the action contract |

The current [Mining integration](../06-development/MINING_ANIMATION_2026-11-20.md) owns the shared `GatheringPresentationState` implementation. Future gathering polish should extend that shared boundary and preserve skill-specific reward routing.

## Follow the editable asset workflow

Use the existing LibreSprite `.ase` masters and export scripts. Normal export must preserve manual source edits. Use explicit regeneration flags only when intentionally rebuilding the authored source.

Inspect the relevant batch README before running its builder. Current entry points include [cast](../../../assets/embervale/cast-pilot/README.md), [woodcutting](../../../assets/embervale/woodcutting-pilot/README.md), [mining](../../../assets/embervale/mining-pilot/README.md) and [melee/equipment](../../../assets/embervale/melee-pilot/README.md). The recovered shared rig and timing helpers are published with the combined integration.

Keep nearest-neighbour scaling, transparent backgrounds, consistent palette and pivot alignment. Repack the atlas only with the matching generated geometry and packaging changes. Never fix a source defect only in an exported PNG. Do not use blur or arbitrary interpolation to hide inconsistent native pixels.

Reference artwork remains reference artwork. Record creator, URL and rights; do not import tutorial sprites or effects without the necessary permission. The research used production lessons while retaining the project's original character.

## Review and verify

Use checks that protect the action's actual risks. A documentation-only change needs link/consistency review; an animation change needs appropriate rendering and state verification.

- Compare idle, loaded, contact, overshoot and settle in all eight directions with the same clock.
- Inspect body plus every supported equipment class, offhand use and two-handed rules at native scale and real camera zoom.
- Check hand attachment, tool edge/point orientation, rear occlusion, clipping and stable feet.
- Check starting, stopping, turning, cancellation, range loss, gear changes, target removal and death.
- Verify hit, miss, repeated equal damage, opening hits and final-use/killing outcomes where relevant.
- Compare travel and animation at different render rates; ensure packaged textures match the authored exports.
- Confirm normal export preserves masters and regenerated previews use the authored timing.
- Review the real renderer against representative ground, resources, roofs and adjacent actors.
- Label a contact sheet, authoring GIF, dummy-video capture and visible playthrough accurately; each proves different things.

A passing atlas or joint check does not establish that the animation feels polished. Watch the full action in context and record remaining visible issues.

## Extend the same lessons

| Future work | Apply | Specific concern |
|---|---|---|
| Mining | Shared body/tool attachments; a lower rock contact pose and pick rebound | Retain the shared Mining/Woodcutting state and actual ore feedback |
| More melee weapons | Reuse the rig, author a distinct silhouette and weight profile | Do not copy a sword arc onto every weapon or infer reach from pixels |
| Monsters | Clear anticipation, attack and recovery; planted base or deliberate locomotion | Telegraph timing must agree with the encounter and committed outcomes |
| Hurt / death | React to actual world state and define interruption priority | No cosmetic knockback that silently moves authoritative entities |
| Idles / cloth | Restrained uneven holds and small secondary motion | Avoid busy motion or changing the ground pivot |
| New character bodies | Reuse workflow and review order | Re-measure joint limits, silhouettes and scale for that body |

## Record the next handoff

Include the following in the action's dated report:

- Checkout/branch and baseline commit; implementation, proposal or local prototype status.
- Intended visible change and the source references that informed it.
- Editable masters, scripts, timing data, exports, bindings and packaged files touched.
- Direction mapping, cell dimensions, ground pivot, key poses, duration arrays and equipment rules.
- Authoritative trigger, event identity, interruption rules and unchanged gameplay contracts.
- Exact checks run, environment, results and any visible acceptance still outstanding.
- Before/after evidence using the same fixture, camera, equipment and timeline.
- Remaining issues and the next concrete change; links from the current index.

Keep this guide focused on reusable practice. Put new measurements and changed timings in the dated action report, then update this guide only when the lesson applies beyond that action.

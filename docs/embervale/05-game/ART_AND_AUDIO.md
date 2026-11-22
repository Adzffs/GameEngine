# Art, animation and audio production guide

**Owner decision, 22 November 2026:** PixelLab is the chosen tool for Embervale artwork. Animation production is paused until the owner confirms his subscription is ready. Then build the reusable artwork pipeline around PixelLab, shared motions, separate equipment and material variants. Preserve existing assets and use local editors/exporters to support cleanup, integration and validation. Do not resume manual/procedural animation polishing during the pause.

**Status:** Ten world-art batches in the current walking candidate render terrain, cast, resources, services, buildings, contextual variation and gathering actions. The player has eight-way walking, chopping and mining; the shared cache covers 3,135 frames in 33 textures. [Current asset resource limits](ASSET_RESOURCE_LIMITS.md) now validate metadata, PNG streams, cache costs and pre-copy resources. The ten-batch candidate uses about 23.21 MiB of logical RGBA pixels and 5.80 MiB of retained alpha; driver/allocator/decoder overhead is separate. A general importer and the broader production workflow below remain proposed.\
**Reviewed:** 21 November 2026.\
**Purpose:** Replace placeholder presentation in manageable steps while preserving gameplay and making assets easy for humans and AI to work with.

The [PixelLab walking report](../06-development/PIXELLAB_WALKING_2026-11-21.md) records the separate empty-handed AI batch, editable master, eight directions, 600-ms ordinary-walking preview and current verification. Existing equipped/gathering/melee batches remain independent; the new gait does not reuse their old wrist overlays.

## First editable batch

The [forest pilot](../../../assets/embervale/forest-pilot/README.md) contains
three grass variants, sixteen connected dirt path tiles, and an ordinary tree
with its matching depleted stump. The 32-pixel ground tiles and 64x96 resource
sprites have LibreSprite `.ase` masters, PNG exports, stable authoring IDs,
pivots, source/output checksums and recorded provenance. The retained generated
tree reference is adapted through LibreSprite JavaScript; terrain is drawn by
the script. A default export command preserves manual edits and rebuilds the
offline native/2x previews from the saved masters.

The kit now has a generated C++ geometry binding and a renderer-owned texture
cache. Normal trees choose intact/depleted images from authoritative resource
state. Ground uses cardinal path connections, with filled quarters for wider
roads. World objects sort by their ground contact points; a tree fades when it
overlaps the player behind it. Opaque canopy pixels target the existing tree
tile while transparent areas retain their normal world actions. F6 switches
to basic shapes, and missing/invalid images fall back individually.

The [integration record](../06-development/FOREST_RENDERING_2026-11-19.md)
includes actual game-renderer captures for intact/depleted trees, overlap and
basic mode, plus a reusable snapshot preview command. SDL 3.4+ enables the PNG
pilot; builds with earlier SDL headers retain basic shapes. Assets are copied
beside the executable by CMake and loaded once, outside the render loop.

The original forest pilot began ART-001 and STUDIO-060/061/063. Bounded STUDIO-063 resource validation is now complete for the full current set; see [verification](../06-development/ASSET_RESOURCE_VALIDATION_2026-11-20.md). This remains a pilot rather than a general
game/Studio importer or finished starter kit. The remaining cast/prop kit, Studio asset
editing, visible interaction and wider acceptance remain. The pilot's
`manifest.json` is an authoring record; the runtime consumes the generated
binding and PNGs rather than parsing a new JSON schema.

## 1. Current presentation and the recommended transition

The next [cast pilot](../../../assets/embervale/cast-pilot/README.md) is now
integrated: an eight-direction player walk sheet, Mara's blue/gold warden sprite,
and a stone furnace. The same cache and generated binding handle both batches.
Walking follows visual travel distance and stops on a neutral frame. See the
[cast integration record](../06-development/CAST_RENDERING_2026-11-19.md) for
editable source/export details and actual River Crossing captures. Player/prop
art is now present in this bounded pilot; the remaining cast, gear, animation
states and visible acceptance remain open.

The [woodcutting batch](../../../assets/embervale/woodcutting-pilot/README.md)
adds eight weighted action poses in every direction and bronze/iron/steel axe overlays.
The player faces the active tree, swings while gathering and stops cleanly on
interruption. Contact chips are cosmetic; floating log/XP feedback consumes
committed world rewards, including depletion. [Local checks and game captures](../06-development/WOODCUTTING_ANIMATION_2026-11-20.md).
The [mining batch](../../../assets/embervale/mining-pilot/README.md) adds lower
rock strikes and bronze/iron/steel pickaxe overlays in all eight directions.
It shares the gathering presentation and interruption rules with woodcutting.
Rock chips mark contact; actual ore and Mining XP notices come from committed
world rewards. [Local checks and game captures](../06-development/MINING_ANIMATION_2026-11-20.md).
The [recovered melee/equipment batch](../../../assets/embervale/melee-pilot/README.md) now supplies equipped walking and eight-way melee with matching sound edges; see [integration and verification](../06-development/PLAYER_ACTION_INTEGRATION_2026-11-20.md). Remaining cast and wider action states stay open.

The current client uses SDL3 drawing code for the world and entities. `Graphics.h` defines a 32-pixel tile size and a 1280×720 window. The rendering code includes terrain colours, resource shapes, combat feedback and world labels. This does not establish an existing general-purpose sprite import pipeline. Sources: [Graphics.h](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Graphics/Graphics.h), [Graphics.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Graphics/Graphics.cpp).

Keep that presentation as a useful debug mode. Begin custom art with one coherent terrain family, one tree, one station and one character. Validate those assets in the game, Studio and previews before producing hundreds more. The first goal is a reliable asset-to-game workflow, followed by a consistent visual identity.

A large graphics source file and a large sprite atlas are different concerns. The former needs maintainable code boundaries; the latter is an asset packaging decision. Neither requires drawing the entire world as one enormous image. Keep source artwork editable and identifiable, then pack runtime atlases when a measured need or a chosen pipeline justifies them.

The [environment pilot](../../../assets/embervale/environment-pilot/README.md)
adds static water, grass/earth/stone/dirt riverbank quarters, stone pavers and
copper/tin rocks with shared depleted rubble. Ground persists beneath objects,
preventing grass squares on the stone mining pad. See the
[environment integration record](../06-development/ENVIRONMENT_RENDERING_2026-11-19.md)
for editable sources and actual river, workshop and depleted mining captures.
The [service pilot](../../../assets/embervale/service-pilot/README.md) adds
bank and crafting-table sprites, editable masters and opaque-pixel targeting
through their existing station actions. The first four batches share a generated
layout and texture cache: 86 frames in twelve PNGs. See the
[service integration record](../06-development/SERVICE_RENDERING_2026-11-19.md).
The [building kit](../../../assets/embervale/building-pilot/README.md) adds 31
overhead wall/window/door/corner, roof/ridge and floor pieces, assembled into the
first workshop. Authored walls block movement; an open doorway preserves access,
and the roof fades around the player. The five batches total 117 frames in
fifteen PNGs. See the [building integration record](../06-development/BUILDING_RENDERING_2026-11-19.md).
Further building assemblies, other ore tiers, remaining cast and full starter
visual acceptance remain open.

The [variation kit](../../../assets/embervale/variation-pilot/README.md) adds
658 native frames in eleven masters. Landscape fields use actual settlement,
path, water and tree positions to place coherent grass patches, varying dirt,
shallow/deep water, stone conditions, meadow flower clusters, grass/reeds and
leaf litter. Quiet woodland accumulates leaves beneath canopies and at stumps;
maintained approaches stay clear. Building surfaces support new, weathered and
repaired conditions and terracotta/slate roofing, with localized workshop repairs.
Normal trees and copper/tin/rubble also have silhouette variants. All six batches
total 775 frames in 26 cached PNGs. See the [contextual variation record](../06-development/WORLD_VARIATION_2026-11-19.md).
These conditions are authored presentation, not simulated decay or seasons.
The [wall orientation correction](../06-development/WALL_ORIENTATION_2026-11-19.md) replaces front-facing side panels with aligned wall caps/windows and corner joins, adds interior front cutaways and aligns the south roof eave.

## 2. Proposed visual direction

Use the existing tile scale as the first compatibility reference. A restrained pixel-art direction can grow from the current simple silhouettes: readable ground, distinct characters, clear resources and strong contrast between the terrain and important interactions. This is a proposed direction, not a final approved style bible.

| Element | Initial production rule | Review question |
|---|---|---|
| Ground | Quiet texture, limited contrast, consistent tile edges | Can the player still locate an item or cursor on it? |
| Resources | Distinct silhouettes and visible depleted states | Can each tier and state be recognised at normal zoom? |
| Characters | Consistent scale and ground contact point | Does the sprite appear to stand on the selected tile? |
| Buildings | Separate roof, walls, doors and collision meaning | Does the image imply an entrance that actually works? |
| Interactables | A readable shape plus a label or contextual cue | Is usability dependent only on colour? |
| Threats | Signals distinguish passive, aggressive and elite encounters | Can danger be recognised before contact? |
| UI | Consistent spacing, text hierarchy and icon language | Does the game remain understandable without memorising icons? |

Create a short style board from approved assets once those first examples work. Record palette swatches, outline treatment, perspective, lighting direction, character proportions and animation cadence. The board should be a production reference, not a substitute for checking assets at actual gameplay scale.

## 3. Start with a small asset list

| Batch | Minimum useful contents | Gate before expanding |
|---|---|---|
| Terrain proof | Grass, dirt, stone, water and their required edge variants | Tiling is clean; collision remains independent of pixels |
| Resource proof | Normal tree, stump, copper rock and depleted rubble | Existing gathering states choose the right visual |
| Service proof | Bank marker, furnace and crafting table | Interaction approaches remain visible |
| Character proof | One friendly character and one monster | Pivot, facing, targeting and draw order behave correctly |
| Settlement proof | One small building kit, door and a few props | Roofs and scenery do not hide mandatory interactions |
| UI proof | A small consistent set of item and action icons | Inventory and recipe views remain legible |

Do not add a new material, monster rule or station type merely because an appealing image exists. Art should represent an explicit gameplay definition or a declared decorative role. If a sprite suggests a new mechanic, record that as a separate design proposal.

## 4. Asset identity and metadata

Each asset needs a stable identifier independent of its filename and atlas location. Moving a frame within an atlas should not invalidate a map's reference. A manifest should connect the authored ID to the source file, generated runtime file, frame geometry, pivot, intended usage and provenance.

The manifest and importer are planned work. The following field groups are requirements for that work, not a currently supported file schema.

| Field group | Required information | Why it matters |
|---|---|---|
| Identity | Stable asset ID, display name, category | References survive renaming and repacking |
| Source | Editable source path and source checksum | The asset can be reproduced and traced |
| Runtime image | Path, dimensions, colour/alpha expectations | Invalid imports fail before runtime |
| Frames | Rectangles, names, timing, directional mapping | Animation is explicit and inspectable |
| Placement | Pivot and visual offsets | Feet, trunks and objects align with tiles |
| Bounds | Visual rectangle, selection region, optional occlusion hints | Large artwork is distinguishable from gameplay occupancy |
| Provenance | Creator/source, licence or permission record, modification notes | Distribution rights can be reviewed |
| Build relationship | Importer version and generated-output relationship | Stale output can be detected |

A tree may have a canopy larger than one tile while its interaction footprint remains one tile. A building may cover many tiles while only selected doors are passable. Never derive either rule from transparent pixels without an explicit, validated design. The renderer can use visual bounds for culling; the authoritative simulation must use authored gameplay geometry.

## 5. Human and AI asset workflow

1. Write a short brief: intended use, dimensions, style reference, required states and how the asset will be selected in game.
2. Create or obtain the source artwork. Preserve the editable source and any permission record.
3. Inspect the actual pixels, including alpha, edge seams and silhouette at normal zoom. A concept image is not automatically a usable sprite sheet.
4. Assign an asset ID and import metadata. Validate dimensions, frame rectangles, file limits and references.
5. Preview against representative terrain with the real camera scale and selection overlays.
6. Play the associated action: gather, deplete, respawn, talk, attack, move or use a station.
7. Commit the source, metadata and appropriate generated output according to the chosen repository policy, with a small before/after preview.

AI assistance can help with briefs, concept variations, placeholder art, metadata and consistency checks. Generated images still need human review and technical import validation. Check that frames share scale and pivot, that directional views agree, and that the image has not invented fine details that become noise at gameplay size. Do not assume a single generated sheet has correct frame boundaries or consistent animation.

Atlas packing is a build concern. Keep canonical IDs and source metadata stable if packing changes. The importer should report which asset and frame failed, not just that a texture could not load. Missing or invalid art should have a visible diagnostic fallback during development; release validation should fail for required missing assets.

## 6. Render integration and performance

Extract reusable presentation data from the existing renderer as needed for a concrete feature. Avoid creating a second unrelated interpretation of map colours, sprite selection or transforms in Studio. The core validator and CLI should remain usable without an SDL window; the preview backend can be chosen separately as long as its content interpretation is shared and its output is testable. See [architecture](../03-engine/ARCHITECTURE.md).

Record ordering rules for ground, low props, entities, tall props, roofs, effects and UI. Add a deliberately small overlap test: player behind a tree, player in front of it, adjacent NPCs, an item on the ground and a roof near an interaction. Do not promise full occlusion or roof fading until the renderer implements and tests it.

Measure texture memory, asset load time, visible entity counts and frame time on a recorded device. Choose atlas sizes and caching limits from that measurement rather than from a guessed universal budget. Keep developer debug overlays available when they explain a rendering or targeting problem.

## 7. Animation contract

**Owner acceptance, 20 November:** Animation is a working first pass. The owner is not satisfied with its appearance and deferred polish (ART-005). Melee/equipment and Mining are now reconciled and published (REL-004 complete). Automated correctness and research do not imply visual approval.

Start animation work with the [animation guide](ANIMATION_GUIDE.md) and [20 November research and findings](../06-development/ANIMATION_RESEARCH_2026-11-20.md). They record directional consistency, weapon attachments, pose/timing review, interruption rules and measured local polish results. The dated research distinguishes its separate local melee/polish prototype from published integrations; check the current source before reusing its helper paths or timing assumptions.

Animation communicates a simulated event; it must not become an alternative authority for its result. Combat damage, resource depletion and item production remain governed by gameplay rules. A presentation can begin, interpolate or finish between simulation updates without granting an extra hit or gathered item.

For each animated definition, record idle, movement, action and state-transition needs. Implement only the states the current game uses. Test interruption: movement during an interaction, a target disappearing, a player dying, a resource depleting and a camera zoom during the action. Select an explicit fallback frame for missing optional directions.

Include a static or reduced-motion presentation where necessary for readability. Avoid relying on repeated flashing to identify every important object. The animation review should include whether the player can see a target and understand an action while several effects overlap.

## 8. Action audio and remaining work

The [first action-sound pass](../06-development/ACTION_AUDIO_2026-11-20.md) is implemented on the current published source. Its [batch](../../../assets/embervale/audio-pilot/README.md) contains 15 original generated PCM effects with source, manifest, exact regeneration and provenance. Local contact, swing, hit/miss/block, successful gather/craft, accepted equipment, food and inventory actions have bounded feedback.

Settings provides independent effects volume and mute; F8 toggles mute and F9/F10 adjust volume. Controls apply to the current session. Six streams, cooldowns and bounded event routing prevent delayed or uncontrolled overlap. Missing/invalid optional clips or an unavailable device leave gameplay usable. Core remains SDL-free.

ART-004 remains in progress for physical-device audible quality and balance/repetition during the complete starter journey. Synchronization with the recovered melee implementation now passes Windows automated checks. Software/dummy checks do not establish audible quality. Preserve the committed-event, consumable contact/swing-edge and single copy-owner boundaries.

Music, ambient loops, footsteps, broader warnings and persistent client preferences can be considered after this pass is heard in real play. Add controls for those categories when they exist. Keep gameplay facts visible/textual as well as audible, and record source and distribution permission for any later third-party audio.

## 9. Acceptance checklist and maintenance

A new required visual asset is ready when it has a stable ID, valid metadata, traceable source, supported dimensions, correct pivot, appropriate state coverage, readable gameplay-scale presentation and no unintended collision change. Its preview must identify the game/content version used. If it changes a selection or interaction cue, include a short gameplay test.

Before a content release, check every required asset reference, atlas rectangle, animation frame and generated-output checksum. Review the licence inventory and include required attribution. Make sure debug assets and development-only items do not enter the normal content package accidentally.

Keep a small reference scene containing representative ground, water, trees, stations, friendly NPCs, enemies and UI. Use it to compare a new art batch against the existing style, while still checking the assets in the actual River Crossing map. Update this guide when the adopted format, style rules or production tools change; keep unselected options in the decision log rather than presenting them as established pipeline behaviour.

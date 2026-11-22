# Custom characters: design and implementation plan

**Updated:** 22 November 2026.  
**Owner direction:** Use PixelLab for artwork and develop custom characters from a reusable base. The owner has identified that the first result needs improvement; no finished visual approval is recorded.  
**Status:** Production plan and retained art proof. Character creation, appearance persistence and a general layered character renderer are not implemented by this document.  
**Parent work:** ART-006, ART-007, ENG-007, GAME-011, GAME-012 and QA-005 in the [canonical backlog](../02-roadmap/BACKLOG.md).

Start with the [pilot review and improvement register](../06-development/CUSTOM_CHARACTER_REVIEW_2026-11-22.md), [asset entry point](../../../assets/embervale/custom-character-pilot/README.md) and [production workflow](CUSTOM_CHARACTER_WORKFLOW.md). [ADR-021](../07-decisions/ADR_021_MODULAR_CHARACTER_APPEARANCE.md) records the architectural direction.

## 1. What this should give the game

A player chooses a coherent appearance, sees that same person while moving and using equipment, and gets the same appearance back after save/reload. Contributors can add a compatible hairstyle, garment or material without redrawing every other combination.

The first useful game delivery is one human body family, a small set of skin/hair/clothing choices, eight-way idle/walk, consistent equipped/action presentation and safe persistence. A huge wardrobe, arbitrary body morphing, races, cosmetic commerce and a general skeletal animation editor are separate investments.

The same compatible profiles can later define authored NPC appearances. Player choices belong to the player descriptor/save; NPC looks belong to authored content definitions. Preserve Mara and other established identities while adding that adapter deliberately. Creatures with a different anatomy need their own body/rig family, rather than being forced through the first human fit set.

| Addition | New authored work | Reused work |
|---|---|---|
| Another skin tone | Reviewed skin palette on a semantic skin region | Compatible body poses, identity and timing |
| Another colour of the same tunic | Cloth ramp and dye mapping | Tunic silhouette, masks and frames |
| A differently shaped shirt | Garment frames/coverage for supported clips and facings | Compatible body motion, palette/export system |
| A new hairstyle | Front/rear hair and necessary frame variants, head alignment and hat rules | Body motion and compatible head track |
| A new sword material | Material mapping, when construction is unchanged | Sword shape, grip and compatible shared motion |
| A bulky coat or long skirt | New silhouettes, overlap and secondary-motion decisions | Compatible rig and infrastructure; artwork is not free |
| A substantially different body shape | Its own body/rig measurements and garment fit set | Catalogue, tools and validation, where compatible |

Colours do not create new body animations. Different garment shapes still need appropriate frames. A shared rig is a compatibility contract, not a promise that any image will fit any body.

## 2. Starting point and source boundaries

The default-branch runtime inspected for this plan is [f2b5c6b](https://github.com/Adzffs/GameEngine/commit/f2b5c6b210654040e449ffe8101f32052a93d980). It has the existing player/action renderer, 32 runtime textures and save version 13. The new pilot is deliberately outside its runtime asset catalogue.

The later [walking and equipment candidate](https://github.com/Adzffs/GameEngine/tree/2c7b9155ec7dc561046aeb0613a88375041f6393) is on a separate branch. Its [nine-slot locomotion contract](https://github.com/Adzffs/GameEngine/blob/2c7b9155ec7dc561046aeb0613a88375041f6393/docs/embervale/06-development/PIXELLAB_WALKING_2026-11-21.md), [modular equipment plan](https://github.com/Adzffs/GameEngine/blob/2c7b9155ec7dc561046aeb0613a88375041f6393/docs/embervale/05-game/MODULAR_EQUIPMENT_ANIMATION_PLAN.md) and [rejected axe masking correction](https://github.com/Adzffs/GameEngine/blob/2c7b9155ec7dc561046aeb0613a88375041f6393/docs/embervale/06-development/MODULAR_AXE_OCCLUSION_FIX_2026-11-22.md) remain authoritative for that candidate. Adding this plan does not merge its runtime changes.

Before implementation, record the actual chosen checkout and reconcile these lines. The default runtime uses eight walking/chopping poses, six Mining poses and sixteen equipped walking/attack slots. The later walking candidate has a separate idle plus eight moving slots. The pilot has four south idle frames and eight south walking frames; none can be substituted by assuming matching array indices.

### Existing foundations to retain

- [ForestSprites](../../../src/Graphics/ForestSprites.h) already has independent body, weapon and shield rendering, ground-pivot placement, cached textures and fallback.
- [DrawEquippedPlayer and gathering rendering](../../../src/Graphics/ForestSprites.cpp) use known equipment styles and item mappings. They are not a general hair/clothing catalogue.
- [Player](../../../src/Player/Player.h) has authoritative equipment and gameplay state. The inspected definition has no general appearance descriptor.
- [Animation timing](../../../src/Animation/PlayerAnimationTiming.h), [animation guidance](ANIMATION_GUIDE.md) and committed action events remain the source of presentation timing/authority.
- [Save compatibility](../03-engine/SAVE_COMPATIBILITY.md), temporary-play isolation and existing failure-safe saves must survive appearance changes.
- [Asset limits](ASSET_RESOURCE_LIMITS.md) and the existing single packaging/cache owner still apply.

### What the pilot actually proves

Eight standing body views; one south idle and walk; separate tunic/trouser overlays for those twelve moving/idle cells; exact source-pixel conservation; exact garment composition; and palette examples in an offline HTML preview. It proves a narrow reusable-layer approach. It does not establish final anatomy, fabric quality, eight-way animated clothes, equipment attachments, game integration or owner acceptance.

## 3. First decisions: improve the base before multiplying it

Use a neutral adult underwear base with no baked hair, armour or tools. Keep it useful as an art mannequin; do not bake a single player class, profession or outfit into its body.

Resolve these decisions in a small in-world comparison:

| Decision | Required evidence | Current disposition |
|---|---|---|
| Body height and width | Same camera, tile, doorway, Mara, tree and equipped examples | Open; pilot body is taller than the existing character |
| Proportions and silhouette | Front/side/rear/diagonal turntable at native and gameplay scales | Open; current build reads muscular and is not an accepted universal body |
| Camera and lighting | Head/shoulder/foot perspective and one lighting convention across eight views | Open visual review |
| Underwear and skin ramp | All shades readable on grass, stone and indoor floors; modest neutral default | Pilot only |
| Baseline body family | One reviewed human fit set, with explicit future family IDs | Proposed first scope |
| Motion and rhythm | Feet, stride, idle-to-walk, loop seam, start/stop and turns in the renderer | South-only generated evidence; game review pending |

The 80×80 pilot cell is an export container, not the intended world height. Its pivot is (40,70); the existing walking candidate uses 64×64 and (32,48). Do not shrink, stretch or offset artwork by trial until it appears to fit. Choose an explicit pixel-density/physical-size contract, regenerate at the intended native scale if necessary, then pack without losing visible pixels.

Freeze a reviewed base revision before producing a wardrobe. Keep the original v1 as evidence even if replaced.

## 4. Appearance model

Introduce a bounded value-only appearance descriptor, separate from inventory/equipment. Stable catalogue IDs select reviewed content. Raw paths, arbitrary URLs and arbitrary image uploads are outside the first implementation.

Illustrative future descriptor; these identifiers and this schema are **proposed**, not currently loadable:

~~~json
{
  "schema": "embervale.character-appearance",
  "version": 1,
  "body": "human.base-a.v1",
  "skin_palette": "skin.warm-03",
  "hair": "hair.short-01",
  "hair_palette": "hair.chestnut-02",
  "facial_hair": "none",
  "base_top": "clothes.linen-tunic-01",
  "top_palette": "cloth.teal-02",
  "base_bottom": "clothes.work-trousers-01",
  "bottom_palette": "cloth.brown-01"
}
~~~

Use validated IDs and deterministic defaults. Keep display names/localisation separate from durable identity. Body families can eventually have different silhouettes without attaching hairstyles or clothing choices to gender labels.

Equipped items continue to come from the existing authoritative Equipment state. An appearance request must not create a sword, grant a shield, change defence, bypass requirements or equip a cosmetic item as gameplay equipment. Define the visual precedence between ordinary clothing and real armour explicitly. Do not silently add cosmetic transmog or hide gameplay-relevant gear.

## 5. Catalogue and compatibility contract

The offline catalogue should resolve a descriptor into bounded compiled presentation definitions. Start with the existing generated-binding approach; a new runtime JSON parser is not required for the first slice.

| Record | Owns | Essential checks |
|---|---|---|
| Body profile | Stable ID, revision, rig family, proportions, semantic skin regions and supported motion sets | Unique ID, valid references and declared coverage |
| Motion set | Action/family, directions, pose keys, cycle/timing and interrupt policy | Complete declared frames and one shared clock |
| Layer definition | Slot, fit family, image cells, palette regions, coverage/occlusion and fallback | Compatible rig/motion revision and complete declared cells |
| Attachment track | Head/grip/joint positions, tool view and per-pose ordering | Bounded coordinates, same anatomical hand and known pose keys |
| Material palette | Named regions and ordered shade ramps | Complete mapping; unrelated regions unchanged |
| Export record | Master/input hashes, trim rectangles, adjusted pivots, frame durations and resource totals | Deterministic output and preserved original sources |
| Appearance definition | Selected content IDs | Valid combinations and stable defaults |

Use explicit keys such as a body/motion revision plus action, direction and pose. Two eight-frame strips are not compatible merely because both have eight columns. Record which layer/motion combinations are supported; do not guess using filenames.

Changing a rig, limb position, silhouette or pose sequence invalidates affected garment/attachment compatibility and requires a new revision or explicit revalidation.

## 6. Layering, occlusion and palettes

A useful initial slot set is skin/body, rear/front hair, facial hair, base trousers, base top, footwear, armour/helmet and equipment. Not every slot needs content immediately.

Resolve body pose and facing **once** per actor presentation frame. Every compatible layer consumes that selection and its ground placement. Layers must not independently advance clocks or choose frames.

The general draw sequence is pose-dependent: rear hair/cape/equipment, body and ordinary garments, armour, visible hair/face parts, foreground equipment/grip hands, then effects where appropriate. It is not one global universal order. Helmets may hide hair regions; long sleeves may cover forearms; shields may cross torso boundaries. Use authored coverage masks and front/rear segments where needed.

The current outfit is an in-silhouette recolour/mask demonstration. Its “no garment pixels outside body” check applies to that pilot, not to coats, shoulder armour or flowing hair. Such assets may deliberately widen the outline, within declared frame bounds. Validate their intended silhouette, clipping and overlap instead.

Production skin, cloth, leather, metal, eyes and hair must have semantic region/palette definitions. The preview's warm-colour heuristic is not a production skin mask. A skin swap must leave eyes, underwear, tools and garments unchanged. A cloth dye must leave skin and metal unchanged.

Start with prebuilt independent layer/palette variants or a bounded cache at load time. Count their texture cost. Avoid a precomposited atlas for every body × hair × top × bottom × dye × weapon combination. A future shader palette path is optional and needs its own evidence.

## 7. Animation and equipment rules

Follow [ANIMATION_GUIDE](ANIMATION_GUIDE.md). Existing world ticks, movement interpolation, gathering rolls, committed combat results and once-only audio edges remain unchanged.

- Walking advances by actual visual distance. The retained pilot's 600-ms cycle follows the later walking candidate's one-tile review convention; default master has a different authored timing reference. Reconcile before integration.
- Idle uses render time only while the appropriate stationary state is active. It must not hold stale movement or block an accepted action.
- Direction changes retain anatomical handedness and a coherent phase. Do not manufacture missing asymmetrical diagonals by mirroring.
- Clothing/hair reuse the body's chosen pose. New frames are needed where their own shape changes.
- Each equipped/grip family can share body motion across compatible material variants. A heavy two-handed weapon can need a different family.
- Author hand positions, aim/view and body/equipment ordering for the improved body. Old wrist coordinates must not be copied to a different physique.
- Preserve actual contact/event timing for hit, miss, death, mining and woodcutting. A visual swing does not grant damage, items or XP.

The earlier axe failure is a required regression lesson: covering a handle with torso/head pixels did not repair a shaft passing through the body. Validate the unmasked path, both grips, clearance and cutting-edge direction, then inspect final occlusion. Do not repeat a mask-only “fix.”

Before a custom character is the ordinary playable default, either provide consistent matching presentation for the actions that can be entered or explicitly constrain an isolated preview. Do not ship a character that changes body/outfit whenever mining or attacking begins.

## 8. Rendering integration and fallback

Add a focused appearance-resolution boundary around existing rendering, rather than rewriting World or Graphics wholesale. Suggested responsibilities: validate/resolve appearance IDs, choose a compatible motion/layer pack, produce a draw list, and let the existing renderer/cache draw it.

The first engine slice should be a developer-selectable, no-save scene/fixture using one body and one outfit. Check front idle/walk against the world before making it the default.

Retain primitive mode, existing complete actor presentation and per-image error handling. For missing optional hair, a declared “none” fallback can be safe. For a missing essential compatible body/action pack, fall back to one complete known presentation, with a diagnostic, rather than mixing an old naked body, new clothes and unrelated weapon track.

Keep fallback policy deterministic and visible in verification. Broken optional metadata must never make an actor disappear or change simulation state. Draw/sort at the authoritative ground point, not at the top of a larger sprite. Collision footprint, reach, movement speed and roof/building semantics remain gameplay-owned.

## 9. Saving and character creation

Plan a small appearance-data version independently of art revisions. Current player saves are version 13; adding appearance requires a deliberate migration at implementation time. Use the next appropriate save version then, after inspecting intervening work. This planning update changes no save format.

Old saves get a deterministic default appearance without losing inventory, crafting escrow, quests, skills or position. Unknown/retired IDs map to documented safe defaults or explicit aliases. Reject malformed/oversized fields before publishing loaded state. Failed writes preserve the last valid save and the live appearance.

Use a preview draft for character creation/change:

1. Start from an existing valid appearance or a deterministic new-player default.
2. Preview compatible options and turn/walk without mutating the live player.
3. Apply validates the whole descriptor and commits once; Cancel/Escape restores the original.
4. Persistent sessions use the existing lifecycle, autosave and retry policy. Temporary/no-save playtests remain temporary.
5. After save/reload, the same valid IDs resolve to the same appearance family.

Offer named choices and labelled colour swatches, rotate/idle/walk controls, keyboard navigation, readable scale and visible focus. Do not force a long creator before the first playable action; provide a sensible default. A future makeover service or creation-screen location is a product decision, not implemented content.

## 10. Production workflow and costs

Use the [workflow guide](CUSTOM_CHARACTER_WORKFLOW.md) for exact checkpoints, briefs and file ownership. Generate a missing reusable body motion once per supported family/facing. Clean up and extract compatible clothing separately. Recolour materials without new AI generation. PixelLab output is source art; outfit transfer does not promise a ready-to-use independent wardrobe layer.

The dated pilot used four generations: two for eight base views, one for south idle and one for south walk. The recorded remaining balance was 4.75. That is a historical receipt, not a live budget or an instruction to spend it. This documentation/publication task requests no further generations, credit purchases or subscription changes.

At the pilot's quoted one-generation-per-direction walk price, seven additional walk directions would require seven generations; the recorded 4.75 was insufficient. Prices, canvas padding and account limits must be rechecked before actual production. Reuse completed groups and count returned frames; do not pay twice because a download or export failed.

### Resource planning

Account for stored per-layer frames and palette variants, not only source-file size. An illustrative idle+walk set of 12 poses × 8 directions × 3 layers × 80×80 pixels uses 7,372,800 RGBA bytes (about 7.03 MiB), plus 1,843,200 retained alpha bytes (about 1.76 MiB). This excludes separate standing diagnostics, hair, equipment, driver/decoder overhead and additional palette copies. It is a planning example, not a measured runtime integration.

Trim transparent space losslessly and adjust pivots together. Share motion, references and source art, but do not claim constant runtime memory as the wardrobe grows. Run the existing resource checks before runtime catalogue/packaging changes; do not loosen budgets merely to fit oversized editing canvases.

## 11. Ordered delivery plan

| Stage | Parent work | Concrete delivery | Exit evidence |
|---|---|---|---|
| 0 — Preserve and plan | ART-006 | Publish v1 sources, masks, previews, findings and canonical links | Checksums, usable entry point and honest coverage |
| 1 — Improve the body | ART-006 / ART-005 | Reviewed proportions, camera, palette, underwear and world scale | Native/gameplay comparisons; owner visual feedback recorded |
| 2 — Prove one composition | ART-006 / ENG-007 | Developer-only in-world front idle/walk with separate layers | Ground contact, layer isolation and transitions in the real renderer |
| 3 — Complete locomotion | ART-007 / ENG-007 | Eight-way body, matching ordinary clothing, head/hand tracks and explicit unsupported states | Every declared facing/pose; no fabricated coverage |
| 4 — Add limited choice | ART-007 | Small hairstyle/facial-hair set and independent material palettes | Fit/hat/skin isolation and combination matrix |
| 5 — Match gameplay actions | ART-007 / ENG-007 / ART-005 | Carry, chop, mine, melee and appropriate hurt/death/recovery presentation | Weapon/shield/grip/occlusion and interruption fixtures |
| 6 — Persist appearance | GAME-011 | Bounded descriptor, defaulting, migration and safe round trips | Old/new/malformed saves; failure preservation; no-save isolation |
| 7 — Provide player controls | GAME-012 | Apply/cancel creator or appearance panel with defaults and keyboard operation | Live/persisted change only on Apply; readable controls |
| 8 — Accept the feature | QA-005 | Real starter use with representative appearances and resources | Recorded visual, gameplay, compatibility and performance results |

The stages describe dependencies, not calendar promises. Stage 0 is a bounded repository/documentation delivery; the parent feature stays in progress. Revisit the plan after each visible proof. Do not mass-generate all actions or variants before the base/fit contract is settled.

### Acceptance matrix

| Scenario | Required result |
|---|---|
| Turn through eight facings | Same person, proportions, palette and stable ground contact |
| Idle → walk → stop → turn | Shared body/layer phase; no pop, slide or independent drift |
| Change skin or tunic colour | Only the named region/layer changes; source animation remains the same |
| Change a garment or hairstyle | Compatible fit and complete declared coverage; honest fallback otherwise |
| Equip sword/shield or two-handed item | Authoritative gear, correct hands and coherent front/rear order |
| Chop/mine, cancel, re-target, exhaust a node | Attached tools, correct edge/point and committed reward/audio rules |
| Hit/miss, lethal hit, death/respawn/recovery | Actual world events preserved; no duplicate effects or stale appearance |
| Save, reload, load an older save | Valid appearance restored/defaulted without losing gameplay state |
| Apply, Cancel, focus loss, Escape | Predictable draft/live distinction and existing modal priority |
| Missing image/palette/ID | Diagnosed bounded fallback; no invisible player or arbitrary file load |
| Multiple actors at normal zoom | Bounded cached work and recorded memory/frame-time impact |
| Export after an art edit | Masters preserved; geometry, pivots, hashes and resource checks agree |

## 12. Completion and maintenance

Keep one canonical backlog and this owning design. The dated review owns observations and evidence; the workflow owns production recipes; the asset folder owns immutable v1 bytes and future explicit revisions. Do not describe a generated PNG, a passing image check or this plan as a finished character creator.

When an improvement lands, update its finding, the parent backlog status/notes, current status, relevant guidance and verification in the same change. Record native renderer captures separately from an authoring GIF or browser preview. Mark owner visual acceptance only after actual feedback on that version.

**Next concrete implementation:** compare the proposed base beside the current player in a temporary world-renderer fixture, settle native size/proportions, then bind one south-facing body/tunic/trouser composition. Preserve the current playable default until that proof is coherent.


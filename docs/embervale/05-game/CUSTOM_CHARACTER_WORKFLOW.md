# Custom character production workflow

**Tunic-fit lesson, 22 November:** The owner rejected the v1 tunic fit. The [v2 correction](../06-development/TUNIC_FIT_2026-11-22.md) uses actual per-pose shoulder/neck/sleeve/hip landmarks and independent cloth shading. Its coverage and protected-hand checks reject the old tunic in all twelve poses. A waistband-derived skin recolour and an inside-body-alpha pass are insufficient garment acceptance. Use the [revised source/recipe](../../../assets/embervale/custom-character-pilot/revisions/tunic-fit-v2/README.md); preserve v1 and retain owner review as pending.

**Updated:** 22 November 2026.  
**Scope:** PixelLab source artwork, reusable body motions, separate clothing/hair/material layers and checked integration into Embervale.  
**Owner:** [Custom character plan](CUSTOM_CHARACTERS.md); current evidence is in the [pilot review](../06-development/CUSTOM_CHARACTER_REVIEW_2026-11-22.md). Runtime support described below is planned unless its implementation is linked.

## 1. Start from a small accepted fit set

A fit set is a body/rig revision, camera, native pixel density, ground pivot, pose-key sequence and the layers authored for it. It is more specific than “64-pixel character.”

Before generating, record the chosen repository commit, actual runtime branch, visual goal, action, facing, anatomy/handedness, equipment family and acceptance fixture. Read [animation guidance](ANIMATION_GUIDE.md), the current action implementation and [resource limits](ASSET_RESOURCE_LIMITS.md).

The first v1 body is a reference, not the approved fit set. [Its sources](../../../assets/embervale/custom-character-pilot/v1/README.md) are preserved. Settle its proportions, real world size, clothing appearance and motion before expanding it.

## 2. Keep source, decisions and exports separate

Recommended future batch structure:

~~~text
custom-character-pilot/
  README.md
  v1/                       retained original proof; unchanged
  revisions/<revision>/
    source/                 original service exports and editable masters
    recipes/                prompts, masks, palette regions and attachment tracks
    manifests/              profiles, clips, compatibility and provenance
    exports/                packed PNGs and generated runtime definitions
    previews/               turntables, layer isolation and timed comparisons
    reports/                commands, costs, hashes and actual results
~~~

This is a proposed organisation for future revisions. The published v1 directory already contains its original source/layer images, manifests, decision files, scripts and preview.

Treat source artwork and authored masks/tracks as production inputs. Exporting must not silently overwrite a manually edited master or regenerate paid artwork. Use explicit new-revision/import actions for that. Store original service output even when rejected, with a reason and a link to the replacement.

PNG masters are retained for v1; no layered LibreSprite master is claimed. A production revision should introduce a proper editable layered source and a normal export path that preserves it.

## 3. Base character brief

A useful brief names:

- Adult body family and intended proportions, with reference to the agreed scale fixture.
- Bald head, neutral expression, plain underwear and no baked gear/hair.
- Camera/projection, lighting direction, outline and shading/detail level.
- Bare hands/feet and a silhouette that leaves readable space for clothing and grips.
- Transparent background and sufficient margin.
- Actual intended visible height, distinguished from canvas dimensions.

Do not use “match the game” as a substitute for explicit settings and a visual comparison. The first generator returned a larger visible body than the requested target; size words are guidance, not measured output.

Generate one coherent set of standing views, inspect every facing, and retain its character ID. Compare head/torso/leg proportions and face identity at native scale. Asymmetrical equipment or hair requires authored directions; a mirrored opposite view is not automatically valid.

## 4. Motion brief and generation checkpoint

Generate the body motion first with no tools or clothing beyond underwear. The pilot used PixelLab V3 for its four-frame south idle and PixMiniMax's Walking template for its eight-frame south walk.

For idle, ask for planted feet, subtle chest/shoulder motion, no drift or new objects, and a seamless loop. For walking, choose a readable complete gait; review arm/leg opposition, stride, weight and loop seam before adding views. A named template can impose its own motion settings, so verify the actual API/website options.

Production sequence:

1. Check the actual account balance, concurrency and current price.
2. Inspect or list existing character/animation groups before requesting work.
3. Generate only the missing selected action/direction.
4. Wait for the real job result; capture returned group ID, stored frame count and stored canvas.
5. Inspect every frame and the timed loop, not just the first preview.
6. Keep or replace the candidate for a documented reason.
7. Expand to other directions only after the base/motion/fit decision is sound.

A validation error or download failure is not a reason to create another paid character. Reuse the existing job/group when supported. Confirm whether an animation contains a separate reference frame: eight generated poses and nine stored images are different contracts.

## 5. Get the original pixels

Use original PNG exports or a verified lossless service sheet, not a screenshot. Some PixelLab workbench inline images include labels, a matte and a frame grid; those are review images, not sprite source.

For this proof, sheets were retained losslessly in PixelLab's gallery and retrieved as actual PNGs. The raw source files in v1 have the declared dimensions and alpha. The workbench's annotated inline preview was not used as source artwork.

On import, verify:

| Property | Required check |
|---|---|
| Dimensions | Actual decoded width/height and per-cell layout agree with metadata |
| Alpha | Transparent source pixels remain transparent; no preview matte |
| Completeness | All declared frames/directions exist; no duplicate/missing names |
| Pixels | Export/repacking preserves visible RGBA exactly |
| Padding and trim | Only declared empty margins may be removed; pivots move with crops |
| Limits | Bound encoded/decoded bytes, dimensions, counts and rectangles |
| Timing | Record authored holds once; do not infer them from a GIF playback default |

Do not trust file extensions, a successful download or visual appearance alone. Keep original binary bytes and checksums beside the interpreted cells.

## 6. Build garments as separate authored layers

First prove one direction and one simple outfit. The v1 example uses a sleeveless tunic and knee-length trousers on the same twelve body frames. PixelLab's exact mask/colour tools produced transparent outfit-only frames; a separate dressed result verified the composition.

The retained decisions have one-based frame numbers and original 84×84 coordinates. The packaged animation atlases remove two pixels of empty padding per side. Therefore a recipe coordinate is not directly an 80×80 runtime coordinate; apply the recorded transform.

### Current proof versus production extraction

The v1 polygons are derived from the existing body's waistband and manually chosen fitting regions. They inherit body shading and stay within the body silhouette. They are evidence that layers can be independent, not a finished generic garment generator.

For production:

1. Mark semantic body regions and protected face/hands/feet on the exact source poses.
2. Author a garment shape/folds/neckline/hem appropriate to cloth or armour.
3. Establish per-frame coverage, including where the base body is hidden.
4. Isolate the garment onto transparency; retain separate material regions.
5. Compare body-only, garment-only and composite on one shared clock.
6. Inspect shoulders, elbows, hips, knees, hands and hem through the entire loop.
7. Test removing the garment and changing only its palette.
8. Register the exact body/motion revision it fits.

An AI outfit transfer can supply dressed source frames, but does not guarantee separate clothing layers or pixel-identical alignment. Extra extraction/cleanup still needs review. Do not present a newly generated dressed body as a reusable garment until the body/layer separation is proven.

For a loose sleeve, coat or skirt, allow its intended extra silhouette and secondary motion. The v1 inside-body alpha assertion must be replaced with a declared shape/coverage check for those assets, not disabled without an alternative.

## 7. Hair, facial hair and hats

Start with a very small set after the head/body contract is stable. Split rear/front hair where it passes behind shoulders or in front of the face. Beards must follow the face in side/rear diagonals; a fixed front sticker is insufficient.

Record head anchors or exact frame variants. Decide which hair regions helmets hide, how long hair behaves with cloaks, and which combinations are unsupported. Keep hair colour independent from skin and cloth. Test a dark and light hairstyle on every skin palette at gameplay zoom.

Do not promise all body families can share hair/garments without a fit review. New head proportions can require different artwork even if the body canvas is unchanged.

## 8. Equipment and overlap

Keep held items separate from clothes and body. Generate/author one compatible tool shape and its required views, define grip and secondary grip if needed, and reuse material ramps across bronze/iron/steel where the construction matches.

Author hand, aim and ordering tracks for the chosen body. Test empty hands, sword, shield, two-handed weapon, axe and pickaxe where those actions exist. Preserve the authoritative equipped items and requirements.

The [earlier axe defect](https://github.com/Adzffs/GameEngine/blob/2c7b9155ec7dc561046aeb0613a88375041f6393/docs/embervale/06-development/MODULAR_AXE_OCCLUSION_FIX_2026-11-22.md) must stay in the review checklist: validate the unmasked shaft path and both grips before final front/rear compositing. Restoring torso/head pixels over a bad shaft path cannot establish a physical fix.

Review tool edge/point direction, face clearance, sleeve/hand overlap, offhand placement, shoulder reach and the transition into the next action. Equipment fit does not change collision, range or damage.

## 9. Recolour without creating more animations

Use named shade ramps on semantic regions. The first tunic has six shades; changing the six cloth entries can create a colour variation without any new body or garment motion.

Keep skin, eyes, underwear, cloth, leather, metal and hair separate. A global colour tint on the dressed sprite is unsuitable. The current preview's skin map is an illustrative warm-colour heuristic; replace it with authored semantic regions before a production catalogue.

Palette variants are cheap to author but still consume runtime resources if stored as separate textures. Reuse independent layers and apply a bounded cache or deliberate compiled variants; measure it rather than baking every outfit combination.

## 10. Export and integrate through the existing boundaries

The first pipeline implementation should:

- Validate IDs, compatibility, cell geometry, coverage and masks offline.
- Produce deterministic PNGs, adjusted per-frame pivots and generated presentation definitions.
- Preserve authored timings and generate preview/runtime timing from one source.
- Run existing bounded art validation before packaging.
- Register only selected runtime images with the existing cache and packaging owner.
- Keep reference/raw/rejected/HTML/GIF material outside the runtime image catalogue.

The v1 archive's builder is a historical packaging utility. It has Windows font paths, project-specific assumptions and evidence fields that a rebuild can replace; it is not the approved portable production exporter. The improvement plan includes a portable, source-preserving successor and separate retained verification records.

Use the real renderer for the first integration proof. Compare scale and ground contact beside known world objects, then movement, turns, clothing swaps and equipment/actions. A browser canvas or authoring GIF cannot substitute for those checks.

## 11. Required handoff

Each candidate needs:

| Field | Record |
|---|---|
| Identity | Repository commit/branch, body/rig/motion/layer revisions |
| Service | PixelLab character/group/job IDs, model/settings and dated receipt |
| Inputs | Original PNG/ZIP bytes, editable masters, masks/tracks/palette definitions |
| Geometry | Source canvas, visible bounds, crop/packing transform and ground pivot |
| Coverage | Exact action/direction/pose matrix, including unsupported entries |
| Timing | Authored holds/distance convention, reference-frame policy |
| Costs | Actual generations charged; prospective work quoted separately |
| Rights | Creator/service/source and terms/licence decision; no invented licence |
| Evidence | Source/layer checks, timed authoring review, real renderer results and owner comments |
| Limitations | Missing facings/actions/variants, fallback and next concrete task |

Never include credentials, bearer tokens or expiring signed download URLs in the repository. Durable service IDs plus retained local source bytes are sufficient for this proof.

## 12. Practical next session

Open the [review findings](../06-development/CUSTOM_CHARACTER_REVIEW_2026-11-22.md). Make the in-world size/proportion comparison first, with no player save. Record a chosen fit revision and the owner's specific visual feedback. Then build one front-facing layered runtime proof. Expand the wardrobe and other directions from that evidence, rather than spending generations on an unsettled body.

# Custom character pilot

**Status:** Retained art/customisation proof, not a runtime world-art batch.  
**Created:** 22 November 2026.  
**Owner feedback:** Improvements are required; finished visual acceptance is open.

Start with the [custom character plan](../../../docs/embervale/05-game/CUSTOM_CHARACTERS.md), [improvement register](../../../docs/embervale/06-development/CUSTOM_CHARACTER_REVIEW_2026-11-22.md) and [production workflow](../../../docs/embervale/05-game/CUSTOM_CHARACTER_WORKFLOW.md).

## Latest revision: trouser fit v3

The owner requested the same fitting treatment for the pants. [V3](revisions/trouser-fit-v3/README.md) adds per-pose hip, crotch, leg and cuff geometry with independent brown cloth shading. The latest combined outfit reuses the original body and unchanged v2 tunic. All twelve south poses pass coverage and protected-hand/lower-leg checks; owner visual review remains open. See the [correction record](../../../docs/embervale/06-development/TROUSER_FIT_2026-11-22.md).

![Previous and revised trousers](revisions/trouser-fit-v3/previews/before-after.png)

[Moving comparison](revisions/trouser-fit-v3/previews/before-after-walk.gif) · [All poses](revisions/trouser-fit-v3/previews/all-poses.png) · [Interactive revised preview](revisions/trouser-fit-v3/character-workshop.html)

## Earlier revision: tunic fit v2

The owner reported that the original tunic did not fit. The [v2 revision](revisions/tunic-fit-v2/README.md) supplies proper shoulder/sleeve/neckline/hip geometry and cloth shading across all twelve existing south idle/walk poses. The body and trousers are unchanged; owner review of the new fit remains open. See the [defect and correction record](../../../docs/embervale/06-development/TUNIC_FIT_2026-11-22.md).

![Previous tunic and revised fit](revisions/tunic-fit-v2/previews/before-after.png)

[Moving comparison](revisions/tunic-fit-v2/previews/before-after-walk.gif) · [Interactive revised preview](revisions/tunic-fit-v2/character-workshop.html)

## Original v1 reference

The faulty garment is retained below as historical source evidence.

![Base body, separate original outfit and combined result](v1/previews/character-layer-preview.png)

## Inspect the first proof

The immutable [v1 source pack](v1/README.md) contains:

- Eight original 64×64 standing source views.
- Four south idle and eight south walk source poses, stored by PixelLab at 84×84.
- Separate tunic/trouser layers and a dressed reference on 80×80 packed cells.
- Original sources, exact mask/colour decisions, palette examples and hashes.
- A [timed layer comparison](v1/previews/character-layer-preview.gif).
- A self-contained [character-workshop.html](v1/character-workshop.html); download/check out the file and open it locally. GitHub's file view does not execute it.

Use Idle or Walk facing South to inspect the clothing options. Other facings currently have standing body views only. Hair, beards, full animated coverage, equipment hand tracks, appearance saves and a game creator are future work.

## Keep the scope precise

The source front body is roughly 60 visible pixels tall. The packed cell/pivot is 80×80/(40,70); this is not the existing player's geometry and is not approved as the final world scale. Compare in the real renderer before replacing anything.

The archive's 600-ms walk preview follows the separate later PixelLab walking candidate's one-tile convention. Default master f2b5c6b has the earlier authored timing. Do not copy constants or array indices between them without reconciliation.

This folder is excluded from the named runtime image catalogue and explicit CMake asset-copy lists. It adds no loaded runtime textures. Its PNG sources and JSON decisions are not a new production importer. Existing art masters, equipment, gameplay and player save version 13 are unchanged.

## Preservation and reproduction

The 23 original files in v1 are kept byte-for-byte with their [original hash inventory](v1/sha256.json). A repository [provenance record](provenance.json) records the original archive checksum and source set. Keep later changes in explicit new revisions rather than editing this reference.

The [historical builder](v1/build_pack.py) is included as provenance and packaging support. It needs Pillow/NumPy, has Windows font paths and regenerates derived previews/evidence. It is not the normal production exporter; do not run it over the retained v1 merely to view the result. A portable source-preserving successor is planned under ART-006.

PixelLab supplied the body poses. Exact PixelLab workbench masks/colour maps supplied the garment artwork. Local assembly only preserves/packs source pixels and verifies composition. No body was manually bent, rescaled or procedurally reanimated for the pack.

The original art task used four generations. Its remaining-balance receipt was 4.75; that is historical information, not a live balance or permission to spend more. Publication and planning request no new artwork generations.

## Evidence limits

Twelve body cells are exact to their sources; garment alpha and layer composition checks pass. Those checks do not prove attractive anatomy, fabric quality, equipment clearance, eight-way animated fit or game acceptance.

The preview JavaScript passed syntax checking. A live browser interaction check was blocked because the active browser address could not be confidently verified on Windows. No successful browser UI test or game-renderer playthrough is claimed. See the [dated review](../../../docs/embervale/06-development/CUSTOM_CHARACTER_REVIEW_2026-11-22.md) for the improvement and verification record.

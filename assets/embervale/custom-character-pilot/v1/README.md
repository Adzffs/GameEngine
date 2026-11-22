# Embervale reusable human base — first customisation pilot

Created 22 November 2026 for Adam. Open `character-workshop.html` in a browser;
it is self-contained and works without a server or an internet connection.

## What is ready

- One original PixelLab adult human body: bald, barefoot and in plain underwear.
- Eight separately generated standing facings, in Embervale's direction order.
- A front-facing four-frame breathing idle and eight-frame walking cycle.
- A separate sleeveless tunic and separate knee-length trousers, matching those
  exact twelve body frames. Body, each garment and the dressed reference remain
  independently available as transparent PNG atlases.
- A preview with motion, facing, body/outfit isolation, skin-palette examples,
  three tunic colours, pause/step and PNG export.

The original body pixels are preserved. The outfit was made with PixelLab's
exact mask and colour tools, not by paying to regenerate a complete dressed
character. The cloth follows the existing close-fitting silhouette and shading.
Loose cloth, bulky armour and sleeves that change the outline need additional
artwork and overlap review.

## Files

| File | Purpose |
| --- | --- |
| `character-workshop.html` | Standalone interactive review |
| `previews/character-layer-preview.gif` | Base, outfit and composition sharing one walk clock |
| `layers/body-standing-eight-way.png` | Eight 80×80 standing cells, one row |
| `layers/body-south.png` | Four idle cells, then eight walking cells |
| `layers/tunic-south.png` | Transparent tunic cells in the same order |
| `layers/trousers-south.png` | Transparent trousers cells in the same order |
| `layers/outfit-south.png` | Both garments, for exact composition checks |
| `layers/dressed-south.png` | PixelLab's combined reference, for exact comparison |
| `source/` | Original transparent generated images and three-row layer proof |
| `manifest.json` | Frame rectangles, coverage, pivot, timing, provenance and limits |
| `outfit-decisions.json` | Replayable exact PixelLab clothing decisions |
| `outfit-tracks.json` | Per-frame garment polygons measured against the generated body |
| `palette-options.json` | Separate cloth ramps and review skin maps |
| `validation.json` | Executed image and layer checks |
| `sha256.json` | File hashes |

## Geometry and timing

The standing generator returned 64×64 cells with a roughly 60-pixel-tall body.
The animation generator added transparent room and returned 84×84 cells.
This pilot packages both losslessly into **80×80 cells with foot pivot (40,70)**.
Only transparent padding is removed from moving frames. Standing art is padded
and translated to align the ground contact across all eight directions. No
source body is scaled, bent, mirrored or procedurally reanimated.

This is **not a drop-in replacement** for the existing player's 64×64 cells and
(32,48) pivot. Runtime integration must deliberately choose the desired physical
on-screen scale and update the relevant binding and resource definitions. The
current game, saved players and existing action art are unchanged.

Direction order: South, West, East, North, Southwest, Southeast, Northwest,
Northeast. All PNGs use binary transparency. Idle holds are 400 ms each (1.6 s).
Walk holds are 80,70,80,70,80,70,80,70 ms (600 ms total), matching the existing
ordinary one-tile walk convention for review. Real travel must advance walking
by visual distance, as the existing game does. These timings do not change
server ticks, movement speed, combat or gathering.

Draw the same clip cell from body, trousers and tunic at the same pivot and
clock, in that order. Palette changes target their own layer; do not globally
tint the finished character. Removing an outfit reveals the untouched base.

## Provenance and generation cost

Character: https://www.pixellab.ai/create-character/506e9990-e9e3-4771-b17f-892c787939d1

- Eight standing views: PixelLab V3, low top-down, size 64, selective outline,
  basic shading, medium detail; two generations.
- South idle: PixelLab V3, four frames, reference frame excluded; one generation.
  Animation group `249aa0d8-8620-4863-853b-f294efdaa4b0`.
- South walk: PixelLab PixMiniMax Walking template, eight frames; one generation.
  Animation group `21093bb9-a630-4aaf-8252-60ec1414f034`.
- Outfit layers: free exact PixelLab pixel workbench mask/colour operations.
- Total: **four generations**. Verified account balance afterwards: **4.75**.
  No subscription upgrade or credit purchase was made.

Source galleries: rotations `6e8b9bb6-782b-4ce4-9dc5-5ef287d84d15`, idle
`a87d6549-c47a-49ee-9ba9-e514a5e590fa`, walk
`411bec70-e143-4d19-8735-f6daf9387549`, body/outfit/composition
`c6bfff89-a034-41a3-84e2-bc751fa9d0b9`. The package contains the actual bytes;
it does not rely on expiring download links.

## Verification and next work

The pack builder checks all twelve generated body frames against the retained
originals, exact transparency, absence of garment pixels outside the body,
lossless crop bounds, independent tunic/trouser material masks, and exact
body-plus-outfit agreement with PixelLab's dressed frames. All eight standing
views retain source pixels and share the recorded ground pivot. Preview assembly
and normal exports never redraw the body.

This is an art/customisation proof, not a game-renderer or gameplay test. Owner
visual acceptance remains open. Animated idle/walk/outfit coverage is South
only; the other seven facings currently have standing body views. Hair, beards,
other body shapes and new equipment hand tracks have not been created.

Next, settle the base's body proportions and on-screen size, then expand the
selected shared walking motion and outfit masks to the other seven directions.
The present 4.75-generation balance is insufficient for seven additional
one-generation walking jobs; keep this proof intact rather than silently mixing
generated and fabricated directions. Existing equipment and action fallbacks
must remain until their own matching clips and hand/occlusion tracks are ready.

`prepare_outfit.py` reads the sources and emits PixelLab decisions; it does not
paint them locally. Replay those against the source animation in PixelLab.
`build_pack.py` then validates and packages already-returned PixelLab pixels.
It uses Python, Pillow and NumPy. The template is editable in
`review-template.html`; rebuilding replaces the generated standalone HTML and
previews, not the original source images.

The preview JavaScript passes a syntax check. Its live browser check was blocked
because Computer Use could not verify the active browser address on Windows.
No successful browser-control or interaction check is claimed. The original
source/layer conservation and composition checks above did pass.

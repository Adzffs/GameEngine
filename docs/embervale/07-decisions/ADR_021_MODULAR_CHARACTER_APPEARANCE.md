# ADR-021: Modular character appearance from reusable PixelLab art

**Date:** 22 November 2026.  
**Status:** Accepted product/production direction; proposed runtime design awaiting implementation evidence.  
**Owner direction:** PixelLab artwork, a reusable base character and a comprehensive plan for custom characters.  
**Scope:** Appearance/content authoring and presentation. Existing gameplay equipment, simulation and save guarantees remain authoritative.

## Context

The original player has complete working action/equipment presentation, while later PixelLab walking and modular axe work lives on a separate branch. A new bald underwear base provides eight standing views and south idle/walk, with separate close-fitting tunic/trouser layers.

Generating a complete new body animation for every hair/clothing/material/loadout combination would duplicate work and content. The narrow pilot proves that body poses and garment colours can be reused; it does not prove a finished wardrobe, general rig or character creator.

The owner identified that the first character needs improvement. Its geometry, proportions, fabric design and missing coverage must be reviewed before production expansion.

## Decision

Adopt these production boundaries:

1. PixelLab supplies original art and shared body-motion candidates. Editors, exact pixel tools and exporters support cleanup, layer extraction, alignment and validation.
2. A body/rig/motion revision defines the fit contract. Clothing, hair and equipment reference compatible revisions explicitly.
3. Keep body, material regions, garment/hair layers, tool shapes, equipment materials and hand/ordering tracks distinct.
4. Use stable bounded cosmetic IDs in a future appearance descriptor. Authoritative equipment remains separate and cannot be granted by appearance choices.
5. Start with one human fit family and a small in-world proof. Preserve the existing playable default until compatible coverage and fallback are demonstrated.
6. Store original sources and dated evidence separately from generated exports. Publish the v1 pilot as reference art, outside runtime batches.
7. Keep the [canonical plan](../05-game/CUSTOM_CHARACTERS.md), [production workflow](../05-game/CUSTOM_CHARACTER_WORKFLOW.md), [review register](../06-development/CUSTOM_CHARACTER_REVIEW_2026-11-22.md) and [backlog](../02-roadmap/BACKLOG.md) synchronised.

Exact body dimensions, production art quality, catalogue/API names, save schema version and the final creator location remain implementation/product decisions. An illustrative JSON format in the plan is not an accepted runtime schema.

## Alternatives considered

| Approach | Consequence |
|---|---|
| Full dressed character for every combination | Fast isolated prototype, but duplicates body motion and scales poorly with wardrobe/loadout combinations |
| One flat body with runtime global tint | Simple, but cannot independently colour skin, eyes, cloth, hair and metal or swap silhouettes correctly |
| Universal skeletal deformation of one sprite | Requires substantial new tooling and quality work; not justified by this first slice or the owner's rejected procedural-body results |
| Compatible authored layers over shared body poses | Chosen direction; reuses motion/material work while acknowledging new garment silhouettes and fit costs |
| Immediate global replacement with the first generated base | Rejected for now: incomplete actions/facings, different scale/pivots and no visual acceptance |

## Consequences

A colour variation of an existing compatible garment needs no new body animation. A different coat, hairstyle, body shape or grip can require new artwork/tracks. Per-layer runtime memory still grows, so shared source work does not imply free rendering or unlimited wardrobe variants.

Layer ordering and body coverage are pose-dependent. The earlier axe masking failure requires an unmasked physical path/grip check before final compositing. General body deformation and massive loadout-specific body generation are not prerequisites.

Appearance persistence will require explicit bounded migration/defaulting, with current save safety and temporary-session isolation preserved. No save change is made by this ADR.

## Adoption evidence and completion

The current [pilot](../../../assets/embervale/custom-character-pilot/README.md) establishes original-pixel conservation and exact body/garment composition for twelve south-facing cells. It is authoring evidence only.

Implementation adoption requires an in-world fit/scale proof, declared eight-way/action coverage, equipment compatibility, bounded resource/fallback checks, safe appearance round trips and player-facing Apply/Cancel acceptance. Record exact commits and owner feedback when each lands. ART-006 is in progress; subsequent backlog tasks remain planned.


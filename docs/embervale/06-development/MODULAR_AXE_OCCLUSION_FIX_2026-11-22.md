# Modular axe handle penetration: failed masking and pose revision

**Date:** 22 November 2026.  
**Task:** ART-005, still in progress.  
**Original defective prototype:** `a58a53591309265dc4d3f5603fde742ae4fcf098`.  
**Rejected masking attempt:** `02fd3cd526ccda6fbb6a68123b9ce1f73d20e005`.  
**Current status:** Arms and tool trajectory reposed; revised authoring candidate, owner visual acceptance open. Runtime art is unchanged.

## What went wrong

The owner reported that the axe handle went through the player and appeared to switch from behind to in front. The original proof switched the whole tool behind the body in loaded poses 1 and 2, then returned it to the front for contact. That was an incorrect compositing workaround.

The first attempted correction removed the whole-tool toggle and restored head/both-grip pixels over the tool. **The owner rejected that result: the problem was still visible.** Its wrists and aim vectors were unchanged. The shaft still ran through the head/chest, and covering part of it with a mask merely hid the intersection. The previous claim that the problem was fixed was wrong.

The first eight preservation/material tests did not check this geometry. The next eleven tests also passed because their expectations explicitly allowed the shaft over the collar and restored head pixels over it. Their 30-before/zero-after landmark count measured the intended masking operation, rather than a clear physical tool path. Those numbers are retained as historical evidence, not visual acceptance.

The failed [original GIF](../../../assets/embervale/equipment-modular-pilot/reports/handle-layering-before.gif) and [original sheet](../../../assets/embervale/equipment-modular-pilot/reports/handle-layering-before.png) remain recorded. The [rejected masking GIF](../../../assets/embervale/equipment-modular-pilot/reports/rejected-masking/materials.gif), [sheet](../../../assets/embervale/equipment-modular-pilot/reports/rejected-masking/contact.png), [recipe](../../../assets/embervale/equipment-modular-pilot/reports/rejected-masking/recipe.json) and [body master](../../../assets/embervale/equipment-modular-pilot/reports/rejected-masking/body.ase) reproduce the second failure.

## What changed in the revised candidate

The actual source poses and attachment track now change together:

- Both hands move forward of the torso. The raised shaft lifts vertically beside the face; it does not pass behind the hair or through the collar.
- The near and far arms are reposed once in the editable body master to reach those same grips. The character's head and feet remain exact; the original outfit is retained beneath the arm edit.
- The contact and bite reach forward, followed by a return to the same ready pose. The tool stays on the same visible side throughout.
- One axe shape, its wooden handle, semantic metal mask and three material palettes remain shared. The correction is not repeated per material.

The [current recipe](../../../assets/embervale/equipment-modular-pilot/recipe.json) retains explicit hand/aim points and body-tool-foreground composition. Grip foreground pixels wrap the handle. Head foreground data remains available, but **no tool pixels overlap the head at all** in this candidate. A head mask is not used to conceal a bad shaft route.

Normal export reads the edited masters and track. It never regenerates the arms or infers a new mask. The initial legacy bootstrap is now retired and rejects: importing the old body would restore the bad attachment geometry. Existing runtime masters are not changed by this offline source edit.

![Reposed candidate with a clear tool path](../../../assets/embervale/equipment-modular-pilot/previews/materials.gif)

The [contact sheet](../../../assets/embervale/equipment-modular-pilot/previews/contact.png) shows all eight poses in three materials. The preview uses a closer integer-scale camera so that the hands, face clearance and full raised tool are visible together.

## Verification that addresses the actual defect

The compositor now examines the **unmasked** tool against the body before composition. Any opaque tool/body overlap outside the two bounded grip regions rejects the candidate before replacing output. Hiding the middle of the shaft under head or shirt pixels cannot satisfy this check.

The [independent PNG tests](../../../tests/test_modular_equipment.py) inspect every native pixel in all 24 material/pose cells. They require all actual tool/body intersections to lie at the grips, zero head intersections, visible skin at both grip anchors, correct composition and an exact ready/closing seam. A real exporter test loads the saved owner-rejected recipe and body and verifies that the new spatial guard refuses them without replacing usable outputs.

Twelve tests pass with real LibreSprite export and no skips; see the [test log](../../../assets/embervale/equipment-modular-pilot/reports/tests.txt). The old fixed-collar expectations were removed because they endorsed the wrong result. Preservation tests now correctly allow the intentional pilot arm edit while checking exact head/foot retention and unchanged masters on normal export. Metal/wood isolation, rejected data, malformed decoded masks and portable receipts remain covered.

The [current revision receipt](../../../assets/embervale/equipment-modular-pilot/reports/pose-revision.json) records the revised clearance and preservation results. The earlier [masking receipt](../../../assets/embervale/equipment-modular-pilot/reports/occlusion-fix.json) belongs to the rejected attempt.

The axe and metal-mask masters are unchanged. The pilot body and transformed tool poses intentionally change. All 33 runtime editable masters and 33 runtime textures remain exact; the live catalogue remains 3,135 frames, 24,342,528 RGBA bytes and 6,085,632 alpha bytes. Timing remains `[260,200,200,80,80,80,100,200]` ms, total 1,200 ms. No runtime code, gameplay timing or save change is included.

## Lesson and remaining scope

Correct the body pose and tool path first. A stable draw order and masks cannot repair a shaft routed through the character. Inspect the unmasked body/tool overlap, both grips and the full timed motion. Tests must reject the visible defect rather than assert pixels chosen to implement a workaround.

This remains one east-facing authoring candidate. The owner has not accepted this revised motion. Body quality, the other seven directions, actual-game migration and the remaining [production plan](../05-game/MODULAR_EQUIPMENT_ANIMATION_PLAN.md) stay open.

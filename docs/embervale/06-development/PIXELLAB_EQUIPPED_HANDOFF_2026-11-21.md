# PixelLab equipped walking: retained pilot and exact handoff

**Date:** 21 November 2026.  
**Task:** ART-005, in progress.  
**Published walking baseline:** `c522956422259256dcfb8c2f5263e225cc34ccac`, draft [PR #48](https://github.com/Adzffs/GameEngine/pull/48).  
**Scope of this commit:** Documentation checkpoint only. The new binary art and authoring changes described below remain local.

## Result

PixelLab generated eight neutral sword-and-shield views and one eight-frame east-facing Walking loop for the existing traveller. The east-facing pilot was downloaded, checked and assembled losslessly into a separate editable master and inspection preview. It has not entered the runtime catalogue or packaging.

Inspection found inconsistent equipment handedness in the south-west neutral view; the north-west weapon/shield overlap also needs review. Do not generate the remaining walking directions from a faulty source or attach the older equipment overlays to the unrelated new gait.

A requested corrective state was refused because fewer than its required ten generation credits remained. The owner said they would add credits; this is intent to replenish the account, not evidence that the credits have arrived. No subscription, upgrade, credit purchase or API credential was created. The account also enforced one concurrent background job.

The local execution session then became unavailable, preventing the final export-preservation checks, regression run and binary/source upload. This documentation checkpoint preserves the exact recovery location and boundaries.

## Service records

| Property | Actual value |
|---|---|
| Original traveller | [Empty-handed source](https://www.pixellab.ai/create-character/73253a5c-9983-4f58-8d67-cd31a1c928d5) |
| Equipped state | [Sword-and-shield traveller](https://www.pixellab.ai/create-character/a8d31a4e-705c-4074-9efc-2323a16934bf) |
| Equipment | Short bronze sword, anatomical right hand; small round wooden shield, left forearm |
| Walking pilot | East only, Walking template, PixMiniMax, eight moving frames |
| Canvas | 64x64; low top-down |
| Selected animation style | Single color outline, Medium shading, Medium detail; no forced palette |
| Review clock | [80,70,80,70,80,70,80,70] ms, 600 ms total |
| Review atlas | 576x64; supplied idle first, then eight moving cells |
| Proposed pivot | (32,48), matching the empty-handed batch |
| Runtime status | Existing complete equipped/gathering/melee batches still apply |

The current service state name is `Equip the same trave`. Preserve its identifier when selecting an export; grouped ZIPs can contain several states.

## Files already saved locally

Recovery checkout:

```text
C:\Users\dohnu\Documents\Codex\2026-11-21\new-chat-2\work\GameEngine
```

The checkout started from the exact published `c522956` source tree, `975b4325b8c6322786025926ba4d73a8d07b5410`. Its local branch is `art/pixellab-equipped-walking`. Inspect its actual index and working changes before resuming.

```text
assets/embervale/pixellab-equipped-pilot/
  generated/equipment-state.zip
  generated/reference-state/        # eight unchanged neutral PNGs and metadata
  generated/walk-east-export.zip
  generated/east/                   # idle, eight moving PNGs, metadata, import validation
  source/walk-east.ase
  textures/walk-east.png
  previews/reference-contact-sheet.png
  previews/walk-east-contact.png
  previews/walk-east.gif
  previews/walk-east-validation.json
tools/art/review_pixellab_walk.py
tools/art/build_pixellab_pilot.py    # local explicit-state selection/evidence-preservation changes
tests/test_pixellab_import.py
```

The GIF is an integer 4x authoring preview, not a native game recording. These paths are a recovery inventory; the files are not uploaded by this documentation commit.

Downloaded service exports remain at:

```text
C:\Users\dohnu\Downloads\Embervale_traveller_huma-Equip_the_same_trave.zip
C:\Users\dohnu\Downloads\Embervale_traveller_huma-Equip_the_same_trave (1).zip
```

The first contains the neutral state; the second also contains the east-facing walk. The checked copies are already in the local art folder.

Prepared runtime integration was withdrawn after the faulty diagonal and credit blocker were established. Its exploratory patches remain outside the checkout at `work/equipped-integration-scaffold.patch` and `work/equipped-import-scaffold.patch`, relative to this chat's workspace. They are incomplete scaffolding, not changes to apply blindly. The final tracked code diff observed before disconnection was limited to the import helper; the review tool, test and new art folder were untracked.

The attempted batch README write failed after the session disconnected. Create it during recovery rather than assuming it exists.

## Executed checks and exact limits

The real LibreSprite review import verified nine nonempty, unclipped 64x64 cells with binary alpha and exact visible imported pixels. It produced the editable master, 576x64 PNG, contact sheet, eight-frame GIF and JSON validation record.

| Local output | SHA-256 |
|---|---|
| Editable east-walk master | `9ee3894999ca0c29f9502637722fc5a99f05379ba171aa37785fb4cf557de9bd` |
| Review atlas | `6ce64240f718d98f78ee0c1868e6a1fb56b6714d1b6fb76adfd70dcd53bab929` |
| Imported walking ZIP | `9f8a8ff2cc5af12bd351bb821c7807e1856b38e01ac08fd802d0c93aa996de03` |

State-selection regression cases passed during development. The resource suite ran 18 tests, with 17 passing and one unavailable-symlink skip. A Windows client build completed before the final scope reduction. These intermediate checks do not establish a final verified updated candidate. Normal-export preservation and the final focused checks remain to run; no new integrated-build or native visual acceptance is claimed.

LibreSprite initially produced the inspection PNG but failed while writing its user brush profile outside the workspace. Repeating it with a writable private `APPDATA` profile succeeded. The working private profile is `work/art-profile`, outside the checkout.

## Resume safely

1. Reconnect the local Windows execution session and inspect the saved checkout, binaries, JSON and actual diff. The interrupted profile contained Unix-style sandbox roots; process creation rejected `file:///workspace`, and the browser kernel rejected `/home/agent/.codex/.arcade-git/aggregate-skills` as a Windows path. Restore a valid host session; do not bypass its access controls.
2. Verify PixelLab credits are available. The prior correction attempt did not start a job. Retain the existing equipped state and raw east-facing export.
3. Review the actual east-facing GIF and the eight neutral views at native scale. Correct south-west handedness and inspect north-west overlap before spending generations on their walks.
4. Generate and inspect the remaining directions one job at a time, preserving source state IDs, actual settings and raw exports. Keep the body's matching equipment together.
5. Finish the local review/export helper before publishing it. In particular, guard copying an import archive onto its own retained path; the initial outside-folder import succeeded, but the same-path reproduction needs that check. Verify normal export preserves the editable master, and verify rejected input preserves earlier raw evidence as well as the master.
6. Publish the accepted raw inputs, editable master, previews, helpers and exact results. Integrate only a complete matched loadout with explicit selection and a complete legacy fallback. Reconcile runtime/both validation tables and packaging only when its art is ready.
7. Update this report, the batch README and current status. ART-005 stays in progress until the wider animation work and owner/native visual acceptance are actually complete.

The runtime resource totals remain those of the earlier empty-handed candidate: ten batches, 33 textures and 3,135 frames. This pilot adds no bound runtime frames.

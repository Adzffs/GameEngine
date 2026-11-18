# Content change review template

Use for region, placement, quest, item, recipe, NPC, loot, dialogue, shop or asset changes. Mark irrelevant sections explicitly rather than pretending all features are present.

## Change identity

- Source commit, region/package ID, base hash and candidate hash:
- Authored IDs created, changed, removed or migrated:
- Purpose and expected player effect:
- Tools and schema/validator versions:

## Safety and compatibility

- Bounds, dimensions, references and resource limits checked:
- Occupancy, approach tiles, spawn and required routes checked:
- NPC/monster lifecycle, wander/cap/respawn rules preserved:
- Quest/dialogue/shop dependencies resolved:
- Player positions, respawns, recovery piles and supported saves considered:
- Draft/publication distinction and stale-base conflict behaviour verified:
- Previous publication and rollback identified:

## Gameplay and presentation

- Relevant fresh-player journey exercised:
- Inputs, outputs, prices, rewards and multi-step economic effects checked:
- Art pivot/footprint/frame/provenance checks complete:
- Preview and game semantics agree:
- Objective text, NPC naming and player instructions align:

## Evidence

Record validation output, relevant tests, manual playtest result, preview and known limitations. Only state checks actually performed. Link the changed catalogue, region plan and release notes.

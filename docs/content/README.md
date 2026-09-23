# Content Catalogue Plan

The catalogue should capture every player-facing content record with stable IDs and links to gameplay rules. Add entries when content becomes real enough to implement. Avoid committing to huge catalogues before the first slice validates the categories.

## Catalogue groups

- **Items:** stable ID, display name, category, stack rules, source, uses, value, equipment stats, persistence and icons.
- **Resources:** node/resource ID, required tool or skill, action duration, success/failure, output, respawn, region and quest interactions.
- **Recipes:** stable ID, grid dimensions, shaped pattern or shapeless ingredients, input counts, output, station/skill conditions, unlock source, consumption rules and test cases.
- **Skills:** stable ID, actions granting progress, XP curve, levels, unlocks, UI presentation and save format.
- **Quests:** stable ID, offer source, prerequisites, objectives, state transitions, reward, dialogue nodes, persistence and failure cases.
- **NPCs:** stable ID, role, location/spawn, dialogue graph, services, identity checks and content links.
- **Pets:** stable ID, acquisition source, role, abilities or cosmetic features, ownership rules, UI, persistence and balance alternatives.
- **Enemies and bosses:** stable ID, location, stats, behavior, attack phases, telegraphs, drops, respawn/retry rules and expected preparation.
- **Regions:** stable ID, landmarks, routes, safe/dangerous areas, resources, NPCs, enemies, gates, discoveries and intended player progression.

## Content rules

- IDs are stable and never reused. Display names can change without changing IDs.
- Every item/resource/recipe has at least one documented use or an explicit reason for being a collection/cosmetic reward.
- Rare loot is never the sole route to essential progression unless a guaranteed alternative is documented.
- Every boss kill has baseline reward value; rare drops are separately identified and tuned.
- Content references other content by stable ID, not display label.
- Keep source data, validation rules and presentation references discoverable from each content brief.

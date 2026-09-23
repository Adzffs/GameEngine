# Development Roadmap

This roadmap is organized around outcomes the player can experience, with foundation verification first because the current repository CI is red and the latest quest review has open evidence gaps. Milestone order is a working plan and should be revised when playtests or technical findings change dependencies.

| ID | Milestone | Exit evidence | Status at 2026-09-23 |
|---|---|---|---|
| M0 | Verify current foundations and restore a green baseline | Current master reviewed; CI failure diagnosed; high-risk quest/save/shop paths tested; open review gaps closed or explicitly scoped. | In progress |
| M1 | Lock player-facing direction | World premise, tone, combat feel, pet role, recipe discovery and multiplayer scope decided or explicitly deferred. | Planned |
| M2 | Specify progression and crafting | Initial skills/resources/unlocks and grid recipe rules are designed with acceptance tests. | Planned |
| M3 | Build first area and resource-to-upgrade path | Small authored region connects a goal, exploration, resource gathering, crafting and a useful upgrade. | Planned |
| M4 | Add one collectible pet | One pet can be obtained, owned, used as designed and saved. | Planned |
| M5 | Add one repeatable boss and rare-loot chase | Learnable encounter, dependable rewards and a rare high-value drop work authoritatively. | Planned |
| M6 | Integrate the first playable slice | New player can complete the connected loop, understand the next goal and retain progress after restart. | Planned |
| M7 | Expand content and prepare releases | More content expands a proven loop; builds, packaging and save compatibility are documented. | Planned |

## Working rules

- Split each milestone into small reviewable tasks in the Work Tracker.
- Avoid starting broad content expansion before the first representative loop is playable.
- A milestone is complete only when its exit evidence is linked and reviewed.
- Technical foundations and player-facing content can progress in parallel only when their dependencies are clear.
- Reorder milestones when an evidence-backed dependency requires it; document the reason in Decisions or Work Log.

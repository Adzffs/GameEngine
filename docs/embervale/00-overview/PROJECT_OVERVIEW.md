# Project overview

**Document version:** 1.0 · **Reviewed:** 18 November 2026  
**Purpose:** Explain the whole project, preserve the user's decisions and show where to begin.  
**Status:** Current project summary plus explicitly labelled future direction.

## 1. What we are building

Embervale is a tile-based RPG being developed in a custom C++20 / SDL3 GameEngine. The present source already supports a substantial collection of RPG systems. The next major improvement is to make its world and content easier to design, understand and change.

The project has three connected parts:

| Part | Responsibility | What success looks like |
|---|---|---|
| **Embervale, the game** | Places, characters, activities, progression, art, sound and the player experience | A coherent starter adventure that people can learn, explore and enjoy |
| **GameEngine, the runtime** | Simulation, movement, interactions, persistence, rendering and platform support | Predictable behaviour, trustworthy saves and reproducible builds |
| **Embervale Studio, the planned tools** | World editing, content definitions, validation, previews and authoring workflows | A human or AI can make the same reviewable content change through shared operations |

The engine is the delivery mechanism for the game. The editor is valuable when it shortens the route from a design idea to a playable, tested change. A large number of editor panels is not, on its own, evidence of useful progress.

## 2. Decisions already established by the user

The following are design constraints from this project's conversation. They are not technical features inferred from code:

- The starter region should use the **balanced approach**, mixing settlement life, gathering, crafting, exploration and gradually increasing danger.
- There should be **no central core or single hub** that every activity revolves around. Services should sit along believable routes and settlement edges.
- Current entity positions are temporary. Their current coordinates are a compatibility baseline to preserve during migration, not a permanent world design.
- **River Crossing** is the settlement direction to develop. The precise final geographic layout remains open to blockout and playtesting.
- The present game has **flat ground**. Higher ground, cliffs, stairs, bridges with separate walkable layers and upper floors need actual runtime work.
- The present artwork is basic. Retain usable placeholders while proving a small, coherent art pipeline; choose final art production after seeing it in the game.
- Studio must be useful to the user visually and to an AI programmatically. The CLI and GUI should operate on the same content rules and commands.
- Documentation should be easy to understand, organised for GitHub and expandable as new systems arrive.

The [game design](../05-game/GAME_DESIGN.md), [River Crossing proposal](../05-game/RIVER_CROSSING.md) and [Studio design](../04-studio/STUDIO_DESIGN.md) develop those decisions without treating every proposed detail as already approved.

## 3. What exists today

Source inspection at the pinned baseline identifies a 100×100 development map; hardcoded terrain and placement definitions; a 600 ms world tick; movement, pathfinding and running; gathering and crafting; combat, rewards and monster behaviours; inventory, equipment, banking and shops; quest and dialogue definitions; development-player saves; content validators; and an SDL3 presentation layer. Source and test registration evidence are catalogued in [Implemented systems](../01-current-state/IMPLEMENTED_SYSTEMS.md).

This is a **development RPG foundation**. It is not yet a completed starter region, a released content editor or a deployed multiplayer service. A local authoritative world does not establish network protocol, authentication, deployment, concurrency or MMO operations.

The current baseline contains seven resource placements, three workstations and ten NPC/monster spawn definitions. These counts describe authored records; delayed creation rules mean the number of live entities varies by lifecycle stage. The [current content catalogue](../05-game/CONTENT_CATALOGUE.md) records the differences.

## 4. What this documentation review established

The default branch was freshly checked on 18 November 2026 and remained [e9d32932e8f9b27e96283313cca320058792a81f](https://github.com/Adzffs/GameEngine/commit/e9d32932e8f9b27e96283313cca320058792a81f). The review covered the repository tree, history, pull requests, CI metadata and selected important implementation files. It did not execute a fresh build or constitute a complete security audit.

The existing CMake configuration registers **81 test executables: 70 headless, 7 graphics and 4 client**. That is a source inventory, not a pass count. The latest inspected push run is failed and has no useful execution steps or logs in the returned evidence. Its failure must be investigated before claiming a clean current baseline. [Build definition](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/CMakeLists.txt) · [Workflow run](https://github.com/Adzffs/GameEngine/actions/runs/44018694678).

At the baseline review, the older documentation pull request was open. Its ideas can inform this pack, but its age and branch state mean it cannot establish current behaviour. The integration notes record how it relates to this documentation update.

## 5. The first valuable release

**Studio Alpha 0.1** should let the user open an exported version of the real development map, paint a small area or move an existing tree, see validation and a preview, undo or redo the edit, save and reopen it, and play the same result. An AI must be able to perform the same scoped change through a documented command interface.

The release depends on stable identities, explicit content versions, safe writes, collision and reachability checks, save compatibility, baseline comparison and a minimal recovery path. These protections belong in the first useful editing slice. Moving them to a later hardening phase would make the first tool unsafe to depend on.

The first release does not require all content editors, a networking rewrite, final artwork or a complete elevation system. Those each receive their own acceptance gates in the [master roadmap](../02-roadmap/MASTER_ROADMAP.md).

## 6. How to read status labels

| Label | Meaning |
|---|---|
| **SOURCE VERIFIED** | The stated implementation or configuration is present in inspected source at the cited commit |
| **REMOTE RESULT VERIFIED** | A particular returned GitHub result was inspected, such as a merged PR or failed workflow |
| **HISTORICAL REPORT** | A previous PR or conversation reports an outcome that has not been rerun here |
| **PLANNED** | Included in the roadmap; implementation and acceptance evidence are still required |
| **PROPOSED** | A candidate design or technical choice requiring validation or an architecture decision |
| **UNVERIFIED** | Evidence is missing, incomplete or contradictory |
| **COMPLETE** | A work item meets its acceptance criteria with linked implementation and validation evidence |

Source-verified functionality can still have defects. A completed document does not make the feature it describes complete. A feature percentage is deliberately not used until there is a defined scope and a trustworthy completion record.

## 7. How the pack is organised

Start with this overview and the master roadmap. Use current-state documents when checking facts about the existing engine. Use engine and Studio specifications when designing an implementation. Use game documents for player experience and content decisions. Use development guides for repeatable work, and the templates to capture new features, bugs, decisions and playtests.

The backlog preserves the original STUDIO planning IDs and maps them to the updated milestones. These IDs are internal references; they are not GitHub issue numbers. An issue number is only recorded after an actual issue exists.

## 8. How we will keep it accurate

Each meaningful change should update the relevant feature document and status row with a commit, PR or test result. Keep aspirations in the roadmap and present-tense claims in the current-state inventory. Record why an architectural choice changed, rather than silently replacing its old rationale. Date verification notes so a reader can judge their freshness.

The documentation pack is designed to grow by adding focused documents and links. Avoid maintaining several competing master roadmaps. The canonical roadmap is [MASTER_ROADMAP.md](../02-roadmap/MASTER_ROADMAP.md), with [BACKLOG.md](../02-roadmap/BACKLOG.md) providing implementation work packages.

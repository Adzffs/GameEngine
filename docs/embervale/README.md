# Embervale documentation

**Documentation version:** 1.2 · **Review date:** 20 November 2026\
**Original source-review baseline:** [e9d32932e8f9b27e96283313cca320058792a81f](https://github.com/Adzffs/GameEngine/commit/e9d32932e8f9b27e96283313cca320058792a81f)

## Start with the question you have

| Your question | Best starting point |
|---|---|
| What is this project? | [Project overview](00-overview/PROJECT_OVERVIEW.md) |
| What has actually been implemented? | [Current reviewed status](01-current-state/CURRENT_STATUS.md) |
| What did the full project review find? | [Critical fixes and roadmap reconciliation](06-development/PROJECT_REVIEW_2026-11-20.md) |
| What should we do next? | [Master roadmap](02-roadmap/MASTER_ROADMAP.md) |
| What task can I pick up? | [Backlog](02-roadmap/BACKLOG.md) |
| How do I run the game? | [Getting started](06-development/GETTING_STARTED.md) |
| Where can I see the first in-game art? | [Forest rendering and captures](06-development/FOREST_RENDERING_2026-11-19.md) |
| Where are the new player, Mara and furnace sprites? | [Cast art and walking](06-development/CAST_RENDERING_2026-11-19.md) |
| Where are river banks, stone floors and mining sprites? | [Environment art and captures](06-development/ENVIRONMENT_RENDERING_2026-11-19.md) |
| Where are the bank chest and crafting table sprites? | [Service art and captures](06-development/SERVICE_RENDERING_2026-11-19.md) |
| Where is the first building and reusable kit? | [Workshop, camera and roof visibility](06-development/BUILDING_RENDERING_2026-11-19.md) |
| Where are contextual plants, leaf litter and age variants? | [World variation and captures](06-development/WORLD_VARIATION_2026-11-19.md) |
| Wall directions, windows and interior cutaways | [Corrected wall orientation](06-development/WALL_ORIENTATION_2026-11-19.md) |
| Which quest follows the starter equipment journey? | [Quest continuity and current rewards](06-development/QUEST_CONTINUITY_2026-11-19.md) |
| How does the first equipment journey work? | [Starter upgrade journey](06-development/STARTER_UPGRADE_JOURNEY_2026-11-19.md) |
| What are the current starter costs and trade results? | [Starter economy correction](06-development/STARTER_ECONOMY_2026-11-19.md) |
| How do I start without development boosts? | [Fresh-player profile](05-game/FRESH_PLAYER_PROFILE.md) |
| Export the actual starter world? | [Starter-world export](04-studio/STARTER_WORLD_EXPORT.md) |
| How will the editor work? | [Studio design](04-studio/STUDIO_DESIGN.md) |
| Play or edit River Crossing? | [Candidate, map overview and launch guide](../../content/worlds/README.md) |
| How is the settlement arranged? | [River Crossing geography](05-game/RIVER_CROSSING.md) |
| How should future animations be authored and reviewed? | [Animation guide](05-game/ANIMATION_GUIDE.md) |
| What did the animation research and polish establish? | [Sources, measured findings and prototype status](06-development/ANIMATION_RESEARCH_2026-11-20.md) |
| What still needs resolving? | [Risk register](07-decisions/RISKS_AND_QUESTIONS.md) |

Use [current status](01-current-state/CURRENT_STATUS.md) for the maintained implementation/evidence summary. Original baseline inventories remain dated references. The [project review](06-development/PROJECT_REVIEW_2026-11-20.md) reconciles editor/art work and the local animation branch, records the safety fixes and identifies work before audio. Milestone releases and visible starter/animation acceptance remain incomplete.

## Suggested reading paths

**Project owner:** Overview → implemented systems → roadmap → release gates → risks.\
**Engine contributor:** Getting started → repository guide → architecture → content model → save compatibility → relevant backlog task.\
**World/content designer:** Game design → catalogue → River Crossing → editor workflows → playtest template.\
**AI contributor:** Overview → source baseline → task and dependencies → CLI contract → contribution rules → evidence/hand-off template.

**Animation contributor:** [Animation guide](05-game/ANIMATION_GUIDE.md) → [research and findings](06-development/ANIMATION_RESEARCH_2026-11-20.md) → current action implementation and dated integration report. Verify the checkout: the research labels local prototype results separately from published code.

## Complete document catalogue

### Project orientation

| Document | What it contains |
|---|---|
| [PROJECT_OVERVIEW.md](00-overview/PROJECT_OVERVIEW.md) | What Embervale, GameEngine and Studio are; established decisions and honest status labels |
| [PRODUCT_REQUIREMENTS.md](00-overview/PRODUCT_REQUIREMENTS.md) | User outcomes, scope, quality requirements and acceptance journeys |
| [GLOSSARY.md](00-overview/GLOSSARY.md) | Plain-language definitions of technical and project terms |

### What has been built

| Document | What it contains |
|---|---|
| [CURRENT_STATUS.md](01-current-state/CURRENT_STATUS.md) | Current source versions, implementation, results, outstanding acceptance and next work |
| [IMPLEMENTED_SYSTEMS.md](01-current-state/IMPLEMENTED_SYSTEMS.md) | Source-backed feature inventory, counts, limitations and integration risks |
| [DEVELOPMENT_HISTORY.md](01-current-state/DEVELOPMENT_HISTORY.md) | Development chapters and all 234 returned default-branch commits |
| [REPOSITORY_GUIDE.md](01-current-state/REPOSITORY_GUIDE.md) | Code responsibilities, build entry points, integration seams and old-document reconciliation |
| [EVIDENCE_LEDGER.md](01-current-state/EVIDENCE_LEDGER.md) | Pinned sources, remote results, historical claims and verification limits |

### What happens next

| Document | What it contains |
|---|---|
| [MASTER_ROADMAP.md](02-roadmap/MASTER_ROADMAP.md) | R0–R10 milestones, dependencies, outcomes and implementation order |
| [BACKLOG.md](02-roadmap/BACKLOG.md) | 96 work packages with priorities, dependencies and acceptance results |
| [BACKLOG.json](02-roadmap/BACKLOG.json) | The same work-package records in a machine-readable form |
| [RELEASE_GATES.md](02-roadmap/RELEASE_GATES.md) | Observable acceptance criteria for foundations, Alpha releases, beta and experiments |

### How the runtime and data fit together

| Document | What it contains |
|---|---|
| [ARCHITECTURE.md](03-engine/ARCHITECTURE.md) | Current and proposed dependencies, authority and content integration |
| [CONTENT_MODEL.md](03-engine/CONTENT_MODEL.md) | Proposed schema, coordinates, layers, IDs, canonicalisation and publication |
| [SAVE_COMPATIBILITY.md](03-engine/SAVE_COMPATIBILITY.md) | Current save boundaries, migration contracts and all saved-location risks |
| [STARTER_WORLD_BASELINE.md](03-engine/STARTER_WORLD_BASELINE.md) | Exact terrain, occupancy, placement/lifecycle ownership and saved-location inventory |

### Tools for people and AI

| Document | What it contains |
|---|---|
| [STARTER_WORLD_EXPORT.md](04-studio/STARTER_WORLD_EXPORT.md) | Implemented read-only command, snapshot contract and compiled baseline fixture |
| [STUDIO_DESIGN.md](04-studio/STUDIO_DESIGN.md) | Workspace scope, minimal editor, shared commands and accessibility |
| [STARTER_WORLD_IMPORT.md](04-studio/STARTER_WORLD_IMPORT.md) | Current runtime input limits, validator, game launch, save isolation and compatibility |
| [CLI_AND_AI_CONTRACT.md](04-studio/CLI_AND_AI_CONTRACT.md) | Proposed command interface, transactions, diagnostics and automation safeguards |
| [EDITOR_WORKFLOWS.md](04-studio/EDITOR_WORKFLOWS.md) | Practical authoring, recovery, conflict and playtest journeys |

### The player experience

| Document | What it contains |
|---|---|
| [GAME_DESIGN.md](05-game/GAME_DESIGN.md) | Game pillars, progression, starter journey and expansion boundaries |
| [FRESH_PLAYER_PROFILE.md](05-game/FRESH_PLAYER_PROFILE.md) | Normal starting skills/tools/tutorial, launch profiles and save isolation |
| [RIVER_CROSSING.md](05-game/RIVER_CROSSING.md) | First authored flat candidate, roads, reservations, placements and remaining acceptance |
| [CONTENT_CATALOGUE.md](05-game/CONTENT_CATALOGUE.md) | Current items, recipes, skills, NPCs, quests, stations and resources |
| [ART_AND_AUDIO.md](05-game/ART_AND_AUDIO.md) | Small art pilot, asset pipeline, animation, sound, readability and provenance |
| [ANIMATION_GUIDE.md](05-game/ANIMATION_GUIDE.md) | Reusable pose, timing, equipment, authority, export, review and handoff guidance |

### Repeatable development

| Document | What it contains |
|---|---|
| [PROJECT_REVIEW_2026-11-20.md](06-development/PROJECT_REVIEW_2026-11-20.md) | Full review, three integrity fixes, current evidence and remaining priorities |
| [GETTING_STARTED.md](06-development/GETTING_STARTED.md) | Local CMake setup, Windows client, saves and editor guidance |
| [FRESH_PLAYER_PROFILE_2026-11-19.md](06-development/FRESH_PLAYER_PROFILE_2026-11-19.md) | Fresh-profile implementation, successful builds and deferred acceptance |
| [RIVER_CROSSING_BLOCKOUT_2026-11-19.md](06-development/RIVER_CROSSING_BLOCKOUT_2026-11-19.md) | Authored map, generation/build evidence, identity/lifecycle contract and remaining acceptance |
| [STARTER_ECONOMY_2026-11-19.md](06-development/STARTER_ECONOMY_2026-11-19.md) | Confirmed timber correction, starter budgets, report commands and deferred pacing |
| [STARTER_UPGRADE_JOURNEY_2026-11-19.md](06-development/STARTER_UPGRADE_JOURNEY_2026-11-19.md) | Tutorial material goals, live hints and unfunded production route with timing limits |
| [QUEST_CONTINUITY_2026-11-19.md](06-development/QUEST_CONTINUITY_2026-11-19.md) | Canonical encounter name, actual quest rules, reward choices and post-tutorial guidance |
| [FOREST_RENDERING_2026-11-19.md](06-development/FOREST_RENDERING_2026-11-19.md) | First forest kit in the real renderer, packaging, state/overlap captures and remaining art work |
| [CAST_RENDERING_2026-11-19.md](06-development/CAST_RENDERING_2026-11-19.md) | Player/Mara/furnace art, distance-driven walk poses, reusable export and actual renderer captures |
| [ENVIRONMENT_RENDERING_2026-11-19.md](06-development/ENVIRONMENT_RENDERING_2026-11-19.md) | Water/bank quarters, stone floors, preserved ground and copper/tin depletion captures |
| [SERVICE_RENDERING_2026-11-19.md](06-development/SERVICE_RENDERING_2026-11-19.md) | Bank/table masters, typed station sprites and actual River Crossing captures |
| [BUILDING_RENDERING_2026-11-19.md](06-development/BUILDING_RENDERING_2026-11-19.md) | Modular overhead building kit, schema-4 workshop, collision, editor preservation and roof cutaway |
| [WORLD_VARIATION_2026-11-19.md](06-development/WORLD_VARIATION_2026-11-19.md) | Contextual terrain/plants/leaves, tree/rock variants, building age/material kit and actual game-camera evidence |
| [WALL_ORIENTATION_2026-11-19.md](06-development/WALL_ORIENTATION_2026-11-19.md) | Directional wall/window/corner art, interior front cutaway, floor/eave alignment and camera captures |
| [ANIMATION_RESEARCH_2026-11-20.md](06-development/ANIMATION_RESEARCH_2026-11-20.md) | Seven primary sources, measured defects, local polish decisions, timing tables, verification and evidence limits |
| [CONTRIBUTING.md](06-development/CONTRIBUTING.md) | Focused work, evidence, review, compatibility and parallel-agent handoff |
| [TESTING.md](06-development/TESTING.md) | Current registrations, meaningful future tests and result records |
| [RUNTIME_IMPORT_VERIFICATION_2026-11-18.md](06-development/RUNTIME_IMPORT_VERIFICATION_2026-11-18.md) | Runtime-input build/test results, real dummy-video launch and precise platform limits |
| [BASELINE_VERIFICATION_2026-11-18.md](06-development/BASELINE_VERIFICATION_2026-11-18.md) | Fresh baseline/export build results, environment, reproducible commands and platform limits |
| [RELEASE_PROCESS.md](06-development/RELEASE_PROCESS.md) | Versioned candidates, packaging, verification, rollback and maintenance |
| [SECURITY_AND_RELIABILITY.md](06-development/SECURITY_AND_RELIABILITY.md) | Bounded content, scoped writes, recovery and future trust boundaries |
| [DOCUMENTATION_MAINTENANCE.md](06-development/DOCUMENTATION_MAINTENANCE.md) | Source-of-truth rules, freshness, updates and archiving |
| [REPOSITORY_CLEANUP.md](06-development/REPOSITORY_CLEANUP.md) | Exact removal reasons, ignore fixes and old-PR integration |

### Decisions and unresolved work

| Document | What it contains |
|---|---|
| [DECISIONS.md](07-decisions/DECISIONS.md) | User direction, proposed architecture choices and open ADRs |
| [ADR_016_READ_ONLY_WORLD_EXPORT.md](07-decisions/ADR_016_READ_ONLY_WORLD_EXPORT.md) | Original export decision, deterministic format and historical dependency boundary |
| [ADR_017_BOUNDED_STARTER_WORLD_IMPORT.md](07-decisions/ADR_017_BOUNDED_STARTER_WORLD_IMPORT.md) | Implemented bounded parser, core/runtime ownership, selected spawn authority and isolated playtests |
| [RISKS_AND_QUESTIONS.md](07-decisions/RISKS_AND_QUESTIONS.md) | Observed findings, design risks, decision triggers and closure rules |

### Reusable work records

| Document | What it contains |
|---|---|
| [FEATURE_SPEC.md](08-templates/FEATURE_SPEC.md) | A focused, implementable feature with evidence and rollback |
| [BUG_REPORT.md](08-templates/BUG_REPORT.md) | A reproducible issue with exact inputs and expected behaviour |
| [PLAYTEST_REPORT.md](08-templates/PLAYTEST_REPORT.md) | Hypotheses, fresh-profile measurements and player observations |
| [ADR_TEMPLATE.md](08-templates/ADR_TEMPLATE.md) | Context, alternatives, decision, consequences and validation |
| [CONTENT_CHANGE_CHECKLIST.md](08-templates/CONTENT_CHANGE_CHECKLIST.md) | Content identity, dependencies, compatibility and evidence |
| [WORK_SESSION.md](08-templates/WORK_SESSION.md) | A concise contributor or AI handoff |

## Existing specialist references

- [Development-player persistence](../DevelopmentPlayerPersistence.md): current application lifecycle and guarantee boundaries.
- [Starter character contract](../StarterCharacters.md): the six characters' voices, wants, relationships and examine text.
- [Economy ledger](../EconomyLedger.md): reconciled item sources/sinks, design targets and the unresolved multi-step economy reproduction task.

## Visual planning reference

The [current candidate overview](../../content/worlds/river-crossing.overview.svg) is drawn from the actual authored world file. The earlier [River Crossing diagram](assets/river-crossing-proposed.png) shows the planning proposal using documented tile coordinates. It is not a screenshot of the current game or a validated runtime map. Its [SVG](assets/river-crossing-proposed.svg) and [reproducible plotting script](assets/render_river_crossing.py) are provided alongside the plan.

## Keeping this useful

Use [documentation maintenance](06-development/DOCUMENTATION_MAINTENANCE.md) when a feature changes. Keep one canonical roadmap, preserve planning IDs, pin factual sources and label proposed examples. New work should enter through a focused specification or issue, with acceptance evidence recorded when it is complete.


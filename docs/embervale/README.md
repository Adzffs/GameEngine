# Embervale documentation

**Documentation version:** 1.1 · **Review date:** 18 November 2026\
**Code baseline:** [e9d32932e8f9b27e96283313cca320058792a81f](https://github.com/Adzffs/GameEngine/commit/e9d32932e8f9b27e96283313cca320058792a81f)

## Start with the question you have

| Your question | Best starting point |
|---|---|
| What is this project? | [Project overview](00-overview/PROJECT_OVERVIEW.md) |
| What has actually been implemented? | [Implemented systems](01-current-state/IMPLEMENTED_SYSTEMS.md) |
| What should we do next? | [Master roadmap](02-roadmap/MASTER_ROADMAP.md) |
| What task can I pick up? | [Backlog](02-roadmap/BACKLOG.md) |
| How do I run the game? | [Getting started](06-development/GETTING_STARTED.md) |
| How do I start without development boosts? | [Fresh-player profile](05-game/FRESH_PLAYER_PROFILE.md) |
| Export the actual starter world? | [Starter-world export](04-studio/STARTER_WORLD_EXPORT.md) |
| How will the editor work? | [Studio design](04-studio/STUDIO_DESIGN.md) |
| Where should the settlement go? | [River Crossing proposal](05-game/RIVER_CROSSING.md) |
| What still needs resolving? | [Risk register](07-decisions/RISKS_AND_QUESTIONS.md) |

The source contains substantial RPG systems, starter-world export, bounded validation and explicit runtime loading of supported starter snapshots. Shared editing, guarded sibling saving, undo/redo and the visual editor are implemented in bounded slices. The [fresh-player profile](05-game/FRESH_PLAYER_PROFILE.md) is now available for normal starting progression, with its runtime/visible acceptance deferred at the owner's request. The proposed geographic redesign remains planned. Follow the [current backlog](02-roadmap/BACKLOG.md) for implementation status. The runtime-import slice passed 77/77 headless and 89/89 Linux SDL tests; see the [runtime verification record](06-development/RUNTIME_IMPORT_VERIFICATION_2026-11-18.md) for executed source/environment evidence. The [earlier baseline/export results](06-development/BASELINE_VERIFICATION_2026-11-18.md) remain separately recorded.

## Suggested reading paths

**Project owner:** Overview → implemented systems → roadmap → release gates → risks.\
**Engine contributor:** Getting started → repository guide → architecture → content model → save compatibility → relevant backlog task.\
**World/content designer:** Game design → catalogue → River Crossing → editor workflows → playtest template.\
**AI contributor:** Overview → source baseline → task and dependencies → CLI contract → contribution rules → evidence/hand-off template.

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
| [IMPLEMENTED_SYSTEMS.md](01-current-state/IMPLEMENTED_SYSTEMS.md) | Source-backed feature inventory, counts, limitations and integration risks |
| [DEVELOPMENT_HISTORY.md](01-current-state/DEVELOPMENT_HISTORY.md) | Development chapters and all 234 returned default-branch commits |
| [REPOSITORY_GUIDE.md](01-current-state/REPOSITORY_GUIDE.md) | Code responsibilities, build entry points, integration seams and old-document reconciliation |
| [EVIDENCE_LEDGER.md](01-current-state/EVIDENCE_LEDGER.md) | Pinned sources, remote results, historical claims and verification limits |

### What happens next

| Document | What it contains |
|---|---|
| [MASTER_ROADMAP.md](02-roadmap/MASTER_ROADMAP.md) | R0–R10 milestones, dependencies, outcomes and implementation order |
| [BACKLOG.md](02-roadmap/BACKLOG.md) | 87 work packages with priorities, dependencies and acceptance results |
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
| [RIVER_CROSSING.md](05-game/RIVER_CROSSING.md) | A proposed flat tile layout, roads, reservations, placements and playtest rules |
| [CONTENT_CATALOGUE.md](05-game/CONTENT_CATALOGUE.md) | Current items, recipes, skills, NPCs, quests, stations and resources |
| [ART_AND_AUDIO.md](05-game/ART_AND_AUDIO.md) | Small art pilot, asset pipeline, animation, sound, readability and provenance |

### Repeatable development

| Document | What it contains |
|---|---|
| [GETTING_STARTED.md](06-development/GETTING_STARTED.md) | Local CMake setup, Windows client, saves and editor guidance |
| [FRESH_PLAYER_PROFILE_2026-11-19.md](06-development/FRESH_PLAYER_PROFILE_2026-11-19.md) | Fresh-profile implementation, successful builds and deferred acceptance |
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

The [River Crossing diagram](assets/river-crossing-proposed.png) shows the proposed flat candidate using exact documented tile coordinates. It is not a screenshot of the current game or a validated runtime map. Its [SVG](assets/river-crossing-proposed.svg) and [reproducible plotting script](assets/render_river_crossing.py) are provided alongside the plan.

## Keeping this useful

Use [documentation maintenance](06-development/DOCUMENTATION_MAINTENANCE.md) when a feature changes. Keep one canonical roadmap, preserve planning IDs, pin factual sources and label proposed examples. New work should enter through a focused specification or issue, with acceptance evidence recorded when it is complete.


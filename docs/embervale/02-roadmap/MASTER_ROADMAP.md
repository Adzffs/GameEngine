# Embervale master roadmap

**Version:** 1.0 · **Status date:** 18 November 2026  
**Scope:** GameEngine, Embervale, Embervale Studio and project delivery.  
**Canonical roadmap:** This document replaces the competing phase sequences in the earlier Studio plans. The original STUDIO task IDs remain traceable in the [backlog](BACKLOG.md).

## 1. The destination

Build a coherent, expandable RPG whose world can be authored through dependable tools. The first player-facing objective is a balanced, flat River Crossing starter area with believable geography, distributed services and satisfying gathering, crafting and combat progression. The first tooling objective is a complete, reversible map-editing workflow available to both a person and an AI.

Longer-term work includes richer assets and content, multiple regions, elevation, repeatable encounters, companions and multiplayer research. Each is a separately scoped investment. None is a prerequisite for proving that the current game can support an enjoyable starter experience.

## 2. Starting point and immediate priorities

The baseline is commit [e9d32932e8f9b27e96283313cca320058792a81f](https://github.com/Adzffs/GameEngine/commit/e9d32932e8f9b27e96283313cca320058792a81f). Source inspection confirms substantial RPG systems, a flat 100×100 development world, placeholder rendering and hardcoded content placement. Studio and an external region pipeline are planned. See [Implemented systems](../01-current-state/IMPLEMENTED_SYSTEMS.md).

Four findings change the implementation order:

1. The latest inspected [push workflow](https://github.com/Adzffs/GameEngine/actions/runs/44018694678) failed, with no usable step execution/log evidence returned. Establish why and obtain a reproducible baseline before treating earlier test reports as current assurance.
2. The development player receives boosted XP under the current configuration. Starter pacing must be measured using a deliberately defined fresh-player profile. See [DevelopmentConfig](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Core/DevelopmentConfig.h).
3. Some existing economy and objective descriptions disagree with implementation. Correct documentation facts, then resolve any remaining gameplay or narrative disagreements explicitly.

4. Source-derived shop/recipe arithmetic suggests a repeatable 3-coin to 24-coin log/plank/shield chain. This needs runtime reproduction and a separate correction if confirmed before starter balance can be signed off. See GAME-008 and the [economy ledger](../../EconomyLedger.md).

## 3. Roadmap at a glance

All R0–R10 milestones below are **planned**. This documentation pack completes a planning deliverable; it does not complete the implementation milestones.

| Milestone | Outcome | Primary dependencies | Release or proof |
|---|---|---|---|
| **R0 — Recover and record the baseline** | Reproducible builds, current evidence, complete placement/save inventory | Existing repository | Baseline verification record |
| **R1 — Safe authored-content foundation** | Versioned content model, read-only export, validators, IDs and equivalence fixtures | R0 | Unchanged development world represented as content |
| **R2 — First complete editing slice** | CLI, minimal GUI, safe edits, recovery, preview and game loading | R1 | **Studio Alpha 0.1** |
| **R3 — Flat starter area and placement tools** | Full current placement categories and a playable River Crossing blockout | R2 | **Studio Alpha 0.2 / starter blockout** |
| **R4 — Art pipeline and starter vertical slice** | Coherent small art set, readable UI/audio and a measured starter adventure | R3 | **Starter vertical slice** |
| **R5 — Gameplay authoring workspaces** | Items, recipes, dialogue, quests, enemies, loot and economy tools, delivered separately | R2 foundation; R3/R4 feedback | Incremental Studio feature releases |
| **R6 — Reliable beta and distribution** | Compatibility, packaging, performance and maintainable authoring workflows | R4; selected R5 tools required by the game | **Studio / game beta candidate** |
| **R7 — Elevation experiment** | Tested surfaces, transitions, targeting and save compatibility | R3 plus relevant R6 compatibility gates | Isolated elevation preview |
| **R8 — Multiple regions and world expansion** | Explicit transitions, regional ownership and measured loading strategy | R4 plus relevant R6 gates | Two connected playable regions |
| **R9 — Multiplayer feasibility and foundation** | Proven network authority and persistence prototype | R0 authority audit; R6 reliability; R8 identity decisions | Small multiplayer technical prototype |
| **R10 — Long-term game expansion** | Companions, bosses, progression breadth and optional persistent-world operations | Depends on each feature, not every earlier milestone | Feature-specific releases |

The milestone numbers organise the work; they are not a rule that every optional project must finish in sequence. Elevation and multiplayer can remain deferred while the flat game improves. Companion or boss prototypes can run locally before multiplayer exists. R5 editors should be built when the game has repeated authoring needs, rather than all at once.

## 4. Workstreams

| Workstream | Near-term focus | Later focus | Decision rule |
|---|---|---|---|
| Engine | Preserve current semantics during content extraction | Surfaces, regions, performance and network authority | Change engine behaviour only with a concrete requirement and regression evidence |
| Studio | One complete safe editing loop | Specialised content workspaces and optional MCP | Each tool must reduce a demonstrated authoring problem |
| Game | Fresh-player progression and flat geography | More encounters, regions and repeatable content | Validate with playtests before expanding content count |
| Presentation | Keep readable placeholders; prove asset import | Consistent art, animation, audio and accessibility | Pilot a small set before large production |
| Delivery | Reproducible builds, evidence and clear docs | Versioned releases, packaging, support and operations | Completion requires reproducible evidence |

## 5. R0 — Recover and record the baseline

**Goal:** Know what currently works, how to reproduce it and which assumptions a content migration must preserve.

### Deliverables

- Investigate the failed workflow using available run details and account/runner configuration. Record the cause only when supported by evidence. Re-run the existing jobs after resolving it; do not infer a code failure from an empty job trace.
- Build the current headless configuration and the supported Windows SDL3 client configuration. Record commit, operating system, compiler, CMake, SDL version, configuration, discovered tests and results.
- Trace all terrain, resources, workstations, NPCs, monster spawns, labels, quests and saved-position dependencies. Inventory source ownership and delayed creation stages.
- Record baseline world state before player creation, after player registration and after representative gameplay actions. Identify invariants rather than blindly freezing every incidental runtime value.
- Reproduce the suspected multi-step economy cycle under exact shop/crafting rules and record starting balance, inputs, outputs and net coins. Fix and regression-test a confirmed loop before balance signoff.
- Define a fresh-player test profile alongside the development profile. Record starting items, skills, tutorial state and persistence isolation.
- Reconcile the older documentation PR and existing economy/character documentation with current source. Apply the narrowly evidenced repository cleanup.

### Exit gate

A contributor can reproduce the selected baseline and distinguish any outstanding failure from an untested assumption. Placement sources and save dependencies are accounted for, and the evidence record gives a trustworthy starting point. If one platform is blocked, record the blocker and constrain subsequent claims; do not label a cross-platform release complete.

**Work packages:** ENG-001, ENG-002, GAME-001, GAME-008, DOC-001, DOC-002, STUDIO-001–004.

## 6. R1 — Safe authored-content foundation

**Goal:** Describe the existing world as external content while preserving the current runtime behaviour.

### Work order

1. Record decisions for the minimal SDL-free model, parser dependency, canonical representation, stable IDs and dependency boundaries.
2. Define coordinates, region dimensions, ground-layer semantics, authored placements, references and resource limits. Keep the actual 100×100 map distinct from illustrative fixtures.
3. Implement schema validation, canonical serialisation and stable ID mappings. Reject unsupported versions, duplicate IDs, invalid dimensions and bad references before constructing a runtime map.
4. Build a **read-only exporter** from the current definitions. Include the constructor-seeded pond, ordered terrain overlays and separate authored placement information.
5. Add semantic baseline comparisons and migration fixtures. Compare the pre-player and post-player lifecycle stages, including delayed NPCs and advanced monsters.
6. Integrate the existing content rules. Resolve canonical spawn-catalogue ownership so externally authored placements can be validated without bypassing spawn authority.

The runtime currently uses tile values for both ground and object occupancy. Export the underlying ground separately and reproduce blocking through an adapter. A resource tile marked TREE is not evidence that its base ground is a tree; ore nodes use the same occupied tile marker. Changing that internal representation is a separate decision.

### Exit gate

An exported baseline is deterministic, validated and complete. Its semantics can be compared with the existing world. Stable authored IDs are separate from runtime creation IDs, and no legacy enum is silently renumbered. Required approach tiles, spawn access and save-position compatibility checks are designed before any map can be published.

**Work packages:** STUDIO-010–012, STUDIO-014–015, STUDIO-050–051, STUDIO-053.

## 7. R2 — First complete editing slice: Studio Alpha 0.1

**Goal:** Make one real content change safely from end to end.

### Deliverables

- A feature-gated runtime loader whose unedited import matches the baseline. An explicitly selected rollback mode remains available during migration; a broken content file must not silently look like a successful load.
- Typed commands for inspection, a small paint edit, moving an existing resource, validation, diff, saving and undo/redo. Place/remove operations share the model but need not expose every entity category in the first GUI.
- A CLI with structured diagnostics, stable exit outcomes, explicit file/region targets, base hash checks and documented capabilities. Basic AI usability arrives here.
- Transactional writes with conflict detection, a lock/recheck strategy, recoverable drafts and a clear Draft → Validated → Published workflow. Validation evidence is tied to exact content and validator versions.
- A preview using shared presentation information. Exact renderer/backend selection is recorded in an ADR; authoritative validation remains headless.
- A minimal visual editor with open, inspect, select, move, small paint, undo/redo, dirty state, errors, save/reopen, pan/zoom and keyboard-accessible controls.
- A reproducible playtest launch for the exact edited content and a disposable player profile.

### Demonstration

Move the existing normal tree to a legal tile. The old tile restores its authored ground, the new tile blocks movement correctly, gathering works from an allowed approach, the preview agrees with the game, undo restores the original state and a supported saved player still loads safely. Repeat the same edit through the CLI and compare the result.

### Exit gate

All [Alpha 0.1 gates](RELEASE_GATES.md) pass. Conflict, invalid placement, interrupted save and malformed-content cases preserve the last valid publication. The GUI and CLI use the same command semantics.

**Work packages:** STUDIO-013, STUDIO-020–027, STUDIO-030, STUDIO-032–033, STUDIO-090, relevant STUDIO-052.

## 8. R3 — Placement tools and the flat River Crossing blockout

**Goal:** Use the tools to improve the actual game geography.

### Studio work

Complete current single-tile resource and station placement; named NPC and enemy spawn editing; search/filter; inspectors; selection; basic batch operations; and overlays for occupancy, approach tiles, spawn bounds and required routes. Migrate hardcoded placement categories incrementally while preserving creation order and delayed lifecycle conditions. Larger footprints are a separate runtime feature, not something the editor can make true by drawing a larger sprite.

### Game work

Build a flat River Crossing candidate from the [geography plan](../05-game/RIVER_CROSSING.md). Use the existing terrain vocabulary and explicit legal crossings. Distribute services across connected settlement edges and paths. Keep encounter risk readable, preserve a safe onboarding route and allow optional detours.

Place the existing NPC roles, gatherable tiers and stations deliberately. Proposed counts and coordinates are blockout hypotheses; revise them after observing travel, congestion, visibility and task completion. Avoid filling the whole 100×100 region just because space exists.

### Exit gate

A fresh-player playthrough can reach essential NPCs and services, gather, process materials, equip an improvement, encounter appropriate danger and return safely. Required routes remain validated. The design has no compulsory central service core. The old development fixture remains available for engine regression tests even after a new geography becomes the preferred playtest region.

**Work packages:** STUDIO-031, STUDIO-040–043, STUDIO-052, STUDIO-054; GAME-002–005.

## 9. R4 — Asset pipeline and starter vertical slice

**Goal:** Make the starter experience readable, coherent and measurable.

First import a grass tile and a tree through a versioned asset manifest. Verify pivots, transparency, atlas boundaries, sprite size, collision footprint, selection and depth order in both editor and game. Keep a diagnostic primitive-rendering mode available. Then expand to the smallest coherent terrain, character, resource, building-prop and UI set needed by River Crossing.

Add readable combat and interaction feedback, basic audio with independent volume controls, clear visual state without colour-only signalling and UI text that matches current objectives. Maintain provenance and licence metadata for every external asset. Generated concepts are references until cleaned up and validated as runtime assets.

Run a fresh-player vertical slice: learn movement and interaction, gather material, process it, make or obtain a meaningful upgrade, use banking/shop services, complete an early quest and investigate the region's next threat. Record route time, confusion, deaths, resource waits, food consumption and upgrade acquisition. The earlier 10–15 minute first-upgrade objective is a design target to test, not a measured fact.

**Exit gate:** A new tester can complete the intended loop without developer coaching. Content and presentation failures are triaged; the team has evidence to choose the next improvements. It is acceptable to revise the map or pacing instead of adding more content.

**Work packages:** STUDIO-060–063; ART-001–004; GAME-006–007.

## 10. R5 — Gameplay authoring workspaces

**Goal:** Make repeated content creation efficient after the shared foundation is useful.

| Workspace | First useful result | Required checks |
|---|---|---|
| Items and equipment | Author one real item using current supported behaviours | IDs, requirements, stats, stack rules, save compatibility |
| Recipes | Author one real recipe and demonstrate it at the correct station | Inputs/outputs, discovery, patterns, duration, skill rules and economy |
| Dialogue | Author one conversation tied to a current NPC | Reachable choices, session authority, localisation-ready text references |
| Quests | Author one complete quest through existing supported objective types | Prerequisites, objective references, rewards, progress and persistence |
| Enemies and loot | Author one encounter definition and reward table | Spawn identity, combat limits, leash, respawn, RNG and reward integrity |
| Skills, shops and economy | Compare or tune one meaningful progression loop | Prices, sinks, throughput, travel cost and buy-craft-sell checks |

Each workspace is a focused release. Its definition format should reuse the existing validators and authoritative behaviour; it should not become an unrestricted scripting engine. Build the editor around real current needs, and record unsupported behaviours explicitly.

**Exit gate:** Each shipped workspace can produce one real game definition, with tests, documentation and a safe migration path. A workspace not needed by the current game can remain planned while other releases proceed.

**Work packages:** STUDIO-070–074; optional STUDIO-044 only when multi-tile runtime interaction is deliberately implemented.

## 11. R6 — Reliable beta and distribution

**Goal:** Make the tool/game combination dependable enough for regular use outside its author's machine.

Establish a compatibility matrix for engine versions, authored schema versions, assets and player saves. Produce deterministic content manifests and repeatable distribution packages. Test installation, first launch, normal save/reload, missing assets, invalid content, update and rollback. Record known issues rather than masking them with a fallback.

Measure larger-map and heavy-content workloads before choosing chunking or caching changes. Profile tick work, pathfinding, validation, preview and texture memory. Add operational diagnostics that distinguish source content problems from runtime failures. Keep player data out of issue attachments unless deliberately sanitised.

Finish Git conflict handling, documentation checks and scoped automation. An optional MCP wrapper may expose the existing command surface; it should not introduce a second implementation or unrestricted filesystem access. Version capabilities so an AI can identify supported commands.

**Exit gate:** The chosen beta scope has reproducible builds, release evidence, compatibility tests, user workflows and rollback. Required CI checks are enabled and passing at the release commit; branch rules are deliberately configured rather than assumed.

**Work packages:** STUDIO-091–094; REL-001–003; QA-001–003.

## 12. R7 — Elevation experiment

**Goal:** Prove higher/lower ground without destabilising the flat game.

Use an isolated 24×24 test map. Introduce explicit surface identity and transitions, then test stairs/ramps, cliff blocking, diagonal corners, NPC movement, interaction and combat reach, camera selection, rendering order, respawn and save/reload. Add overlapping walkable surfaces only after basic transitions work.

Artwork that looks raised does not create elevation. A bridge drawn above water is not enough to allow a character on the bridge and another below it. Those cases need explicit positions, navigation and interaction rules.

**Exit gate:** The isolated fixture and all flat-map regression tests pass. Only then consider applying elevation to River Crossing. Keep the flat starter region viable if the experiment is postponed.

**Work packages:** STUDIO-080–084; ENG-010 where an occupancy refactor becomes necessary.

## 13. R8 — Multiple regions and world expansion

**Goal:** Connect two real playable regions with explicit identity and transition rules.

Define boundary exits, destination entry points, loading responsibilities, content references and player position ownership. Validate that exits are reciprocal where intended and that invalid destinations fail predictably. Decide how NPCs, projectiles if later introduced, drops, recovery piles and quests behave across boundaries. Measure memory and transition behaviour before adopting streaming or chunking.

**Exit gate:** A player can travel between two regions, save and reload on either side, recover safely from a removed/blocked destination and retain valid progression. A two-region implementation is not yet evidence of a large persistent online world.

**Work packages:** WORLD-001–004. Elevation is optional unless the particular region design requires it.

## 14. R9 — Multiplayer feasibility and foundation

**Goal:** Determine whether and how the current architecture can support a small authoritative multiplayer prototype.

Audit every player mutation and input path, including direct client-side changes. Design commands, sequence/identity rules, validation, snapshots, tick ownership and error handling. Prototype a dedicated runtime with two clients using a controlled local test environment. Test invalid commands, duplicate/reordered messages, disconnect/reconnect, contention and player-state persistence.

Choose protocol, transport, persistence backend and identity/security requirements through ADRs after a concrete prototype requirement exists. Do not infer production readiness from the existing command queue or fixed tick.

**Exit gate:** Two clients observe consistent authoritative results, and a documented failure experiment demonstrates recovery. Broader internet hosting, moderation, operations and capacity are separately scoped.

**Work packages:** NET-001–005.

## 15. R10 — Long-term game expansion

| Direction | Small first experiment | Evidence needed before expansion |
|---|---|---|
| Collectible companions | One companion with acquisition, following and persistence | Clear role, reliable navigation, fair progression and readable UI |
| Repeatable bosses | One encounter with a learnable mechanic and reward loop | Combat readability, recovery, repeat value and reward balance |
| Additional professions and recipes | One profession or branch that uses existing resources meaningfully | Useful inputs/sinks and an enjoyable loop |
| Larger regions and dungeons | One distinct region with a specific player purpose | Route, content and performance evidence |
| Persistent online world | A bounded hosted playtest after R9 | Authority, data integrity, operations, moderation and recovery procedures |

These are aspirations, not promised release dates. Keep features that reinforce the proven game loop; reconsider those that only enlarge maintenance cost.

## 16. First eleven executable work packages

1. ENG-001: diagnose and document the current CI failure.
2. ENG-002: reproduce baseline builds and record environment/results.
3. DOC-001: integrate this pack and narrowly scoped cleanup. **Documentation delivered through [PR #3](https://github.com/Adzffs/GameEngine/pull/3).**
4. DOC-002: reconcile old documentation and objective/ledger drift.
5. STUDIO-001: complete placement ownership inventory.
6. STUDIO-002: trace save, respawn and position compatibility.
7. GAME-001: define a genuine fresh-player profile.
8. GAME-008: reproduce the source-derived multi-step economy cycle.
9. STUDIO-003: record model/parser/target-boundary decisions.
10. STUDIO-004: capture baseline world observations and meaningful parity fixtures.
11. STUDIO-010: implement the minimal authored model after the preceding inputs are clear.

Documentation integration can proceed alongside the read-only baseline investigation. Runtime changes wait for relevant evidence. Do not begin every editor workspace in parallel.

## 17. Scheduling and scope control

No calendar dates or total completion percentage are promised in this roadmap. Contributor availability, baseline recovery and implementation unknowns have not been measured. After R0, estimate the next two or three work packages using observed delivery time and technical uncertainty. Keep later milestones as outcome-based planning.

Use one active implementation slice per contributor, with clear file ownership for parallel work. Limit work in progress. At each milestone review, inspect the demo, evidence, unresolved risks and player feedback; decide whether to continue, narrow, revise or defer the next investment.

## 18. How the roadmap evolves

Update a task status only with evidence. Preserve IDs when moving a task between milestones, record the reason and update dependencies. Archive obsolete decisions rather than silently losing them. When a release is complete, link the exact tag/commit, validation record, known issues and user documentation.

This structure lets Embervale grow while keeping the next useful action clear: **establish the baseline, then prove one safe content edit in the real game.**

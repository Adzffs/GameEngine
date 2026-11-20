# Embervale master roadmap

**Version:** 1.1 · **Status date:** 20 November 2026\
**Scope:** GameEngine, Embervale, Embervale Studio and project delivery.\
**Canonical roadmap:** This document replaces the competing phase sequences in the earlier Studio plans. The original STUDIO task IDs remain traceable in the [backlog](BACKLOG.md).

## 1. The destination

Build a coherent, expandable RPG whose world can be authored through dependable tools. The first player-facing objective is a balanced, flat River Crossing starter area with believable geography, distributed services and satisfying gathering, crafting and combat progression. The first tooling objective is a complete, reversible map-editing workflow available to both a person and an AI.

Longer-term work includes richer assets and content, multiple regions, elevation, repeatable encounters, companions and multiplayer research. Each is a separately scoped investment. None is a prerequisite for proving that the current game can support an enjoyable starter experience.

## 2. Current starting point and immediate priorities

The original engine baseline e9d32932 remains historical context. Read [current status](../01-current-state/CURRENT_STATUS.md) and the [20 November review](../06-development/PROJECT_REVIEW_2026-11-20.md). Published source now includes schemas 1-4, shared editing/history, a Python/Tk editor, guarded new drafts, River Crossing, eight art batches and walking/chopping/mining.

The review fixed recovery-file deletion after failed loads (ENG-003), crafting cursor/grid omission from saves (GAME-009) and inventory/bank stack overflow (ENG-004). Windows checks pass 102/102 SDL, 87/87 standalone headless and 42/42 Python editor tests. Automated results do not establish human playthrough or visual approval.

1. **REL-004:** reconcile/publish local melee/equipment with the newer Mining/shared-gathering implementation and matching editable sources/previews.
2. **GAME-005 / GAME-004:** automated combat/food, death/recovery and six save/reload checkpoints now pass, with the tutorial skip fixed. Preparation remains 454 ticks; actual tutorial return is 741 and the deliberate death/recovery exercise totals 2370. Finish visible/uncoached navigation, input/confusion, balance and real-time acceptance on the recovered candidate.
3. **ART-004:** finish acceptance of the implemented first sound pass: audible/device balance, Windows and synchronization with the recovered animation candidate.

The owner is not satisfied with the current animation appearance and deferred polish. Keep ART-005 open. GAME-010 periodic autosave is complete. Interrupted editor recovery, validated publication, asset-copy ownership and accessibility remain separately scoped tasks.

The original Windows integration worktree remains unavailable, so REL-004 stays **in progress**. Independent work on the restored published source has delivered the first sound pass and fixed runtime asset copying; [results and limits](../06-development/ACTION_AUDIO_2026-11-20.md). ART-004 is **in progress** with audible/device and integration acceptance open. GAME-005 is **in progress** with the [automated starter continuation](../06-development/STARTER_COMBAT_RECOVERY_2026-11-20.md) passing; human/visible acceptance remains open. The [original recovery handoff](../06-development/INTEGRATION_HANDOFF_2026-11-20.md) still applies.

GAME-010 is now **complete** for bounded periodic player persistence: 60-second checkpoints, 10-second retries, crafting/death safety, temporary-save isolation and killed-process reload. See the [autosave policy and verification](../06-development/PLAYER_AUTOSAVE_2026-11-20.md). Current Linux checks pass 112/112 SDL and 93/93 standalone headless entries, plus 42/42 Python units; current Windows/human acceptance stays open.

## 3. Roadmap at a glance

R0 and R1 remain **in progress**. The local baseline, ownership inventory, deterministic snapshot, bounded parser, runtime geography validation and selected spawn authority are implemented. R2 is **in progress** with shared move/paint/place/remove commands, history, guarded sibling drafts, the visual editor, overlays and isolated playtests implemented. Recovery, in-place publication, manifests, structured capabilities and broader accessibility remain open. The remaining baseline game tasks and broader identity/migration requirements are still open. R3 has completed bounded resource/station placement and an authored first River Crossing blockout candidate with existing actor homes; dynamic/visible/pacing acceptance and broader placement tools remain open. R4 is in progress with eight bounded world-art batches; full starter presentation and playthrough acceptance remain open. R5-R10 remain planned. See [current task progress](BACKLOG.md#current-progress) and the [runtime verification record](../06-development/RUNTIME_IMPORT_VERIFICATION_2026-11-18.md); no complete Studio release is claimed.

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

- Build the current headless configuration and the supported Windows SDL3 client configuration. Record commit, operating system, compiler, CMake, SDL version, configuration, discovered tests and results.
- Trace all terrain, resources, workstations, NPCs, monster spawns, labels, quests and saved-position dependencies. Inventory source ownership and delayed creation stages.
- Record baseline world state before player creation, after player registration and after representative gameplay actions. Identify invariants rather than blindly freezing every incidental runtime value.
- Reproduce the suspected multi-step economy cycle under exact shop/crafting rules and record starting balance, inputs, outputs and net coins. Fix and regression-test a confirmed loop before balance signoff.
- Define a fresh-player test profile alongside the development profile. Record starting items, skills, tutorial state and persistence isolation.
- Reconcile the older documentation PR and existing economy/character documentation with current source. Apply the narrowly evidenced repository cleanup.

### Exit gate

A contributor can reproduce the selected baseline and distinguish any outstanding failure from an untested assumption. Placement sources and save dependencies are accounted for, and the evidence record gives a trustworthy starting point. If one platform is blocked, record the blocker and constrain subsequent claims; do not label a cross-platform release complete.

**Work packages:** ENG-002, GAME-001, GAME-008, DOC-001, DOC-002, STUDIO-001–004.

## 6. R1 — Safe authored-content foundation

**Goal:** Describe the existing world as external content while preserving the current runtime behaviour.

**Current slice:** [Starter-world import](../04-studio/STARTER_WORLD_IMPORT.md) implements the existing 100×100 registry, geography validation and runtime compatibility path. This is a bounded starter-region contract, not arbitrary region/definition authoring.

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

**Historical implementation notes:** The [20 November review](../06-development/PROJECT_REVIEW_2026-11-20.md) supersedes the unexecuted-test notes below; manual acceptance remains open.

**19 November 2026 reconciliation:** PRs #18–#22 are merged through master
`2e3da94`. The bounded starter editor now supports move/paint, shared history,
guarded sibling saves, dirty-state warnings, save/reopen, temporary game launch,
and collision/spawn/reachability/approach overlays. These complete useful slices
of STUDIO-020–024, STUDIO-030, STUDIO-032 and STUDIO-033, not the whole R2 gate.
Crash recovery, in-place atomic publication, versioned publication manifests,
structured capabilities/diagnostics and broader accessibility remain open.
See the [master acceptance record](../06-development/MASTER_ACCEPTANCE_2026-11-19.md).
Resource/station creation and removal (STUDIO-040) are now implemented with
schema-2 allocation reservations, shared history, guarded save/reopen and a
visible game gathering acceptance. See [placement verification](../06-development/STUDIO_PLACEMENT_VERIFICATION_2026-11-19.md)
and [ADR-018](../07-decisions/ADR_018_BOUNDED_OBJECT_PLACEMENT.md).
NPC/enemy home movement, the first bounded slice of STUDIO-041, is implemented
with shared history, save/reopen and regression scenarios. The owner requested
integration without testing for now. Game/tool builds succeeded; tests and
visible acceptance are recorded as deferred, with no test pass claimed. The remaining current-runtime
spawn settings are now implemented: monster wander radius/interval, spawn cap, respawn enablement
and per-spawn delay, with a shared command/history and Studio settings dialog. Schema 3 retains
actor bindings/lifecycle and historical schema-1/2 restrictions; named NPCs remain unique and
stationary. This follow-up also builds successfully, with tests deferred at the owner's request.
See the [home movement record](../06-development/STUDIO_SPAWN_MOVE_REVIEW_2026-11-19.md) and
[spawn-settings record](../06-development/STUDIO_SPAWN_SETTINGS_2026-11-19.md).
GAME-001 now implements an explicit fresh-player profile: level-1/zero-XP skills,
bronze gathering tools, normal tutorial/quest/recipe state and no developer weapon.
`--profile fresh` has no implicit save; Studio defaults to a temporary fresh start
with a development-profile option. Existing selected saves retain their progress.
Game/tool builds passed; runtime and visible acceptance remain deferred at the
owner's request. See the [profile contract](../05-game/FRESH_PLAYER_PROFILE.md).
GAME-002/003 now deliver the first named flat River Crossing candidate with two land
crossings, connected routes, 18 resources, three stations and ten existing actor homes.
Production content validation and game/tool builds passed; dynamic/visible and pacing
acceptance remain deferred. See the [candidate and launch guide](../../../content/worlds/README.md)
and [implementation record](../06-development/RIVER_CROSSING_BLOCKOUT_2026-11-19.md).
GAME-008/GAME-004 now include the confirmed timber correction, intermediate recipe-cost validation and a twenty-coin first shield. The source-derived sword/shield material budget is two normal Logs, three Copper and three Tin, with one plank remaining. [Production replay and budget evidence](../06-development/STARTER_ECONOMY_2026-11-19.md). The starter journey now uses recipe-derived ore/bar/sword readiness, live destination and tool hints, and an H/click material and quest plan. An unfunded production route gathers, crafts, equips, banks, claims rewards and buys food in 451 simulation ticks; [route and guidance evidence](../06-development/STARTER_UPGRADE_JOURNEY_2026-11-19.md). Next acceptance is uncoached navigation and progression, combat/recovery food pressure, visible interaction and human elapsed upgrade time. The 10-15 minute target remains unverified; regression suites remain deferred.

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

GAME-007 now implements the bounded narrative/quest continuity pass: names and reward rules agree with source, the next quest follows the tutorial, and full details identify both reward contacts. Builds and compiled content validation passed; [implementation and deferred acceptance](../06-development/QUEST_CONTINUITY_2026-11-19.md). Eight bounded LibreSprite batches now render grass/paths, normal tree/stump, player/Mara/furnace, water/banks/stone/copper/tin and bank/table sprites, plus a modular building kit and the first workshop. Contextual variation adds coherent grass patches, maintained/worn/overgrown paths, shallow/deep water, stone conditions, clustered wildflowers, reeds, grass clumps and leaf litter under real trees. Building conditions include new, weathered and repaired surfaces, plus terracotta/slate kit materials; the workshop uses localized repairs. Directional wall caps/windows and corners now form a coherent room, with interior front cutaways. The shared cache covers 1,175 frames across 30 PNGs, with editable masters and reproducible default exports. Client/launcher builds and actual software-renderer captures passed. Bank/shop building assemblies, remaining cast/tiers, audio and general importing remain open. See the [wall-orientation record](../06-development/WALL_ORIENTATION_2026-11-19.md). The 20 November follow-up adds eight-way walking, woodcutting and mining, bronze/iron/steel tool visuals, interruption handling and reward feedback from committed world events. [Woodcutting implementation](../06-development/WOODCUTTING_ANIMATION_2026-11-20.md) and [latest mining record and local checks](../06-development/MINING_ANIMATION_2026-11-20.md). Broader R4 playthrough and presentation acceptance are still required.

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

**Exit gate:** The chosen beta scope has reproducible builds, release evidence, compatibility tests, user workflows and rollback. The release commit has recorded local build and regression results for the chosen platforms.

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

## 16. Current executable work order

| Order | Work | Observable result |
|---|---|---|
| Completed in this review/follow-ups | ENG-003, ENG-004, GAME-009, GAME-010, QA-004 | Integrity fixes, periodic player saves and current verification published |
| Next | REL-004 | Combined published Mining/melee/equipment and matching assets/previews |
| Then | GAME-005 and GAME-004 | Fresh route with real elapsed time, confusion, food/combat/recovery findings |
| During starter acceptance | ART-004 | First sound pass implemented; finish audible/device review and synchronization with the recovered animation candidate |
| Reliability work | STUDIO-025/026, STUDIO-063, QA-003 | Editor recovery/publication, packaging and visible usability |
| Owner-deferred | ART-005 | Posing, weight, timing/transitions and visual acceptance |
| As concrete changes require | ENG-005/006 | Progression/health integer policy and targeted responsibility extraction |

GAME-005 supplies acceptance evidence to the progression/map tasks. Requiring their human pacing acceptance before measuring the journey would make that workflow circular. QA-001/DOC-003 apply to each slice and do not wait for R6 packaging.

The 10-15 minute goal remains a hypothesis. Earlier 451-tick data stays in its dated report; current prepared return is tick 454. Neither is an uncoached player time.

## 17. Scheduling and scope control

No calendar dates or total completion percentage are promised in this roadmap. Contributor availability, baseline recovery and implementation unknowns have not been measured. After R0, estimate the next two or three work packages using observed delivery time and technical uncertainty. Keep later milestones as outcome-based planning.

Use one active implementation slice per contributor, with clear file ownership for parallel work. Limit work in progress. At each milestone review, inspect the demo, evidence, unresolved risks and player feedback; decide whether to continue, narrow, revise or defer the next investment.

## 18. How the roadmap evolves

Update a task status only with evidence. Preserve IDs when moving a task between milestones, record the reason and update dependencies. Archive obsolete decisions rather than silently losing them. When a release is complete, link the exact tag/commit, validation record, known issues and user documentation.

This structure lets Embervale grow while keeping the next useful action clear: **protect progression, reconcile the builds, observe the fresh starter loop and finish audible acceptance of the implemented sound pass.**

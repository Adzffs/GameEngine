# Design decisions and architecture decision register

**Purpose:** Preserve why choices were made and distinguish agreed direction from implementation proposals.\
**Status date:** 19 November 2026. Use the [ADR template](../08-templates/ADR_TEMPLATE.md) for decisions that need more detail.

## 1. Decision status

**User direction** records an explicit project preference. **Proposed** describes a recommended technical approach awaiting evidence and adoption. **Accepted in implementation** requires a linked implementing change. **Superseded** retains an earlier decision and links its replacement. This document does not invent approvals or assume that a planning preference has already become code.

## 2. Established product direction

| ID | Decision | Status | Consequence |
|---|---|---|---|
| DEC-001 | Balanced starter area with gathering, crafting, exploration, settlement activity and danger | User direction | Assess the whole player loop, not just geometric appearance |
| DEC-002 | No compulsory central hub/core | User direction | Distribute services along useful routes and make circulation legible |
| DEC-003 | Develop River Crossing as the settlement direction | User direction | Prototype geography; exact coordinates remain proposals until playtested |
| DEC-004 | Make tools usable visually and by AI | User direction | Shared command/validation semantics; basic automation belongs early |
| DEC-005 | Flat-ground capability is the current reality | Source verified and user constraint | Elevation requires an isolated engine feature milestone |
| DEC-006 | Make documentation comprehensive, organised and expandable; publish it cleanly to GitHub | User direction | One canonical index/roadmap, evidence-based status and focused repository changes |

## 3. Technical decision register

| ADR | Topic | Current recommendation/status | Decision trigger |
|---|---|---|---|
| ADR-001 | Authored definitions versus runtime state | Proposed: separate content from depletion, live NPC positions and player state | Before model implementation |
| ADR-002 | Shared content and command semantics | Proposed: one headless interpretation, avoid dependency cycles | Before choosing CMake targets |
| ADR-003 | Content representation/parser | Implemented for the current ASCII snapshot in ADR-017; general schema evolution remains open | Before widening input scope |
| ADR-004 | Stable identity | Proposed: durable authored IDs plus explicit enum/legacy mapping | Before exporting external references |
| ADR-005 | Ground and occupancy migration | Starter adapter implemented: authored ground first, legacy occupancy second | Before broader occupancy changes |
| ADR-006 | Canonical ordering versus lifecycle | Starter importer preserves explicit stages/order independently of array sorting | Before adding placement lifecycle kinds |
| ADR-007 | Safe edits and concurrent writes | Proposed: typed transactions, content hash, lock/recheck, atomic replacement | Before persistent editor changes |
| ADR-008 | Drafts, validation and publication | Proposed: separate recoverable drafts and immutable validated packages | Before save/recovery UI |
| ADR-009 | Editor packaging | Open: separate executable or embedded mode, based on dependency and UX evidence | Before GUI target implementation |
| ADR-010 | Preview renderer | Open: share interpretation/palette; choose backend with reproducible local evidence | Before preview acceptance |
| ADR-011 | Assets and provenance | Proposed: stable manifest IDs, source assets separate from derived atlas | Before first import |
| ADR-012 | Surface and region identity | Proposed: reserve ground identity; implement transitions separately | Before schema freezes/elevation |
| ADR-013 | Save migration | Proposed: preserve current format when possible; explicit registry for changes | Before content affects persisted references |
| ADR-014 | Network architecture | Open: use prototype requirements; do not infer MMO readiness | R9 feasibility phase |
| ADR-015 | Repository and asset licensing | Open owner decision; no licence invented by documentation | Before distribution/contributor intake |
| [ADR-016](ADR_016_READ_ONLY_WORLD_EXPORT.md) | Read-only snapshot before import | Original export decision; runtime boundary subsequently extended by ADR-017 | Retain compiled baseline export |
| [ADR-017](ADR_017_BOUNDED_STARTER_WORLD_IMPORT.md) | Bounded explicit runtime import | Implemented starter parser/validator, shared core, selected spawn catalogue and isolated playtests | Shared editing/publication remains separate |
| [ADR-018](ADR_018_BOUNDED_OBJECT_PLACEMENT.md) | Bounded resource/station membership | Implemented schema-2 allocations, shared place/remove/history and guarded drafts | Larger footprints and actor membership remain separate |
| [ADR-019](ADR_019_BOUNDED_SPAWN_SETTINGS.md) | Existing monster spawn settings | Implemented schema-3 wander/cap/respawn editing; game/tool builds passed, runtime/visible checks deferred | New actor identities, friendly wandering and population filling remain separate |

| [ADR-020](ADR_020_STATIC_BUILDING_SHELLS.md) | Static building shells | Schema-4 rectangular shells, shared wall/door collision, readonly Studio footprint and independent roof visibility | More building styles, entrances and authoring controls remain separate |

## 4. Rationale for the key proposals

### One content interpretation

The editor should not maintain a second collision, spawn or quest rule set. A shared content model/codec can serve runtime and authoring, while authoring commands call the same relevant validators. Exact library boundaries must avoid a runtime-to-Studio-to-runtime cycle. SDL-dependent UI/rendering belongs outside the headless model and authoritative rules.

### Compatibility before redesign

The current engine combines ground and occupied tile markers and has lifecycle-dependent actor creation. A small adapter that reproduces this behaviour is easier to verify than combining extraction with a broad world rewrite. A later occupancy refactor remains possible, but must preserve the tested flat baseline.

### Durable identity

Coordinates and names are editable properties. Runtime IDs can be reassigned on loading or generated by creation order. An authored identity must survive those changes and must not be silently reused for an unrelated object. Mapping, aliasing, deletion and unsupported-reference rules belong in a registry with tests.

### Recovery separate from publication

A designer may temporarily break a route while making a legitimate multi-step change. Saving that work as a draft is useful. Selecting it as game content requires the full applicable validity checks. Validation binds to exact semantic content and tool/profile versions; a later edit invalidates prior proof.

### Measured scaling

Current map size and workload provide a real baseline. Chunking, streaming, caching and network architecture should solve demonstrated problems. Profile the relevant operations before deciding an optimisation or imposing an arbitrary performance budget.

## 5. Decisions still needing a concrete answer

The starter input uses bounded ASCII JSON with terrain rows. General schema evolution, preview backend, GUI packaging and eventual network transport remain open. The proposed River Crossing coordinates are a blockout candidate rather than an approved final map. Character and objective naming drift needs an intentional narrative choice. The confirmed outcome of the suspected economy loop may require price, recipe, stock or progression changes; the design should be chosen after reproduction.

Open does not mean work must stop for every minor implementation choice. A contributor can make routine reversible decisions within the agreed scope and document them. Escalate a choice when it changes the user experience, compatibility contract or a major investment.

## 6. Recording and changing a decision

Use a stable ADR number, problem statement, evidence, considered alternatives, decision, consequences, validation and linked implementation. When changing it, keep the original record marked superseded and explain why the evidence changed. Do not let implementation details silently override the no-central-core, flat-first or AI-accessible requirements.

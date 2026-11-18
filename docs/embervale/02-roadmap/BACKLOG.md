# Implementation backlog

**Version:** 1.0 · **Reviewed:** 18 November 2026\
**Purpose:** Turn the roadmap into clear, expandable work packages. These are planning records, not automatically created GitHub issues.

## How to use this backlog

P0 means a prerequisite or integrity risk that needs early resolution; P1 is required for the named release; P2 is a valuable subsequent improvement; P3 is optional or longer-term work. These are planning priorities rather than security severity ratings. Current implementation progress is recorded below; unlisted tasks remain planned. DOC-001 is complete for delivery of this documentation pack and focused cleanup through [PR #3](https://github.com/Adzffs/GameEngine/pull/3); it does not complete the engine/editor milestones. GitHub records the PR merge state.

The acceptance column states what must be demonstrated. When opening a GitHub issue, copy the task into the [feature template](../08-templates/FEATURE_SPEC.md), add exact source and test references, estimate only that bounded task, and record the actual issue URL. Split a task if the acceptance result cannot be reviewed in a focused change. Keep the ID stable even if its milestone moves.

Each dependency below is a direct prerequisite, not a suggestion that every earlier milestone must finish. The machine-readable [BACKLOG.json](BACKLOG.json) contains the same task records for future automation; it does not execute work or create issues.

## Current progress

The export foundation is followed by bounded parsing, geographic validation and explicit runtime import of the existing starter region. `in_progress` retains larger acceptance obligations; implementation below is not a claim that R1, R2 or Studio Alpha 0.1 is complete. [Runtime import](../04-studio/STARTER_WORLD_IMPORT.md) and [ADR-017](../07-decisions/ADR_017_BOUNDED_STARTER_WORLD_IMPORT.md) document the current scope. The [runtime verification record](../06-development/RUNTIME_IMPORT_VERIFICATION_2026-11-18.md) records 77/77 headless tests, 89/89 Linux SDL tests, a content-tools-disabled check and a real dummy-video game launch. The earlier [baseline/export record](../06-development/BASELINE_VERIFICATION_2026-11-18.md) remains separately dated.

| Work | Planning IDs | Status | Evidence and remaining work |
|---|---|---|---|
| Reproducible baseline | ENG-002 | in_progress | Linux runtime slice 77/77 headless and 89/89 SDL; Windows execution and visible playtesting remain unverified |
| Ownership and saved-location inventory | STUDIO-001, STUDIO-002 | ready_for_review | Full placement/lifecycle and save-position paths documented; general migration remains separate |
| Minimal architecture | STUDIO-003 | in_progress | Bounded parser, shared core, explicit input and world-local spawn decisions recorded; preview/backend remains open |
| Semantic baseline fixtures | STUDIO-004 | ready_for_review | Existing full-grid export fixture extended by imported-world construction, stages and interaction tests |
| Authored starter model | STUDIO-010 | in_progress | Terrain, coordinates and selected player spawn can vary within the exact starter registry; arbitrary identity/region authoring remains unsupported |
| Bounded codec | STUDIO-011 | in_progress | Strict ASCII schema-1 parser, canonical writer, resource budgets and round-trip/rejection tests implemented; broader schema evolution remains open |
| Read-only current-world export | STUDIO-012 | ready_for_review | Complete compiled baseline export is retained; input/runtime additions do not change its output contract |
| Selected content authority | STUDIO-014 | in_progress | Per-world validated spawn catalogue preserves existing type/capacity/lifecycle checks; other definition catalogues remain compiled |
| Identity/migration registry | STUDIO-015 | in_progress | Exact 20 stable IDs and legacy bindings enforced; allocation, aliases, removals and migrations remain future work |
| Static geography and access | STUDIO-050, STUDIO-051 | in_progress | Bounds, overlaps, water, clear player spawn, reachable actors and cardinal approaches validated; general regions/exits remain unsupported |
| Position compatibility | STUDIO-053 | in_progress | Selected safe spawn and saved-player/recovery handling are integrated for this region; broader content/save migration remains open |
| Runtime compatibility adapter | STUDIO-013 | in_progress | Opt-in snapshot loading preserves ground, occupancy and staged creation; explicit compiled mode, temporary playtests and save-path guards are implemented |
| Headless command surface | STUDIO-022 | in_progress | Export, validation and resource/station move dry-run commands exist; guarded writes and structured machine diagnostics remain planned |
| Create-only guarded draft writer | STUDIO-021 | in_progress | Source-byte SHA-256, exclusive source lock, read/recheck, same-directory new-file publication; limited isolated verification only, not in-place replacement or full platform acceptance |
| Shared placement move and terrain paint | STUDIO-020 | in_progress | Existing resources/stations can move by stable ID; a new single-tile G/D/S/W terrain paint command validates the entire candidate and supports guarded draft output; the stacked terminal session now includes terrain paint within the same undo/redo history as moves. Full stacked build, authoritative viewport integration and broader content edits remain open |
| Bounded command undo/redo | STUDIO-023 | in_progress | In-memory resource/station move batches have inverse replay and semantic conflict detection; full project verification, persistence and UI remain open |
| Interactive map-preview/session | STUDIO-022, STUDIO-024 | in_progress | Terminal map/list session now includes typed movement, painting, undo/redo, authoritative terrain rows and create-only guarded saves; complete integration verification still open |
| Mouse-driven visual editor | STUDIO-030 | in_progress | Stacked Python/Tk viewport supports stable-ID selection/movement, terrain brush, zoom, history and protected draft saves. C++ supplies the authoritative map and placement state; the bridge fails closed on source placement kind/coordinate drift, illegal post-edit placement types/positions and changed read-only actor spawn positions. Additional source-verified bridge transition and observed-state history checkpoint guards now reject incorrect undo/redo replies and stale redo branches; three new mocked regressions are committed. Six earlier isolated history checks and eight receipt-validator checks passed; backend save acknowledgements must now identify the requested draft and match its on-disk SHA-256. The new committed bridge regressions and real C++/Tk integration are not yet executed |

The current editor stack ends at draft PR #18, which depends on draft PRs #9–#17. The next priority is **building and testing the complete stacked code** with real C++ editing services, then correcting any integration defects and consolidating dependencies before extending authoring scope. The guarded draft writer only publishes fresh sibling files; in-place replacement, durable recovery and full platform acceptance remain open. A parser or a successful load alone does not demonstrate a complete authoring product. Fresh-player and economy work also remain separate.

## Complete work inventory

### R0

| Planning ID | Priority | Work package | Depends on | Acceptance result |
|---|---|---|---|---|
| ENG-002 | P0 | Reproduce baseline builds | None | Record compiler/CMake/SDL/platform versions, configure/build commands, test discovery and actual headless/client/graphics outcomes. |
| DOC-001 | P1 | Publish documentation and focused cleanup | None | Add the indexed pack on a dedicated branch/PR, reconcile older docs, list exact removals, validate links and preserve useful existing material. |
| DOC-002 | P1 | Resolve source and documentation drift | DOC-001 | Correct the ledger; decide the final quest target wording; separate old PR findings from current verified issues; record remaining decisions. |
| STUDIO-001 | P0 | Inventory all placement ownership | ENG-002 | Trace ground, object, station, NPC, monster, label, quest and initialization ownership with source links. |
| STUDIO-002 | P0 | Audit position and save compatibility | STUDIO-001 | Cover player creation/loading/death/respawn, recovery piles, spawn catalogue checks, interaction approach tiles and historical saves. |
| STUDIO-003 | P1 | Decide minimal content architecture | STUDIO-001; STUDIO-002 | Record parser, schema representation, target dependencies, coordinates and preview design ADRs with consequences. |
| STUDIO-004 | P0 | Capture semantic baseline fixtures | STUDIO-001; STUDIO-002 | Snapshot terrain/occupancy and lifecycle stages; define deterministic behavioural checks without overfitting transient IDs. |
| GAME-001 | P0 | Define a genuine fresh-player profile | ENG-002 | Specify non-boosted skills/items/tutorial state and disposable saves; distinguish it from current development defaults. |
| GAME-008 | P0 | Reproduce multi-step shop arbitrage | ENG-002 | Replay the source-derived Log→planks→shield chain; record actual net coins, constraints and stock behaviour; correct/test confirmed loops before balance signoff. |

### R1

| Planning ID | Priority | Work package | Depends on | Acceptance result |
|---|---|---|---|---|
| STUDIO-010 | P1 | Define authored region model | STUDIO-003; STUDIO-004 | Represent region identity, dimensions, ground, placements, definition references and ground surface without changing current gameplay. |
| STUDIO-011 | P1 | Implement bounded codec and canonical writer | STUDIO-010 | Round-trip valid fixtures deterministically; reject malformed/oversized/unsupported/duplicate content before allocation or map writes. |
| STUDIO-012 | P1 | Export current world read-only | STUDIO-011; STUDIO-004 | Export actual 100×100 definitions, pond, overlays, placements and lifecycle metadata; compare complete semantic baseline. |
| STUDIO-014 | P1 | Integrate content validation providers | STUDIO-010; STUDIO-002 | Use existing rule semantics; establish one canonical definition provider and preserve spawn authority checks. |
| STUDIO-015 | P1 | Maintain stable ID and migration registry | STUDIO-010 | Map authored IDs to current enums, forbid silent reuse/renumbering, document aliases/removals and test references. |
| STUDIO-050 | P1 | Validate static placement safety | STUDIO-011; STUDIO-014 | Reject out-of-bounds/overlap/bad-reference/blocked-spawn cases using authoritative occupancy rules and precise diagnostics. |
| STUDIO-051 | P1 | Validate mandatory route and approach access | STUDIO-050 | Use actual diagonal and interaction semantics to prove required services, spawn and exits reachable; test false-positive traps. |
| STUDIO-053 | P1 | Define map-update position compatibility | STUDIO-002; STUDIO-015 | Specify and test safe player/respawn/recovery-data handling when geometry changes, preserving unrelated progress. |

### R2

| Planning ID | Priority | Work package | Depends on | Acceptance result |
|---|---|---|---|---|
| STUDIO-013 | P1 | Load content through compatibility adapter | STUDIO-012; STUDIO-014; STUDIO-050; STUDIO-051; STUDIO-053 | Match baseline terrain/occupancy/lifecycle and save behaviour; make rollback explicit and invalid file failure observable. |
| STUDIO-020 | P1 | Implement shared edit operations | STUDIO-010; STUDIO-050 | Typed inspect/paint/place/move/remove/validate/diff/save operations produce a candidate and affected references; first release exposes bounded scope. |
| STUDIO-021 | P1 | Guard writes and dry runs | STUDIO-020; STUDIO-011 | Expected content hash, file locking/recheck and atomic replacement prevent stale writes; dry-run persists nothing. |
| STUDIO-022 | P1 | Provide headless CLI | STUDIO-020; STUDIO-021 | Document supported syntax, structured diagnostics and exit outcomes; tests operate without SDL. |
| STUDIO-023 | P1 | Add undo and redo | STUDIO-020 | Reverse complete commands/batches while retaining authored identity; reject incompatible history after external changes. |
| STUDIO-024 | P1 | Produce reproducible previews | STUDIO-013 | Preview true content, occupancy and selected overlays; share interpretation and version rendering inputs. |
| STUDIO-025 | P1 | Recover separately stored drafts | STUDIO-021; STUDIO-023 | Recover last valid draft after interruption; distinguish draft/base/published states and handle conflicts without overwriting publication. |
| STUDIO-026 | P1 | Implement validated publication selection | STUDIO-013; STUDIO-021; STUDIO-050; STUDIO-051 | Bind validation to exact content/validator versions; publish immutable manifest and load its selected version in a fresh session. |
| STUDIO-027 | P1 | Make batch transactions conflict-safe | STUDIO-020; STUDIO-021 | Validate final candidate atomically; reject partial application, unexpected file changes and unsafe paths. |
| STUDIO-030 | P1 | Build minimal visual editor shell | STUDIO-022; STUDIO-024 | Open/select/inspect/move/paint with shared commands; preserve camera and coordinate transforms. |
| STUDIO-032 | P1 | Handle file state and recovery visibly | STUDIO-025; STUDIO-030 | Show dirty state, save/reopen, unsaved work and conflicts; demonstrate undo/redo and recovery. |
| STUDIO-033 | P1 | Provide essential overlays and access | STUDIO-030; STUDIO-050 | Grid, collision, spawn and approach overlays; keyboard focus, visible selection and text diagnostics. |
| STUDIO-090 | P1 | Publish machine-readable capabilities | STUDIO-022 | AI can inspect supported command/schema versions, dry-run a scoped change and interpret conflict/validation outcomes. |

### R3

| Planning ID | Priority | Work package | Depends on | Acceptance result |
|---|---|---|---|---|
| STUDIO-031 | P2 | Expand terrain tools | STUDIO-030; STUDIO-023 | Add bounded brushes, rectangle/fill, erase and selection; batch operations use one reversible command. |
| STUDIO-040 | P1 | Edit current resources and stations | STUDIO-013; STUDIO-030 | Move/place/remove supported single-tile definitions while preserving approach access and restored base ground. |
| STUDIO-041 | P1 | Edit NPC and monster spawn definitions | STUDIO-014; STUDIO-015; STUDIO-030 | Edit home/wander/cap/respawn fields allowed by runtime; preserve delayed lifecycle, quest references and authority. |
| STUDIO-042 | P2 | Add placement inspection and dependency views | STUDIO-040; STUDIO-041 | Search/filter/select/inspect current placements and warn of referenced content before moves/deletions. |
| STUDIO-043 | P1 | Migrate hardcoded placement categories | STUDIO-040; STUDIO-041; STUDIO-004 | Migrate category by category; retain baseline parity and mapping/creation-order obligations until explicitly changed. |
| STUDIO-052 | P1 | Automate edited-world smoke scenarios | STUDIO-013; STUDIO-022 | Exercise gathering, crafting, NPC interactions, banking, combat and save/reload on the exact content version; Alpha 0.1 uses its relevant subset. |
| STUDIO-054 | P2 | Measure geography and show diagnostics | STUDIO-024; STUDIO-051 | Record route distance/time, resource access and validation hotspots using documented, reproducible scenarios. |
| GAME-002 | P1 | Build flat River Crossing geography | STUDIO-040; GAME-001 | Create a named candidate with distributed routes, safe onboarding, legal crossings and no compulsory central core. |
| GAME-003 | P1 | Place services and NPCs deliberately | GAME-002; STUDIO-041 | Place current NPC/station roles and monster records along appropriate routes; validate required approaches, actual wander/aggression/leash behaviour and safe escape routes; align lore/location references. |
| GAME-004 | P1 | Tune first resource and upgrade loop | GAME-001; GAME-008; GAME-003 | Measure gathering, inventory, travel, bank/shop and crafting costs; compare actual fresh-player outcomes with stated targets. |
| GAME-005 | P1 | Run structured starter blockout playtests | GAME-002; GAME-003; GAME-004 | Capture uncoached navigation/task results, confusion and failure points; implement and retest priority fixes. |

### R4

| Planning ID | Priority | Work package | Depends on | Acceptance result |
|---|---|---|---|---|
| STUDIO-060 | P1 | Introduce asset identity and provenance | STUDIO-015 | Manifest names source/licence/author/path/version and stable IDs; missing references are diagnosed. |
| STUDIO-061 | P1 | Render sprites with cache and fallback | STUDIO-060 | One tile/tree appear in game and preview with correct pivot/scale/transparency; retain debug primitive mode. |
| STUDIO-062 | P2 | Import and preview sprite metadata | STUDIO-061; STUDIO-030 | Author animations/variants through validated metadata; verify atlas coordinates and render ordering. |
| STUDIO-063 | P1 | Validate image and atlas resources | STUDIO-060 | Reject invalid dimensions/rectangles/frames and excessive resource usage; record memory costs. |
| ART-001 | P1 | Choose a small coherent art pilot | GAME-005; STUDIO-060 | Compare style at real game scale using grass/tree/player/prop examples; record palette and readability decisions. |
| ART-002 | P2 | Produce starter visual kit | ART-001; STUDIO-061 | Complete only the terrain, cast, resources and props needed by the measured starter slice with provenance. |
| ART-003 | P2 | Improve readable UI and animation | ART-001; GAME-005 | Confirm selection, hit feedback, names, objectives and UI scale using real tasks and accessibility checks. |
| ART-004 | P2 | Introduce a restrained audio set | ART-001 | Add meaningful feedback and independent volume controls; document audio rights, formats and missing-asset behaviour. |
| GAME-006 | P1 | Deliver starter vertical slice | GAME-005; ART-002; ART-003; GAME-007 | New tester completes intended gathering/crafting/combat/quest loop; record recovery, pacing and confusion results. |
| GAME-007 | P1 | Reconcile narrative and progression text | DOC-002; GAME-003 | Current quest text, NPC names, objective targets, rewards and environmental cues agree on the tested journey. |

### R5

| Planning ID | Priority | Work package | Depends on | Acceptance result |
|---|---|---|---|---|
| STUDIO-070 | P2 | Author quests using supported behaviours | STUDIO-015; STUDIO-026; GAME-006 | One real quest is authored, validated, played and saved/reloaded; prerequisites and rewards remain authoritative. |
| STUDIO-071 | P2 | Author dialogue graphs | STUDIO-015; STUDIO-026; GAME-007 | One NPC conversation has valid reachable choices, session constraints and quest integration. |
| STUDIO-072 | P2 | Author items, equipment and recipes | STUDIO-015; STUDIO-026; GAME-004 | One item/recipe works with current requirements, patterns, stations, discovery, saves and economy checks. |
| STUDIO-073 | P2 | Author monsters and loot | STUDIO-015; STUDIO-026; STUDIO-041 | One encounter preserves spawn/AI/respawn/RNG/reward semantics and passes a bounded playtest. |
| STUDIO-074 | P2 | Support skill, shop and economy analysis | GAME-008; STUDIO-072 | Deterministic models cover input chains, sources/sinks, travel and anti-arbitrage, with assumptions visible. |
| STUDIO-044 | P3 | Add real multi-tile interaction footprints | STUDIO-040; STUDIO-051 | Runtime, pathfinding, interaction, selection and editor agree on footprints/approaches; sprite dimensions alone never create collision. |

### R6

| Planning ID | Priority | Work package | Depends on | Acceptance result |
|---|---|---|---|---|
| STUDIO-091 | P3 | Optionally wrap commands in MCP | STUDIO-090; STUDIO-027 | Expose the same validated operations with explicit workspace scope and capability versions; CLI remains usable independently. |
| STUDIO-092 | P2 | Improve Git-aware review and conflict workflow | STUDIO-021; STUDIO-027; DOC-003 | Show changed identities and dependencies; resolve Git conflicts through revalidation rather than coordinate-only auto-merging. |
| STUDIO-093 | P1 | Version content packages and migrations | STUDIO-026; STUDIO-015 | Publish engine/schema/asset/save compatibility matrix, migration tests and reversible package selection. |
| STUDIO-094 | P2 | Profile representative scale and contention | STUDIO-054; GAME-006 | Measure tick/route/validation/render/memory workloads before selecting chunking; document local simulation limits. |
| QA-001 | P1 | Maintain local regression and content checks | STUDIO-052; STUDIO-093 | Run meaningful local tests and content checks at the release commit; preserve failure logs and report skipped groups. |
| QA-002 | P2 | Exercise malformed content and failure recovery | STUDIO-011; STUDIO-021; STUDIO-093 | Bounded negative/fuzz/fault scenarios demonstrate safe failure and preserved previous content/player data. |
| QA-003 | P2 | Verify accessibility and usability | ART-003; STUDIO-033; GAME-006 | Record keyboard, focus, colour-independent state, readable scale and error recovery results with fixes. |
| REL-001 | P1 | Define versioned release metadata | STUDIO-093 | Tie engine/content/schema/assets/save compatibility and known issues to a reproducible release commit. |
| REL-002 | P1 | Package and verify clean installation | REL-001; QA-001 | Install/run on supported clean environments, verify dependencies, first launch, save location, update and rollback. |
| REL-003 | P1 | Establish release and maintenance ownership | REL-001; DOC-003 | Release checklist, issue triage, rollback decision and documentation freshness have named maintainers or explicit unassigned roles. |
| DOC-003 | P2 | Keep documentation synchronized | DOC-001 | Update affected inventory/decisions/user workflows with code changes; check links and archive superseded plans. |

### R7

| Planning ID | Priority | Work package | Depends on | Acceptance result |
|---|---|---|---|---|
| ENG-010 | P2 | Separate runtime ground and occupancy when required | STUDIO-013; STUDIO-004 | Preserve flat-world semantics while enabling a demonstrated new requirement; independent parity and rollback. |
| STUDIO-080 | P3 | Introduce tested surface metadata | STUDIO-093; STUDIO-015 | Flat maps remain unchanged; surface positions and unsupported values are validated. |
| STUDIO-081 | P3 | Implement explicit surface transitions | STUDIO-080; ENG-010 | 24×24 fixture proves stairs/cliffs/diagonals and NPC navigation with authoritative transition rules. |
| STUDIO-082 | P3 | Render and select elevated content | STUDIO-081; STUDIO-061 | Camera, occlusion, draw order and clicks identify correct surface at overlaps. |
| STUDIO-083 | P3 | Support overlapping walkable surfaces | STUDIO-081; STUDIO-082 | Bridge/floor/tunnel fixture differentiates occupants above/below and rejects unintended cross-layer movement. |
| STUDIO-084 | P3 | Migrate interactions and saves for surfaces | STUDIO-080; STUDIO-053 | Combat, gathering, shops, respawn, recovery and old saves remain correct across surfaces before rollout. |

### R8

| Planning ID | Priority | Work package | Depends on | Acceptance result |
|---|---|---|---|---|
| WORLD-001 | P3 | Define region identity and transition contract | STUDIO-015; STUDIO-093 | Explicit exits, destinations, loading ownership and cross-region reference rules are documented and validated. |
| WORLD-002 | P3 | Connect two playable regions | WORLD-001; GAME-006 | Player travels both ways and retains valid state; broken destination produces defined recovery. |
| WORLD-003 | P3 | Handle regional saves and location-bearing state | WORLD-001; STUDIO-053 | Reload and recovery work in both regions; map updates cannot silently strand player or saved ground data. |
| WORLD-004 | P3 | Measure world-loading strategy | WORLD-002; STUDIO-094 | Profile real transition/memory workloads before adding streaming or chunks; record chosen approach. |

### R9

| Planning ID | Priority | Work package | Depends on | Acceptance result |
|---|---|---|---|---|
| NET-001 | P3 | Audit complete command authority | ENG-002; STUDIO-093 | Inventory every player mutation including direct running toggles; define trust boundaries and authoritative command ownership. |
| NET-002 | P3 | Choose protocol and identity model by ADR | NET-001; WORLD-001 | Specify bounded messages, sequence rules, snapshots, authentication needs and persistence identity without assuming an untested stack. |
| NET-003 | P3 | Build two-client authoritative prototype | NET-002 | Two controlled clients observe one authoritative simulation and consistent interaction outcomes. |
| NET-004 | P3 | Test network failure and contention | NET-003 | Reject invalid/duplicate/stale commands; demonstrate disconnect/reconnect and resource contention behaviour. |
| NET-005 | P3 | Decide hosted-playtest readiness | NET-004; REL-003 | Record security, recovery, observability, moderation and capacity requirements before public access. |

### R10

| Planning ID | Priority | Work package | Depends on | Acceptance result |
|---|---|---|---|---|
| FUT-001 | P3 | Prototype a collectible companion | GAME-006; STUDIO-073 | One acquisition/follow/persistence loop has a clear player purpose and reliable navigation; multiplayer is optional. |
| FUT-002 | P3 | Prototype a repeatable boss | GAME-006; STUDIO-073 | One readable encounter demonstrates repeat value, recovery and balanced rewards. |
| FUT-003 | P3 | Expand professions and regions selectively | GAME-006; STUDIO-074 | Each new resource/profession/region has demonstrated useful inputs, sinks and player decisions. |
| FUT-004 | P3 | Scope persistent online operations | NET-005 | A separately approved hosted scope defines data integrity, support, recovery, moderation and operating responsibilities. |

## Original plan reconciliation

All STUDIO IDs present in the earlier reviewed plan are retained. Their order is updated to put safety and compatibility before editable publication. STUDIO-050/051/053 move to the foundation; STUDIO-090 moves to the first CLI release. STUDIO-004/015/025/026/027 make baseline evidence, identity migration, draft recovery, publication and batch conflict handling explicit. STUDIO-044 separates real multi-tile runtime support from ordinary single-tile placement editing. The former M0–M9 sequence is historical; R0–R10 in the master roadmap is canonical.

The first release retains a bounded GUI and the actual-tree demonstration. Broad brushes, full placement categories and gameplay workspaces arrive through subsequent tasks. Renderer choice, broader schema evolution and embedded-versus-separate Studio packaging remain ADR decisions, not instructions to start all approaches.

## Handling partial work and new findings

A task with code written but no acceptance evidence stays in progress or ready for review. If an implementation reveals a new risk, link a new issue or dependency rather than quietly weakening the gate. A bug discovered in this documentation review is not automatically fixed by describing it. GAME-008 still needs runtime reproduction.

For FUT-003, WORLD-002 and WORLD-003 become required dependencies when the selected expansion adds connected regions.

## Task completion record

On completion, add implementation PR, exact commit, tests run and results, user-visible demonstration, compatibility/migration evidence, updated documentation and any residual limitation. If a task is deferred or dropped, record the reason and the features affected. Preserve history so future contributors can understand why a system exists.


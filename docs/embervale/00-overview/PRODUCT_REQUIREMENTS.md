# Product requirements and scope

**Status:** Requirements baseline for planning; proposed technical details remain subject to recorded decisions.  
**Owner:** Project owner. **Maintenance:** Update when scope or an accepted constraint changes.

## 1. User outcomes

| ID | Required outcome | Acceptance evidence |
|---|---|---|
| REQ-001 | Understand what the game currently does and what remains | Source-linked inventory and a roadmap whose status can be audited |
| REQ-002 | Design the starter area through visual editing | A playable flat River Crossing blockout created through Studio |
| REQ-003 | Let an AI make the same changes reliably | Documented structured commands, validation results and reviewable diffs |
| REQ-004 | Keep existing gameplay and saves working during content migration | Baseline parity tests and historical save fixtures |
| REQ-005 | Explore an organic settlement without a mandatory central core | Distributed services, route alternatives and playtest evidence |
| REQ-006 | Improve artwork gradually | One imported tile and one tree agree across editor, preview and game before broader replacement |
| REQ-007 | Expand tools and geography without discarding the foundation | Versioned content, explicit references and tested migration procedures |
| REQ-008 | Understand and recover from errors | Actionable diagnostics, undo, draft recovery, conflict handling and preserved previous published content |

These requirements express the intended product. They do not assert that the supporting systems are already implemented.

## 2. Functional requirements for the first editor release

**Open and inspect.** Show region identity, dimensions, ground layer, placements, references and validation state. An unknown schema or missing content dependency must produce a clear diagnostic before it reaches unchecked map indexing.

**Make a small change.** Paint ground and move an existing single-tile resource using shared commands. Keep the placement identity while changing its position. Display affected collision and interaction routes.

**Review.** Show before/after content, validation errors and warnings, a deterministic semantic preview and the expected base content hash. A dry run produces no persistent change.

**Recover.** Support undo and redo within the editing session, a separately stored recoverable draft and an explicit save conflict if another writer changed the file. Recovery must not silently replace a published map.

**Publish locally for playtesting.** Validate the exact candidate, produce a versioned manifest and load that version in a fresh test session. Publication here means selecting validated local content for the game; it does not mean internet deployment or editing a running production server.

**Verify equivalence.** The GUI, CLI, preview and game must interpret the same region identity, coordinates, definitions and collision rules. A visual difference in renderer antialiasing is distinct from a gameplay difference in occupancy or approach tiles.

## 3. Quality requirements

| Area | Requirement | Measurement approach |
|---|---|---|
| Correctness | Invalid dimensions, positions and references fail before runtime construction | Negative fixtures, boundary tests and structured diagnostics |
| Determinism | Equivalent authored input canonicalises consistently | Round-trip, repeated hash and command replay checks |
| Compatibility | Existing spawn lifecycle, world semantics and player state survive initial migration | Snapshots before/after initialization stages; save fixtures |
| Reliability | A failed save or conflict leaves the previous valid publication usable | Interruption, filesystem and stale-base tests |
| Accessibility | Errors, selection and state are understandable without relying on colour alone | Keyboard checks, text labels, focus and contrast review |
| Performance | Editor actions remain usable on the real development region | Measured timings on recorded hardware; budgets chosen from results |
| Maintainability | One implementation owns each authoritative rule | Dependency review and shared-validator integration |
| Automation | Tools return predictable results and scope mutations explicitly | CLI contract tests and machine-readable capability reporting |

There is no invented millisecond budget in this baseline. Record actual measurements, representative map sizes and expected developer hardware before adopting a performance threshold. A 100×100 map is the known current fixture, not a guarantee that arbitrarily larger maps will be efficient.

## 4. Scope boundaries

The initial product scope is a local development workflow for a single author or coordinated branches. It includes safe file edits, but does not require real-time collaborative editing. It includes explicit stable content IDs, but does not require replacing every runtime ID. It reserves a ground surface identity, but does not claim implemented multi-floor navigation.

The later roadmap covers broad content editing, presentation improvements, elevation and multiplayer research. Each requires its own technical evidence and acceptance gate. A future MMO ambition should inform interfaces without forcing the first editor release to implement online services.

## 5. User journeys to demonstrate

### A. The first tree move

Open the exported map; select the existing normal tree; move it to a reachable, unoccupied tile; inspect warnings and collision; undo and redo; save; reopen; launch a disposable playtest profile; walk to the new approach tile and gather; close; reload the player; confirm identity and state remain valid.

### B. The same change by an AI

Inspect a named region and capture its canonical hash; submit a typed move command against that exact hash; request a dry run; inspect the resulting diff and diagnostics; commit the candidate through the same transaction implementation; generate a preview and test evidence. If the base changed, stop with a conflict and reread it.

### C. A designer makes a temporarily invalid map

Block a route while rearranging several placements; see the invalid state immediately; save the draft for recovery; fail publication while required access is broken; complete the rearrangement; validate the final candidate; publish only that final valid content. This journey is why draft saving and gameplay-valid publication must be distinct.

### D. A player returns after a map update

Load an older supported save against the new published content; verify its region and position; use an explicit safe fallback if the old position is blocked; preserve inventory, equipment, skills, quest progress and other valid state; report any migration. Apply the same review to saved ground items or recovery piles, respawns and other position-bearing data.

## 6. Definition of ready

A work item is ready when its user value, baseline source, affected systems, acceptance criteria, dependencies and rollback are clear. Unknown parser, renderer or networking choices should have an explicit decision task. A large feature with no demonstrable intermediate result should be split before coding.

## 7. Definition of done

A feature is complete when its specified behaviour exists; relevant checks have run on the actual change; compatibility obligations are met; user-facing workflows are documented; failures are understandable; and the status record links the evidence. Reviewers must be able to reproduce the result without relying on an unrecorded conversation.

An editor feature also needs a real authored example that the game can consume. A game feature needs a player-facing demonstration. Documentation-only work needs accurate status wording, working links, consistent terminology and a clear relationship to older documents.

## 8. Traceability

REQ-001 maps to the current-state inventory, roadmap and documentation maintenance. REQ-002 and REQ-005 map to the flat starter-region milestone. REQ-003 and REQ-008 map to the first safe editor slice. REQ-004 maps to the foundation and save-compatibility gates. REQ-006 maps to the art pilot. REQ-007 maps to content versioning, later release compatibility and optional world expansion.

New requirements should use a stable ID and link the affected milestone. Do not rewrite an accepted requirement merely to make an implementation appear complete.

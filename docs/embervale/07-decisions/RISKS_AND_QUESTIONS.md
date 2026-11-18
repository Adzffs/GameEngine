# Risk register and open questions

**Reviewed:** 18 November 2026. **Status:** Open items unless evidence below says otherwise.\
This register distinguishes observed findings from future design risks. Priority is a planning judgement, not a measured probability or a security severity score.

## 1. Immediate findings

| ID | Finding | Evidence and effect | Next action |
|---|---|---|---|
| RISK-001 | GitHub CI cannot start because of an account billing lock | Public run annotation establishes the blocker; local Linux baseline passes all 81 tests, but Windows/remote CI remain unverified | Owner resolves account lock, then rerun CI on the intended SHA; [evidence](../06-development/BASELINE_VERIFICATION_2026-11-18.md) |
| RISK-002 | Development defaults are boosted | Current configuration injects XP; elapsed starter progression could be misleading | GAME-001: define/test a genuine fresh profile |
| RISK-003 | Ground and occupied tiles are mixed | Resource/station creation overwrites tile markers; moving a resource can lose base ground if exported naïvely | Export now preserves underlying ground; STUDIO-013 still needs the runtime compatibility adapter |
| RISK-004 | Spawn catalogue and lifecycle are authoritative | External coordinates cannot bypass canonical equality checks; delayed actors and creation order affect behaviour | STUDIO-001/014/043: explicit provider and lifecycle mapping |
| RISK-005 | Saved coordinates can become invalid | Player fallback alone does not cover respawns or recovery piles | STUDIO-002/053: trace/test all location-bearing state |
| RISK-006 | Current map writes require validated bounds | External content expands input surface before unchecked tile access | STUDIO-011/050: reject dimensions/positions before construction/writes |
| RISK-007 | Narrative and economy descriptions have drift | Elite target naming and older ledger rows disagree with source | DOC-002/GAME-007: correct facts and decide remaining intent |
| RISK-008 | Local editor files are stale; ignore rules miss actual output paths | Tracked .vscode setup targets old local paths; dash-named builds and saves need coverage | DOC-001: focused cleanup with removal manifest |
| RISK-009 | Multi-step economy arbitrage is plausible | Source-derived 3 logs→12 planks→2 shields implies 3 coins spent and 24 returned; not runtime-replayed | GAME-008: reproduce complete chain and fix/test confirmed exploit |
| RISK-010 | Branch protection is not established | Inspected branch reports unprotected and rulesets returned none | Decide supported review/check rules; do not claim existing protections |

These statements are grounded in the [evidence ledger](../01-current-state/EVIDENCE_LEDGER.md) and pinned source references there. The documentation update addresses RISK-008 and factual parts of RISK-007 on its branch. Runtime findings remain implementation work.

## 2. Design and delivery risks

| ID | Risk | Trigger to watch | Mitigation / owner role |
|---|---|---|---|
| RISK-011 | AI or concurrent tools overwrite newer edits | Mismatched base hash, external file change during save | Shared guarded transactions; Studio maintainer |
| RISK-012 | Canonical sorting changes runtime initialization | Imported IDs or actor timing differ despite visually identical map | Separate serialisation order from lifecycle mapping; engine maintainer |
| RISK-013 | Invalid drafts become runtime content | Draft saved without gameplay validation is selected for play | Separate draft/publication paths and exact validation proof; tool maintainer |
| RISK-014 | Preview diverges from game rules | Image looks passable but pathfinding rejects route | Shared semantic data and integration checks; engine/tool maintainers |
| RISK-015 | Editor scope consumes game development | Many panels, no end-to-end playable content change | Freeze Alpha 0.1 and ship real demonstrations; project owner |
| RISK-016 | Settlement recreates a central hub | Every errand requires the same crossing/service cluster | Observe route usage and distribute necessary functions; game designer |
| RISK-017 | Art production outpaces pipeline | Large atlas exists before pivots/frames/provenance are validated | Small real-scale art pilot; presentation maintainer |
| RISK-018 | Elevation corrupts flat assumptions | Coordinate, selection or save code assumes x/y alone | Isolated fixture and explicit surface contract; engine maintainer |
| RISK-019 | Performance work is speculative | Architecture grows without reproducible slow workload | Profile current/map-scale scenarios first; engine maintainer |
| RISK-020 | Documentation becomes a second outdated system | Feature changes without status/source updates | Update docs in same PR; regular evidence refresh; change author |
| RISK-021 | Release package lacks usable setup/recovery | Works only on author machine or changes save location | Clean-environment install/update/rollback checks; release maintainer |
| RISK-022 | Online scope exceeds proven authority | Client-local mutations or unbounded commands reach server | R9 command audit and failure prototype; networking maintainer |

Owner roles describe responsibilities; they do not assert that additional team members exist. The project owner can assign or combine them.

## 3. Open questions by decision point

### Before the content model

- Which bounded input parser should consume the future authoring format? The initial export uses canonical JSON with ASCII terrain rows.
- How will future edit commands allocate lifecycle/order keys for new placements while preserving the explicit baseline stages and durable IDs?
- Which references need durable placement identity versus a reusable definition ID?
- Which additional saved-content observations are needed to extend the existing terrain, lifecycle and interaction baseline to runtime import?

### Before the first editor release

- What is the minimal GUI packaging that preserves a clean headless dependency boundary?
- Which preview backend can be reproduced in CI while sharing presentation semantics?
- What lock/recheck and replace guarantees are supported on each target filesystem?
- What exact fields are included in content hashes, manifests and validation provenance?
- How are autosaved drafts discovered, retained, recovered and discarded visibly?

### Before starter balance signoff

- What is the genuine new-player starting state?
- What does runtime reproduction show for GAME-008, and which correction preserves useful shop/crafting roles?
- Does the proposed region require more resource copies, or would shorter routes/respawn pacing solve the observed issue?
- Which NPC/objective name is canonical for the elite quest?
- Does first-upgrade pacing feel good when travel, inventory pressure, mistakes and recovery are included?

### Before expansion

- Does a particular gameplay objective justify elevation or can flat geography deliver it?
- Which second region proves useful transitions and references with the smallest implementation?
- Which command paths still mutate locally and need an authority boundary before networking?
- Who owns release recovery, asset rights and ongoing maintenance as external contributors arrive?

## 4. How to close an item

Attach implementation or decision evidence, verification outcome and any residual limitation. “Documented” closes a documentation task; it does not close an unresolved runtime risk. If a risk is accepted, record the scope and reason. If deferred, identify the milestone it blocks. Keep closed items with their resolution rather than deleting the history.

# Risk register and open questions

**Reviewed:** 18 November 2026. **Status:** Open items unless evidence below says otherwise.\
This register distinguishes observed findings from future design risks. Priority is a planning judgement, not a measured probability or a security severity score.

## 1. Immediate findings

| ID | Finding | Evidence and effect | Next action |
|---|---|---|---|
| RISK-002 | Development defaults are boosted | Current configuration injects XP; elapsed starter progression could be misleading | GAME-001: define/test a genuine fresh profile |
| RISK-003 | Ground and occupied tiles are mixed | Resource/station creation overwrites tile markers; moving a resource can lose base ground if exported naïvely | Export and runtime import preserve base ground; retain this contract in shared move/delete operations |
| RISK-004 | Spawn catalogue and lifecycle are authoritative | External coordinates cannot bypass canonical equality checks; delayed actors and creation order affect behaviour | Selected per-world spawns and staged creation now preserve this boundary; broaden providers only with definition-specific tests |
| RISK-005 | Saved coordinates can become invalid | The imported starter path now handles player, death-spawn and recovery accessibility; broader region/version migrations remain open | STUDIO-053: retain tested position handling when widening content scope |
| RISK-006 | Current map writes require validated bounds | External content expands input surface before unchecked tile access | Implemented bounded parser and runtime validator reject unsafe starter input; retain limits for future extensions |
| RISK-007 | Baseline narrative/economy drift; bounded current correction delivered | Ledger and canonical Cinder Warden objectives/reward wording now agree with source | [Current continuity](../06-development/QUEST_CONTINUITY_2026-11-19.md); full quest-chain/visible acceptance remains deferred |
| RISK-008 | Local editor files are stale; ignore rules miss actual output paths | Tracked .vscode setup targets old local paths; dash-named builds and saves need coverage | DOC-001: focused cleanup with removal manifest |
| RISK-009 | Timber arbitrage reproduced and corrected | Old cycle gained 21 coins; current whole cycle loses 6 | [Production before/after evidence](../06-development/STARTER_ECONOMY_2026-11-19.md); deferred regressions precede balance signoff |

These statements are grounded in the [evidence ledger](../01-current-state/EVIDENCE_LEDGER.md) and pinned source references there. Earlier documentation cleanup addresses RISK-008 and factual parts of RISK-007. The runtime-import slice adds bounded handling for RISK-003–006; wider authoring and migration remain scoped work.

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

### Before extending the current starter contract

- Which additional schema or encoding features justify widening the current bounded ASCII JSON contract?
- How will future edit commands allocate lifecycle/order keys for new placements while preserving the explicit baseline stages and durable IDs?
- Which references need durable placement identity versus a reusable definition ID?
- Which further save/content compatibility fixtures are needed before supporting new region identities or placement types?

### Before the first editor release

- What is the minimal GUI packaging that preserves a clean headless dependency boundary?
- Which preview backend can be reproduced locally while sharing presentation semantics?
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

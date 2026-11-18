# Release gates and acceptance demonstrations

**Status:** Proposed acceptance contract. No Studio release is marked complete by this document.\
**Purpose:** Make completion observable and prevent unfinished foundations from being hidden behind a large feature list.

## 1. Evidence required for every gate

Record the exact source commit, content hash/version, build configuration, tool versions, platform, commands, results and known limitations. Keep automated results separate from manual observations. A screenshot establishes appearance; it does not establish collision, reachability or save compatibility. An exported file establishes output; it does not establish successful runtime loading.

A fresh local Linux baseline passes, and the export slice has its own regression evidence. Windows remains unverified; the local Linux run does not establish a cross-platform result. See the [verification record](../06-development/BASELINE_VERIFICATION_2026-11-18.md).

## 2. Baseline gate

| Requirement | Evidence |
|---|---|
| Current build can be reproduced | Headless build/test log and supported SDL3 client build/test log with commit and environment |
| World ownership is known | Inventory of terrain, object, station, NPC, monster, label and quest-location definitions |
| Save obligations are known | Fields, versions, player loading, respawns, recovery piles and fallback routes traced |
| Test profile is honest | Development boost distinguished from fresh-player conditions |
| Documentation is current | Root index, source-backed status, existing-doc corrections and retained historical context |

If a platform cannot be exercised, the record must identify that gap and constrain the next release scope. A gate should never be passed by changing the wording from “tested” to an ambiguous “supported.”

## 3. Content-foundation gate

The read-only export represents the actual 100×100 development region. It preserves base ground, the seeded pond, ordered terrain overlays, placements, source definition identities and lifecycle rules. Canonical serialisation is deterministic without changing runtime initialization order.

All dimensions and coordinates are checked before allocation or indexing. Duplicate IDs, unknown references, unsupported versions and excessive inputs fail cleanly. Baseline snapshot comparisons cover both pre-player and post-player world states, relevant occupancy and interaction behaviour. Existing enum mappings and save fixtures remain intact.

## 4. Studio Alpha 0.1 gate

This is the first complete editing slice. Every row is required unless an explicit release decision narrows the advertised product.

| Scenario | Expected outcome |
|---|---|
| Open baseline content | Region identity, dimensions, ground and placements agree with export |
| Move an existing tree | Same authored ID, new legal position, restored old ground and correct new occupancy |
| Paint a small ground area | Terrain change is visible, reversible and checked against required routes |
| Undo and redo | Full semantic state returns correctly; no duplicate placement IDs |
| Dry-run an AI/CLI edit | Diff/diagnostics are returned and no persistent file changes occur |
| Apply against stale base | Conflict is reported; newer content and recovery data remain intact |
| Save and reopen | Canonical data and hash represent the intended edit |
| Crash/interruption during draft write | Previous publication remains usable; recovery policy produces a clear result |
| Keep an unfinished draft | Structurally valid work can be recovered even while gameplay validation fails |
| Publish invalid content | Publication fails with actionable diagnostics and preserves prior selection |
| Preview and launch | Exact validated content version is used; selection/collision semantics agree |
| Interact in the game | Player can approach the moved tree and gather normally |
| Load a supported existing save | Valid progress survives; blocked old position follows a defined safe fallback |
| Use GUI and CLI | Equivalent operations produce equivalent canonical content |
| Use the keyboard | Essential actions, focus, selection and errors remain accessible |

Do not call this release complete when only a separate editor mockup exists. The demonstration must use real runtime content and a real gameplay interaction.

## 5. Studio Alpha 0.2 and starter blockout gate

All current placement categories are editable through supported definitions. Named NPCs, monsters, resource nodes and stations preserve identity, lifecycle and required approaches. Deleting a referenced placement is either prevented or performed through an explicit validated dependency change.

The flat River Crossing candidate can be played with a genuine fresh-player profile. The player can reach guidance, safe gathering, crafting, bank/shop services and appropriate danger without depending on a mandatory central hub. Routes avoid blocked corners and inaccessible station sides. Enemy placement and recovery routes are tested through runtime movement, not only an artistic diagram.

Document proposed counts and observed congestion separately. Keep the original development region as a regression fixture. The first art import can be piloted alongside this milestone, but the coherent visual kit is owned by R4.

## 6. Starter vertical-slice gate

An uncoached tester can complete the intended first gameplay loop. The test records profile, route taken, elapsed time, resource delays, supplies, upgrades, quest outcomes and points of confusion. The 10–15 minute upgrade goal is evaluated as a hypothesis; a missed target produces design work, not a fabricated pass.

Before claiming the economy balanced, reproduce and address any confirmed GAME-008 profit chain, validate multi-step transformations and check repeatable coin/item sources and sinks. Keep permanent upgrades, consumable sinks and recovery purchases distinguishable.

Art and audio use a consistent, validated asset set. Sprite size does not alter gameplay footprint. Visual state is clear without colour alone, UI text matches the implemented objectives and missing assets fail or fall back visibly according to policy.

## 7. Per-workspace authoring gate

Every additional content editor must author one real definition consumed by the authoritative runtime. Required checks include reference integrity, the relevant existing behaviours, unsupported-value rejection, version/migration rules and a demonstrated user workflow. A dialogue graph needs working choices and session rules; a quest editor needs a saved/reloaded quest; a recipe editor needs a real station craft. Merely serialising a graph is insufficient.

## 8. Beta candidate gate

- Supported build/install combinations are explicitly listed and exercised.
- The release commit's relevant local checks pass, and any intentionally excluded environment is stated.
- Engine, content schema, asset and player-save compatibility are documented together.
- Package creation and selection are repeatable; release metadata identifies exact inputs.
- Normal operation, corrupted/missing content, update and rollback are tested.
- Performance budgets are grounded in recorded measurements and representative workloads.
- User workflows, contributor instructions, known issues and migration notes are current.
- Any optional MCP integration uses the same command rules and explicit workspace scope.

## 9. Elevation gate

Before a production region gains elevation, an isolated 24×24 fixture proves surface identity, legal transitions, cliffs, diagonal restrictions, navigation, camera selection, interaction/combat separation, respawn and save/reload. Overlapping walkable surfaces have distinct tests. Flat-region behaviour stays unchanged. The isolated preview can remain experimental without blocking the flat game.

## 10. Multiple-region and multiplayer gates

Two-region acceptance requires reliable travel, identity, save/reload and recovery on either side. A multiplayer prototype requires two clients agreeing on one authoritative result, with explicit tests for invalid commands, message duplication/order, disconnects and contention. A local demonstration does not establish internet-hosting readiness. Public operation needs its own delivery and recovery scope.

## 11. Gate record template

For each candidate, record: release/milestone; exact commit and content versions; required scenarios; automated commands/results; manual observations; platform/tool versions; unresolved failures; compatibility/migration checks; rollback verification; and decision. Decisions are **pass**, **fail**, or **deferred with a named scope limitation**. Avoid a generic “mostly done” status.

# Glossary

| Term | Meaning in this project |
|---|---|
| Authoritative runtime | The part of the game that decides valid simulation outcomes. Current local authority is not proof of a deployed online server. |
| Authored content | Definitions made by a designer: terrain, placement homes, item rules or quest text. |
| Runtime state | What is happening in a running world: depleted trees, moving NPCs, health, dropped items and player progress. |
| Definition ID | A stable reference to a reusable kind of content, such as a normal-tree definition. |
| Placement ID | A stable reference to one authored instance, even after its coordinates change. |
| Runtime entity/object ID | An identifier assigned during world construction or play; it may differ across sessions. |
| Region | A named authored map area with local coordinates and content ownership. |
| Surface | A distinct walkable layer, such as ground or a future bridge deck; current gameplay is flat. |
| Terrain | The underlying ground material, separate in the proposed model from objects occupying it. |
| Occupancy | The gameplay effect of an object or rule on a tile, including whether movement is blocked. |
| Footprint | Tiles an object occupies. It is not necessarily the same size as the visible sprite. |
| Approach tile | A legal location from which the player can perform an interaction. Reachability of the object's centre alone is insufficient. |
| Spawn definition | Authored home, type and lifecycle rules for an NPC/monster, distinct from its current live position. |
| Tick | A fixed simulation step. The inspected engine uses 600 ms ticks. |
| Headless | Running without the graphical client; important for tests, validation and automation. |
| Canonical serialisation | Writing equivalent data in one consistent representation so diffs and hashes are reliable. |
| Content hash | A digest identifying the exact relevant content; used to detect stale edits and tie validation to a version. |
| Schema version | The version of a file's structure and meaning; separate from game release or player-save versions. |
| Transaction | An edit or batch that commits as one validated operation, or leaves the previous state intact. |
| Dry run | Evaluate the proposed change and report its consequences without persisting it. |
| Draft | Recoverable work that may still fail gameplay validation. |
| Published content | A validated immutable package selected for the game; the term does not automatically mean public internet deployment. |
| Parity | Evidence that the new content path reproduces the relevant behaviour of the old path. |
| Golden baseline | A reviewed expected observation used for regression comparison; choose meaningful semantics rather than incidental values. |
| ADR | Architecture decision record: the problem, alternatives, choice and consequences. |
| CLI | Command-line interface, used by scripts, developers and AI. The Studio CLI is proposed. |
| MCP | An optional future integration interface around the same commands; it is not required for the first editor. |
| Blockout | A simple playable layout used to test space and flow before detailed artwork. |
| Vertical slice | A small complete player experience spanning the systems needed to deliver it. |
| Source/sink | How an item or currency enters and leaves the game economy. |
| Arbitrage | A repeatable conversion/purchase/sale route that produces a profit under the rules. The documented candidate needs runtime reproduction. |
| Milestone | An outcome with acceptance gates, not just a date or a collection of files. |
| Planning ID | A stable task reference such as STUDIO-012; it is not an existing GitHub issue number. |

The fixed-tick fact is supported by the pinned [Clock source](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Time/Clock.h). Other entries define terminology used in this documentation.


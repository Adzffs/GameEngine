# Evidence ledger and review limits

**Current evidence reconciliation, 20 November:** [Periodic autosave](../06-development/PLAYER_AUTOSAVE_2026-11-20.md) pins current code fa5e0e93efa71ac60519287cfd9a35b328ec6270 and Linux results: 112/112 SDL, 93/93 separate headless and 42/42 Python units, including killed-process reload after a completed checkpoint. The earlier [project review](../06-development/PROJECT_REVIEW_2026-11-20.md) retains Windows 102/87 results for its source. See [current status](CURRENT_STATUS.md) for scope. Chronological descriptions below retain their original source/method limits.


**Original review:** 18 November 2026.\
**Game-code baseline:** `e9d32932e8f9b27e96283313cca320058792a81f`.\
**Original review method:** Read-only GitHub metadata, pinned source reads, recursive tree/history inventory, PR inspection and source-derived counts. That initial documentation review did not build the game.
**Subsequent verification:** A clean local build of documentation merge `c697386382845f17ce02027e6bade24591a9eb36` passed all 81 existing tests. The exporter change passed all 83 CTest entries. The [dated record](../06-development/BASELINE_VERIFICATION_2026-11-18.md) preserves those baseline/export results. Later parser/runtime changes require evidence for their own source revision.

This ledger is the factual boundary for the documentation pack. A roadmap can describe ambitious future work while this file remains conservative about what exists and what was verified.

## 1. Status vocabulary

| Status | Meaning | Evidence required to use it |
|---|---|---|
| SOURCE VERIFIED | A definition, path, algorithm, configuration, or behaviour is present in inspected source. | Exact commit and source path; explain any inference. |
| REMOTE RESULT VERIFIED | A specific repository action has an observed outcome. | Exact commit where applicable, result URL, date, outcome, and limitations. |
| HISTORICAL REPORT | An earlier PR, note, or document claims an outcome. | Link and date; preserve the distinction from current verification. |
| PLANNED | Work is included in the current proposed implementation sequence. | Scope, dependency, owner when assigned, and exit evidence. |
| PROPOSED | A design option or recommendation awaits an implementation/design decision. | Rationale, tradeoffs, and decision point. |
| UNVERIFIED | Evidence is missing or does not establish the claimed outcome. | State what would resolve it. |

“Done” in a work tracker should require the relevant acceptance evidence and merged implementation. A source inventory, generated document, passing isolated unit test, or test result for a different commit is not a substitute.

## 2. Original source-review observations before PR #3

| ID | Observation | Evidence and interpretation |
|---|---|---|
| EV-001 | Default branch is `master`; reviewed head is `e9d3293…`. | [Repository](https://github.com/Adzffs/GameEngine), [branch API](https://api.github.com/repos/Adzffs/GameEngine/branches/master), and [commit](https://github.com/Adzffs/GameEngine/commit/e9d32932e8f9b27e96283313cca320058792a81f), checked 18 November. |
| EV-002 | Head merged PR #2 on 17 November 2026 at 04:39:29 GMT. | [Merge commit](https://github.com/Adzffs/GameEngine/commit/e9d32932e8f9b27e96283313cca320058792a81f) and [PR #2](https://github.com/Adzffs/GameEngine/pull/2). |
| EV-003 | Complete recursive baseline tree returned 363 file entries, including 272 `src/` files and 81 test `.cpp` files. | [Pinned tree](https://github.com/Adzffs/GameEngine/tree/e9d32932e8f9b27e96283313cca320058792a81f); API recursive response was not truncated. Counts exclude directories. |
| EV-004 | Exactly three Markdown documents are on the reviewed default branch, with no root README or licence file in that tree. | Same complete tree. This describes the baseline before this pack is integrated. It is not a claim about every branch. |
| EV-005 | PR #1 was open and unmerged at the baseline review. | [PR #1](https://github.com/Adzffs/GameEngine/pull/1), branch `docs/project-documentation`, inspected head `6e61d4984e55385ab2031d5a99b11189b4f87c23`. It proposes eighteen documentation files based on a September snapshot. |
| EV-006 | No release objects were returned by the releases endpoint. | [Releases](https://github.com/Adzffs/GameEngine/releases), checked 18 November. This does not rule out untagged/manual builds. |
| EV-007 | The branch summary reported `protected: false`; the repository rulesets endpoint returned an empty array. | [Branch API](https://api.github.com/repos/Adzffs/GameEngine/branches/master) and [rulesets API](https://api.github.com/repos/Adzffs/GameEngine/rulesets), checked 18 November. Record as observed settings, not a full organisation/security audit. |

## 3. Updated local build evidence

[PR #3](https://github.com/Adzffs/GameEngine/pull/3) merged the handbook and focused cleanup at `c697386382845f17ce02027e6bade24591a9eb36` without changing engine/test source. A separate clean worktree at that commit built successfully on Linux; all **81 original tests passed**, including 4 client and 7 graphics tests using SDL’s dummy driver. The exporter implementation also passed its **72-test headless configuration** and **83-test client configuration**.

The [verification record](../06-development/BASELINE_VERIFICATION_2026-11-18.md) preserves the environment, commands, limitations and machine-readable test/source record. No Windows execution is claimed.

### Current runtime-input implementation

The current slice implements a strict schema-1 parser and file reader, 100×100 runtime geography validation, selected spawn authority and an opt-in World construction path. `EmbervaleWorldValidate` and `GameEngine --world` share that interpretation. Imported playtests default to no player save; explicit save selection includes world/save-sidecar alias checks. [Current source and usage](../04-studio/STARTER_WORLD_IMPORT.md) and [ADR-017](../07-decisions/ADR_017_BOUNDED_STARTER_WORLD_IMPORT.md) identify the implementation boundary.

The runtime-import implementation passed **77/77 headless tests** and **89/89 tests** in the Linux SDL configuration (77 headless, 5 client, 7 graphics). A content-tools-disabled build passed its two selected runtime/launch tests. A real game process loaded an edited normal-tree coordinate `(40,20)`, ran for two seconds with temporary progress and stopped gracefully with exit `0`; it created no files. This used dummy video/software rendering, not visible manual gameplay. The [runtime verification record](../06-development/RUNTIME_IMPORT_VERIFICATION_2026-11-18.md) contains exact source, environment and commands. Windows execution is unverified.

## 4. Principal source evidence

All links in this table are pinned to the reviewed game-code baseline.

| ID | Source | Supports |
|---|---|---|
| EV-010 | [CMakeLists.txt](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/CMakeLists.txt) | C++20, target split, SDL conditions, 70 headless/7 graphics/4 client test registrations. |
| EV-011 | [World.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/World.cpp) | Construction, ordered update, command integration, occupancy, spawn validation, recovery and persistence integration. |
| EV-012 | [DevelopmentWorldContent.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Development/DevelopmentWorldContent.cpp) and [header](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Development/DevelopmentWorldContent.h) | 100×100 dimensions, `(0,0)` spawn, fifteen terrain rectangles, seven resources, three stations. |
| EV-013 | [Map.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Map.cpp) and [Tile.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Tile/Tile.cpp) | Constructor pond, unchecked low-level tile access, bounds-aware traversability, terrain/occupancy coupling. |
| EV-014 | [NpcSpawnDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/NPC/NpcSpawnDatabase.cpp) and [NpcDefinitionDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/NPC/NpcDefinitionDatabase.cpp) | Ten definitions and spawn anchors; six friendly names and four enemy roles. |
| EV-015 | [ServerCommand.h](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Command/ServerCommand.h) and [Engine.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Core/Engine.cpp) | Existing command boundary and client integration; direct running-toggle seam. |
| EV-016 | [PlayerSaveData.h](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Persistence/PlayerSaveData.h) and [codec](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Persistence/PlayerSaveTextCodec.cpp) | Current version 13, saved fields, version-dependent decoding and migration code. |
| EV-017 | [File store](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Persistence/PlayerSaveFileStore.cpp), [entry point](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/main.cpp), and [existing persistence document](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/docs/DevelopmentPlayerPersistence.md) | Temporary/backup handling, relative save-path policy, startup and controlled-shutdown guarantees and limitations. |
| EV-018 | [Quest catalogue](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Quest/QuestDefinitionDatabase.cpp), [dialogue catalogue](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Dialogue/DialogueDefinitionDatabase.cpp), and [tutorial](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Tutorial/TutorialState.h) | Six quests, prerequisites, alternate completions, dialogue and tutorial definitions. |
| EV-019 | [Item catalogue](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Item/ItemDatabase.cpp), [recipes](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Recipe/RecipeDatabase.cpp), and [shop](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Shop/ShopDefinitionDatabase.cpp) | 23 registered items, 14 recipes, one shop; current economic connections. |
| EV-020 | [Player.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Player/Player.cpp) and [DevelopmentConfig.h](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Core/DevelopmentConfig.h) | Stamina implementation and enabled development XP boosts. |
| EV-021 | [Graphics.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Graphics/Graphics.cpp) and [header](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Graphics/Graphics.h) | Procedural rendering, 32-pixel tile size, camera/minimap and UI features, presentation coupling. |
| EV-022 | [ContentValidator.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Content/ContentValidator.cpp) | Existing catalogue and cross-reference validation to reuse in Studio. |
| EV-023 | [Shop catalogue](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Shop/ShopDefinitionDatabase.cpp), [recipes](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Recipe/RecipeDatabase.cpp), and [starter recipe book](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Recipe/RecipeBook.cpp) | Source-derived profitable cycle: 3 logs cost 3 coins, yield 12 planks, produce 2 shields sold for 24 coins; net +21 before time costs. This inference has not been replayed through runtime transactions. |

## 5. How quantities were derived

Repository file counts were calculated from entries whose Git-tree type is `blob`. Test counts were calculated from CMake calls: seventy `add_gameengine_headless_test`, seven `add_gameengine_graphics_test`, and four explicitly registered client targets. This is not an assertion count. Catalogue counts came from the explicit registered lists, excluding `NONE` sentinels, rather than from the number of switch cases or enum tokens in an entire file.

The history appendix selects gameplay and project commits from the reviewed `master` history. A commit title is evidence that the change was recorded; it does not establish that the original implementation remains unchanged or currently passes tests. The current source is the stronger evidence for current behaviour.

## 6. Review limits and unresolved questions

- The recursive tree was fully inventoried; source inspection was selective. This is not a line-by-line audit of every source or test file.
- Local builds and automated tests are now recorded above. No visible manual playthrough, performance profile, coverage measurement, accessibility assessment or production security audit was performed.
- Existing save codec, file-store, world and engine persistence tests passed in the local suites. This does not prove every legacy edge case or a future edited-map saved-location migration matrix.
- Branch/ruleset observations do not establish all account, organisation, or external enforcement settings.
- The original baseline had no Studio files. Later work adds export, bounded validation and starter runtime import. A narrow resource/station move-preview command is now on a development branch; full editing, guarded publication, undo and graphical preview remain future work.
- Existing economy and character documents contain design intentions as well as factual claims. Code and explicit current decisions take precedence where the documents are stale.
- The old PR #1 quest-verification gaps are historical review notes. Recheck them against current code and tests before marking them unresolved defects or closing them.

## 7. Refresh protocol

For each future baseline, record the new game-code SHA, date, changed systems, exact build/test commands and environments, result records, and a concise limitation statement. Keep old evidence immutable or clearly dated. Update current-state pages and roadmap statuses together so that implementation and documentation do not drift apart.

When a check is blocked, record the operation, observed failure, and what remains unknown. When a hypothesis is tested, link the result and remove only the uncertainty that result resolves. This keeps the roadmap useful as both a plan and a trustworthy record.

## 8. Bounded guarded draft experiment (18 November 2026)

The separate development branch `studio/guarded-draft-revisions` adds a raw-byte SHA-256 inspector and guarded *new sibling file* publication to the resource/station move preview. The source is re-read under a cooperative lock, the expected hash must still match, and a new destination is installed using a same-filesystem hard link from private staging. This is **not** in-place replacement, noncooperating-writer isolation, power-loss durability, full editor support, or a guarantee across every filesystem. The parent move PR #9 is itself still a draft.

The source file and test harness compiled with strict GCC C++20 settings on Linux in a local isolated workspace; NIST SHA-256 test vectors, create-only publication, existing-target rejection, stale source detection, lock rejection and input symlink rejection passed. The expanded CLI compiled and ran against **mocked gameplay validation and move APIs**, and its CMake command script passed against that mock. The local container could not resolve GitHub to check out the complete repository. Neither the actual game tests nor the full-game executable were run at this branch tip. See [bounded verification record](../06-development/GUARDED_DRAFT_VERIFICATION_2026-11-18.md). The experimental source must remain under review until those checks run.

## 9. Bounded in-memory placement history experiment (18 November 2026)

On the separate stacked `studio/placement-undo-redo` branch, `StarterWorldHistory` supports grouped reversible resource/workstation moves by durable ID. Undo and redo replay validated inverse/forward commands, reject unexpected canonical semantic revisions, preserve base terrain and avoid overwriting unrelated content. The history is process-local and writes neither snapshots nor player saves. The registered new regression suite was compiled and executed **only with reconstructed mock snapshot/runtime definitions**: 30 checks passed; full repository builds, real geometry/runtime tests and Windows remain unverified. Do not consider STUDIO-023 complete before integration and editor-session verification. [Evidence](../06-development/STUDIO_HISTORY_VERIFICATION_2026-11-18.md).

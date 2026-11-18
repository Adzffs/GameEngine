# Evidence ledger and review limits

**Reviewed:** 18 November 2026.  
**Game-code baseline:** `e9d32932e8f9b27e96283313cca320058792a81f`.  
**Review method:** Read-only GitHub metadata and selected pinned source reads, complete recursive file-tree inventory, complete default-branch commit listing returned across three pages, PR inspection, workflow and job metadata, and source-derived counts. No local build or live game run was performed for this documentation review.

This ledger is the factual boundary for the documentation pack. A roadmap can describe ambitious future work while this file remains conservative about what exists and what was verified.

## 1. Status vocabulary

| Status | Meaning | Evidence required to use it |
|---|---|---|
| SOURCE VERIFIED | A definition, path, algorithm, configuration, or behaviour is present in inspected source. | Exact commit and source path; explain any inference. |
| REMOTE RESULT VERIFIED | A specific repository action or CI run/job has an observed outcome. | Exact commit where applicable, result URL, date, outcome, and limitations; CI is a subtype. |
| HISTORICAL REPORT | An earlier PR, note, or document claims an outcome. | Link and date; preserve the distinction from current verification. |
| PLANNED | Work is included in the current proposed implementation sequence. | Scope, dependency, owner when assigned, and exit evidence. |
| PROPOSED | A design option or recommendation awaits an implementation/design decision. | Rationale, tradeoffs, and decision point. |
| UNVERIFIED | Evidence is missing or does not establish the claimed outcome. | State what would resolve it. |

“Done” in a work tracker should require the relevant acceptance evidence and merged implementation. A source inventory, generated document, passing isolated unit test, or a green job for a different commit is not a substitute.

## 2. Fresh repository observations

| ID | Observation | Evidence and interpretation |
|---|---|---|
| EV-001 | Default branch is `master`; reviewed head is `e9d3293…`. | [Repository](https://github.com/Adzffs/GameEngine), [branch API](https://api.github.com/repos/Adzffs/GameEngine/branches/master), and [commit](https://github.com/Adzffs/GameEngine/commit/e9d32932e8f9b27e96283313cca320058792a81f), checked 18 November. |
| EV-002 | Head merged PR #2 on 17 November 2026 at 04:39:29 GMT. | [Merge commit](https://github.com/Adzffs/GameEngine/commit/e9d32932e8f9b27e96283313cca320058792a81f) and [PR #2](https://github.com/Adzffs/GameEngine/pull/2). |
| EV-003 | Complete recursive baseline tree returned 363 file entries, including 272 `src/` files and 81 test `.cpp` files. | [Pinned tree](https://github.com/Adzffs/GameEngine/tree/e9d32932e8f9b27e96283313cca320058792a81f); API recursive response was not truncated. Counts exclude directories. |
| EV-004 | Exactly three Markdown documents are on the reviewed default branch, with no root README or licence file in that tree. | Same complete tree. This describes the baseline before this pack is integrated. It is not a claim about every branch. |
| EV-005 | PR #1 was open and unmerged at the baseline review. | [PR #1](https://github.com/Adzffs/GameEngine/pull/1), branch `docs/project-documentation`, inspected head `6e61d4984e55385ab2031d5a99b11189b4f87c23`. It proposes eighteen documentation files based on a September snapshot. |
| EV-006 | No release objects were returned by the releases endpoint. | [Releases](https://github.com/Adzffs/GameEngine/releases), checked 18 November. This does not rule out untagged/manual builds. |
| EV-007 | The branch summary reported `protected: false`; the repository rulesets endpoint returned an empty array. | [Branch API](https://api.github.com/repos/Adzffs/GameEngine/branches/master) and [rulesets API](https://api.github.com/repos/Adzffs/GameEngine/rulesets), checked 18 November. Record as observed settings, not a full organisation/security audit. |

## 3. Current build evidence: unresolved failure

The [master push run 44018694678](https://github.com/Adzffs/GameEngine/actions/runs/44018694678) is associated with the exact reviewed SHA and has conclusion **failure**. Both jobs—Linux headless and Windows UCRT64 client—also have conclusion failure. Their returned job records contain no steps, no named runner, and approximately two seconds between start and completion.

The job-log download requests returned 404. The commit check-run records confirmed failure and annotation counts, but their summary/text fields were empty and the available fetch interface did not expose the annotation endpoint. **The failure cause is therefore unverified.** These observations do not establish whether the issue is account/runner startup, configuration, compilation, or a failing test. Do not invent a cause from the short duration.

[PR #2](https://github.com/Adzffs/GameEngine/pull/2) says “All 81 tests pass.” Preserve that as a **HISTORICAL REPORT**. CMake independently supports the count of 81 registered test executables, but no passing logs for the exact reviewed baseline were obtained here. The first verification milestone must produce a current, diagnosable result.

The [workflow source](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/.github/workflows/ci.yml) defines Linux headless and Windows UCRT64 jobs. Its Windows test groups temporarily continue after failure so all groups can report, then an always-run enforcement step requires every group to succeed. The workflow structure is not itself evidence of an executed test outcome.

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

The history appendix contains all 234 unique commits returned by the three-page `master` history request. A commit title is evidence that the change was recorded; it does not establish that the original implementation remains unchanged or currently passes tests. The current source is the stronger evidence for current behaviour.

## 6. Review limits and unresolved questions

- The recursive tree was fully inventoried; source inspection was selective. This is not a line-by-line audit of every source or test file.
- No current local build, test run, manual playthrough, performance profile, coverage measurement, accessibility assessment, or production security audit was performed.
- The full historical save-fixture matrix was not executed. Source recognition of a version is not proof that every legacy edge case passes.
- Branch/ruleset observations do not establish all account, organisation, or external enforcement settings.
- Absence of Studio files in this baseline does not rule out private work, local work, or other branches.
- Existing economy and character documents contain design intentions as well as factual claims. Code and explicit current decisions take precedence where the documents are stale.
- The old PR #1 quest-verification gaps are historical review notes. Recheck them against current code and tests before marking them unresolved defects or closing them.

## 7. Refresh protocol

For each future baseline, record the new game-code SHA, date, changed systems, exact build/test commands and environments, remote run links, and a concise limitation statement. Keep old evidence immutable or clearly dated. Update current-state pages and roadmap statuses together so that implementation and documentation do not drift apart.

When a check is blocked, record the operation, observed failure, and what remains unknown. When a hypothesis is tested, link the result and remove only the uncertainty that result resolves. This keeps the roadmap useful as both a plan and a trustworthy record.

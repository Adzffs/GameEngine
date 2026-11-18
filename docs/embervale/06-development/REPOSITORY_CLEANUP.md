# Repository cleanup and documentation integration

**Purpose:** Make every cleanup change reviewable.  
**Baseline:** e9d32932e8f9b27e96283313cca320058792a81f, reviewed 18 November 2026.  
**Scope:** Documentation, local editor configuration hygiene and ignore rules. Runtime code changes are separate work packages.

## 1. Evidence from the existing tree

The reviewed default branch contains 363 tracked files, including 272 source files, 81 test C++ files and three Markdown documents. The complete tree inspection found no tracked executable/build-output directory that needed deleting. The three existing documents contain useful material and are retained. See the [repository guide](../01-current-state/REPOSITORY_GUIDE.md).

Three VS Code configuration files are tracked even though the existing ignore file already excludes .vscode/. Their contents describe a specific older local setup rather than a portable current CMake workflow. The new [Getting started](GETTING_STARTED.md) guide documents the supported target names and reproducible commands.

## 2. Exact removals in this documentation change

| Removed tracked path | Reason | Replacement / recovery |
|---|---|---|
| .vscode/launch.json | Hardcodes c:/GameEngine and build/Debug/outDebug, which does not match the present GameEngine CMake target | Portable CMake onboarding; users can keep their own ignored editor configuration; old bytes remain in Git history |
| .vscode/settings.json | C/C++ Runner-specific local settings, including disabled error squiggles and an installation-specific MSVC path | Editor-neutral build instructions and optional local VS Code setup |
| .vscode/c_cpp_properties.json | Hardcoded local compiler location and broad include discovery | Configure IntelliSense from the chosen CMake toolchain/compile commands in local settings |

Removing these tracked files does not change the CMake targets or source files. Developers with personal customisations should retain those locally before switching branches; .vscode/ remains ignored.

## 3. Ignore-rule corrections

Retain existing underscore-named build entries for compatibility. Add the dash-named build-client/ and build-headless/ directories used by the documented local build commands, and ignore the application's local saves/ directory. Keep authored fixtures under explicit test fixture paths so ignoring local player saves does not hide future committed compatibility fixtures.

Do not use a broad rule that hides all JSON, all images, all data files or the future content directory. Authored content and asset manifests are project source and must remain reviewable.

## 4. Existing documentation retained and reconciled

| Existing document | Treatment |
|---|---|
| docs/DevelopmentPlayerPersistence.md | Retained; linked to the new architecture/save guides; preserve controlled-shutdown, no-tick-autosave and crash limitation details |
| docs/StarterCharacters.md | Retained; clarify that character wants/voices/examines are a design contract, not proof that every line is implemented |
| docs/EconomyLedger.md | Correct stale source/sink/shop/recipe claims; retain useful balance targets and the 23-item ledger; expose the source-derived multistep economy risk |

The new root README and docs/embervale/README.md supply navigation. The detailed roadmap, backlog, decisions and release gates are canonical; additional documents explain specific systems rather than creating competing schedules.

## 5. Relationship to older PR #1

[PR #1](https://github.com/Adzffs/GameEngine/pull/1) proposed a documentation foundation against an older September baseline. The fresh review found it open. Useful vision, templates and authority/persistence principles are incorporated into the new pack. Its old status snapshot and M0–M7 sequence are superseded; features such as crafting and recipe discovery have since appeared in source.

[PR #3](https://github.com/Adzffs/GameEngine/pull/3) is the replacement documentation change. PR #1 was closed as superseded on 18 November 2026, after the replacement was published. Its branch and commit history remain available. Its historical findings are not automatically treated as current bugs.

## 6. Publication procedure

Create a dedicated documentation branch from the freshly checked default-branch head. Add this pack, correct the existing ledger/header links, apply the exact three removals and update ignore rules. Review the complete changed-file list. Validate internal links, source path references, backlog identifiers/dependencies and example syntax. Open a focused PR with the baseline limitations and cleanup manifest in its description.

The documentation PR is not evidence that GAME-008, Studio or the starter redesign have been implemented. Record local verification for the actual change separately from historical test results. Merge and release status must reflect what GitHub actually reports.

## 7. Future cleanup policy

Prefer evidence over age or aesthetics. A source file with an unclear purpose needs reference analysis before removal. A useful older document should be corrected or consolidated. A generated artifact can be removed when its source and reproduction path are known. Record path, reason, replacement and validation for each removal. Avoid broad renames and engine rewrites inside a documentation cleanup.

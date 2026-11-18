# Release, packaging and rollback process

**Status:** Proposed project release process. This documentation pack does not declare any engine, game or Studio release complete.  
**Updated:** 18 November 2026

## 1. What is being released

Keep the deliverable explicit. A documentation publication, an engine build, an authored content package and a public game release have different evidence and effects. Creating a validated Studio content package makes it available for explicit loading; it does not automatically deploy a server, create a GitHub release or replace a player's save.

The [Master roadmap](../02-roadmap/MASTER_ROADMAP.md) names planned outcomes such as Studio Alpha 0.1, the flat starter blockout and a beta candidate. Those labels describe scope to achieve. The detailed [Release gates](../02-roadmap/RELEASE_GATES.md) define completion. This process explains how to package and record a candidate once the relevant gates pass.

## 2. Release record

Every candidate should have one record containing:

| Field | Why it matters |
|---|---|
| Release identity and purpose | Distinguishes a development candidate from a supported release |
| Exact source commit/tag | Identifies the code actually built |
| Content package manifest/hash | Identifies the geography and definitions actually tested |
| Supported platform/toolchain | Limits claims to verified environments |
| Engine/content/schema/save compatibility | Explains what can load together |
| Build and test evidence | Makes the result reproducible |
| Known issues and limitations | Prevents an incomplete feature being mistaken for supported behaviour |
| Asset/dependency provenance | Establishes what is being redistributed |
| Update and rollback instructions | Explains recovery without accidental state loss |

A version string by itself does not establish compatibility. The current player data structure declares save version 13; authored region schema and content package versioning are planned separate domains. [Source: PlayerSaveData.h](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Persistence/PlayerSaveData.h).

## 3. Candidate preparation

Choose a focused scope from the roadmap and freeze the candidate's source/content identities for verification. Resolve required blockers or document which planned feature is excluded from that candidate. Keep unrelated work on separate branches so a release does not acquire unreviewed changes during packaging.

Build from a clean, reproducible checkout/environment using the documented configuration. Record dependency versions rather than assuming a package manager's latest packages are the same as a previous run. For authored content, normalise and validate the selected package and record the validator version/profile.

Run the relevant headless and SDL3 client configurations locally for the intended release platforms. Record the exact commit, commands and outcomes, and explicitly identify platforms not exercised. See [Getting started](GETTING_STARTED.md) and [Testing](TESTING.md).

## 4. Verification and acceptance

Run the required automated groups and the milestone's end-to-end scenario on the exact candidate. Use [Testing](TESTING.md) to report executed evidence accurately. A prior commit's passing run can inform review but cannot be relabelled as a run of a changed candidate.

For a playable build, verify installation/unpacking, first launch, required runtime dependencies, normal exit, save/reload and the essential gameplay loop. For Studio, also verify draft recovery, conflict handling, CLI/GUI agreement, preview consistency and explicit content selection. For a content-only change, evaluate affected routes, interactions and save-position compatibility.

Use a genuine fresh-player profile for onboarding/pacing acceptance. The current development configuration enables XP boosts, so a new default development save is not sufficient evidence of fresh-player progression. [Source: DevelopmentConfig.h](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Core/DevelopmentConfig.h).

If an environment is blocked, report the specific limitation and narrow the candidate's supported scope. Do not replace an unavailable test with a statement that it probably passes. Release decisions should use the configured project review/check process without inventing additional permission gates for routine reversible preparation.

## 5. Packaging

A distributable should contain only the required executable/runtime dependencies, selected content/assets, licences/notices, version/manifest information and relevant user instructions. Exclude personal saves, credentials, machine-specific paths, temporary build caches and private logs.

On Windows, do not assume that copying `GameEngine.exe` alone supplies every SDL/compiler runtime dependency. Resolve dependencies from the chosen toolchain, verify redistribution terms and test the package in a clean environment. Portable paths must not refer to the developer's `c:/GameEngine` checkout or a personal installation layout.

For authored content, package immutable files with a manifest recording schema and hashes. Avoid activation until all files are present and validated. A partial multi-file copy must not become an apparently complete published package.

The repository's code licensing policy and asset licensing decisions must be explicit before broad redistribution. Public availability is not a licence selection. Preserve third-party notices and the asset manifest; do not invent an MIT or other licence in a release checklist.

## 6. Publish a traceable result

After the candidate meets its chosen gates, publish through the authorised project workflow and record the exact tag/commit and artifact identifiers. Release notes should explain concrete player/developer changes, compatibility implications, important fixes, known issues and upgrade instructions. Separate documentation-only changes from implemented gameplay/tool features.

Do not move an already-published tag or silently replace artifacts with different bytes under the same version. A corrected artifact needs a traceable revision and explanation. Keep hashes/manifests with the release record so support reports can identify the actual package.

Update the roadmap and backlog only for the work evidenced by this release. Shipping one gameplay editor does not automatically complete every workspace in the broader toolkit.

## 7. Update and rollback

Before an update, determine whether it changes only content, engine behaviour, persistent save encoding or several of these. Preserve the prior known-good package and document compatible combinations. A content rollback normally selects the earlier immutable package; a player-save rollback restores earned state and is a separate potentially destructive operation.

If a new building blocks a saved player tile, use the documented position fallback/migration policy. Do not restore an old player save merely to undo the building. If a schema migration makes rollback incompatible, state that clearly and provide a tested forward recovery or supported backup procedure.

Use [Save compatibility](../03-engine/SAVE_COMPATIBILITY.md) for player state, recovery piles, respawns and identity migration. An update process must not duplicate rewards, erase unknown items or run a persistent migration twice after an interruption.

## 8. Post-release maintenance

Collect reports with version, source/content identity, platform, steps to reproduce and sanitised evidence. Triage whether the issue comes from content, code, packaging, save compatibility or environment configuration. Prefer a small fix with a targeted regression scenario over unrelated redesign during recovery.

Record incident cause only when supported. Update known issues and the owning documentation as evidence changes. Retire unsupported combinations deliberately, with migration guidance where practical, rather than leaving old files that appear supported but cannot be reproduced.

The next release starts from the last traceable result, its outstanding issues and observed player/developer needs. Calendar promises and “complete” labels should follow evidence, not substitute for it.

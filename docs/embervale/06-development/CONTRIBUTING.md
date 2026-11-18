# Contributing to Embervale and GameEngine

**Status:** Recommended working process for this project; repository enforcement is documented separately.  
**Updated:** 18 November 2026

## 1. Start with a concrete outcome

A contribution should explain the player or developer problem it solves. “Move map placement into authored content” is a direction; “export the existing region without changing its world behaviour” is a reviewable increment. Keep the smallest complete increment that demonstrates useful behaviour and can be reversed independently.

Use the [Master roadmap](../02-roadmap/MASTER_ROADMAP.md) and [Backlog](../02-roadmap/BACKLOG.md) for priorities and stable task identifiers. Planning IDs such as `STUDIO-012` are not GitHub issue numbers. If a corresponding issue is created, link the two rather than silently replacing the planning ID.

Read the relevant current-state and design documents before changing a subsystem. Source facts, historical reports, planned functionality and proposed decisions have different statuses. A future command example is not evidence that an executable already exists.

## 2. Prepare a focused branch

Inspect the working tree and current branch. Preserve unrelated local work. Create a branch appropriate to the task, such as `docs/embervale-roadmap`, `studio/read-only-exporter` or `fix/save-position-fallback`. These are naming examples, not existing branches.

Record the baseline commit and keep the change scope clear. Avoid combining a map redesign, save-format migration, renderer rewrite and editor GUI in one pull request. A narrowly scoped documentation cleanup can proceed independently of runtime changes when its evidence is clear.

Use local verification for the affected code and record the results with the change. A task can proceed once its relevant local evidence is sufficient; distinguish untested platforms from observed failures.

## 3. Work with the existing architecture

The project already separates a headless `GameEngineCore` from SDL-dependent client code. Keep simulation, content validation and typed authoring logic testable without opening a window. Preserve the direction of dependencies described in [Architecture](../03-engine/ARCHITECTURE.md). [Source: CMakeLists.txt](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/CMakeLists.txt).

Before adding another subsystem, inspect whether an existing command, validator, catalogue or presentation model owns the responsibility. Extend that boundary where practical. If an extraction is necessary, keep its behaviour-preserving part separate from the new behaviour so reviewers can understand the evidence.

Treat temporary runtime IDs, durable content IDs and enum values as distinct concepts. Do not change persisted meanings as an incidental refactor. For geography changes, review underlying terrain, effective collision, interaction approaches and spawn lifecycle; moving an image is not sufficient.

## 4. Implementation standards

| Area | Expected practice |
|---|---|
| Scope | Solve the assigned problem and identify any necessary follow-up work |
| Ownership | Keep gameplay authority in the world; keep UI selection/presentation in the client/editor |
| Input | Validate dimensions, references and supported values before unsafe allocation/indexing |
| Errors | Preserve useful context and return explicit failure rather than silently substituting defaults |
| Determinism | Use controlled seeds/ticks in relevant tests and canonical authored output |
| Persistence | Preserve existing state and document compatibility changes |
| Dependencies | Explain why a new library is needed, its supported versions and licence implications |
| Documentation | Update the owning document alongside changed behaviour |

Follow nearby code conventions rather than introducing a formatter or naming overhaul in an unrelated task. Performance changes require a measured problem and a relevant comparison. Broad caching, chunking or parallelism should not be prerequisites for the first content edit.

## 5. Tests and evidence

Choose checks that address the changed behaviour. A parser change needs malformed-input and canonicalisation coverage; a movement change needs relevant route/interaction regressions; a documentation-only change needs link and consistency review. Do not add tests that merely restate an implementation detail without protecting a meaningful contract.

Use [Testing](TESTING.md) for the actual CMake/CTest groups and new Studio coverage. State exactly what ran, on which commit and environment, and what did not run. Existing test registration counts and old pull-request statements are not current run evidence.

For visual changes, include a relevant before/after view or preview and explain what was inspected. For content migration, include semantic world comparisons before and after player registration, plus affected save/interaction scenarios. Keep test evidence proportional to the risk and stop repeating checks once the concrete remaining concern is resolved.

## 6. Pull request contents

A useful pull request description covers:

- The concrete problem and resulting behaviour.
- The scope of the change and the main files/responsibilities affected.
- Relevant task IDs, design decisions and source baseline.
- Tests or checks actually executed, including failures or environment blockers.
- Content/schema/save compatibility implications.
- Preview or playtest evidence for visual/gameplay changes.
- A practical rollback route and material unresolved limitations.

Lead with the change's value; do not force reviewers to reconstruct it from a file list. A screenshot can support a claim about appearance but cannot prove authoritative movement, rewards or persistence. A passing build cannot prove that a new map is enjoyable.

Reviewers should inspect the meaningful diff, dependency direction, compatibility and evidence. Resolve disagreements in the owning issue or decision record so the conclusion survives after the conversation ends. Merge policy should reflect the project's actual contributors and configured checks; this guide does not add a separate approval step to every reversible edit.

## 7. Working with coding agents or parallel contributors

Give each worker an explicit outcome, baseline, allowed files/subsystem, relevant invariants and acceptance evidence. Assign non-overlapping write ownership where practical. Read-only research can run in parallel; competing schema or world-construction edits need coordination.

Agents should inspect current files before editing, preserve unrelated changes and report assumptions. When the base revision changes, reconcile the work against that revision rather than claiming validation from an earlier snapshot. Use the same tests and documentation expectations for human and agent contributions.

AI editing through future Studio commands must use explicit targets, expected hashes and shared validation. Its convenience does not justify bypassing a conflict or publishing an unvalidated candidate. See [CLI and AI contract](../04-studio/CLI_AND_AI_CONTRACT.md).

## 8. Repository hygiene and cleanup

Keep generated build output, personal save data, local logs and machine-specific editor settings out of source commits unless a specific fixture or diagnostic artifact is deliberately required. Review tracked-file deletion individually. A file is not unnecessary solely because it is old; it may document a migration, support a platform or explain a compatibility constraint.

When removing stale local configuration, explain the hardcoded path/tool mismatch and point to portable setup instructions. Preserve the ability for a developer to create their own local settings. Avoid adding broad ignore rules that hide real source/content files.

Asset contributions require source and licence information. The project owner must choose the repository's licensing policy; contributors should not assume that a public repository implies an MIT licence or permission to redistribute every included asset.

## 9. Close the loop

After a change lands, update the task status with the exact commit/PR and evidence. Amend the appropriate guide, catalogue or decision record; do not copy the same authoritative fact into several conflicting places. Record deferred work with a specific reason.

A completed contribution means the scoped outcome is implemented and evidenced. It does not mean its entire roadmap milestone, all platforms or every future extension are complete.

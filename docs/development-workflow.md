# Development and Documentation Workflow

## Before starting a task

1. Confirm the current `master` tip and working-tree state.
2. Find the matching tracker ID and feature/system brief.
3. Confirm scope, non-goals, acceptance evidence and dependencies.
4. Create a named review branch from the pinned base commit for implementation work.
5. If delegating, give each worker one bounded role and assignment; keep implementation and independent review separate.

## While working

- Keep changes within the agreed scope and make clean commits.
- Record implementation decisions in the design or system document, not only chat.
- Add tests for meaningful behavior and failure paths; report exact commands and results.
- Keep server-authoritative outcomes on the server. The client communicates intent and presents results.
- For persistence changes, define historical schema behavior and transactional failure behavior.

## Review and completion

A feature can move to **Done** only when:

- Code is on the intended branch and the commit is identified.
- Acceptance criteria are satisfied.
- Relevant tests/builds pass, or remaining failures are clearly documented and accepted as separate blockers.
- An independent review has resolved required findings.
- User-facing design documentation and system/content index are current.
- Save, authority, determinism, UI and accessibility impacts are handled where relevant.

## Branch and evidence record

For each tracker item, record: base commit, branch, implementation commit(s), review outcome, focused tests, full required CI, and merge commit or current status. Never infer that a review branch is merged because it exists remotely.

# Documentation maintenance

**Purpose:** Keep this pack useful as implementation changes. This is a working procedure, not a claim that an automated documentation system already exists.

## 1. Canonical sources of truth

| Question | Canonical record |
|---|---|
| What does the code currently implement? | Inspected source plus exact execution evidence; summarised in current-state documents |
| What should we build next? | MASTER_ROADMAP.md and its BACKLOG records |
| Why did we choose a design? | Decision register and linked ADRs |
| How should a feature behave? | Relevant engine, Studio or game specification |
| Has it shipped? | Release record/tag with compatibility and verification evidence |
| What happened historically? | Development history and Git history, preserving dates and context |

Conversation ideas can enter the roadmap, but are not implementation evidence. A stale document must not outweigh newer source without an explicit unresolved discrepancy being recorded.

## 2. Update rules for ordinary changes

For a feature PR, update the user workflow or specification that changes, the relevant implemented-status entry, and the roadmap/backlog status where appropriate. Include compatibility and migration notes when saved or authored data changes. Link the exact implementation and test evidence once available.

For a content-only PR, update the affected catalogue and region/quest/asset document, identify changed stable IDs and dependencies, and include the relevant playtest or validation result. For a bug fix, explain the former behaviour and new behaviour in the issue/PR and update any instruction that would otherwise be wrong.

For a documentation-only PR, check factual claims, relative links, code fences, command availability and status wording. Avoid introducing a new competing roadmap for each conversation.

## 3. Status vocabulary

Use SOURCE VERIFIED for inspected implementation, REMOTE RESULT VERIFIED for a specific GitHub outcome, HISTORICAL REPORT for an old unrerun claim, PLANNED for intended work, PROPOSED for candidate design and UNVERIFIED for missing evidence. COMPLETE requires the stated acceptance criteria and evidence. REMOTE CI VERIFIED, where retained in historical evidence, is a specific remote-result subtype.

Always date a source snapshot and pin its commit. Recount catalogue/test registrations when they change rather than assuming old totals are permanent. Registered tests, passing tests and manually exercised workflows are different facts.

## 4. Folder and naming conventions

The numbered folders are reading categories, not implementation phases. Use one clear title per document, a short purpose/status header, related links and source references near factual claims. New large topics receive a focused document and an index entry. Small additions belong in the existing relevant document.

Keep examples labelled as implemented, proposed or illustrative. An illustrative content fixture must be syntactically complete if it is shown as parseable data. Never publish example credentials or private player data. Keep generated documentation exports outside the source tree unless there is a specific versioned-release need.

## 5. Backlog changes

The Markdown backlog is the readable view; BACKLOG.json stores the same fields for automation. When editing a task, update both. A future generator may make the JSON canonical, but no hidden automatic synchronization is assumed today. Run the documentation checker to catch missing IDs/dependencies and then review that both descriptions agree.

Use stable planning IDs. When an actual GitHub issue is created, add its URL without replacing the planning ID. Move or split work deliberately and preserve the old reference. Do not mark DOC-001 merged merely because its documentation branch exists.

### Run the documentation checker

This documentation update includes [tools/check_documentation.py](../../../tools/check_documentation.py), which uses Python's standard library. From the repository root, run:

```bash
python3 tools/check_documentation.py
```

It checks local Markdown/reference links and supported anchors, balanced code fences, JSON examples, Markdown/JSON backlog parity, duplicate or missing task IDs, dependency cycles, declared task totals and retention of the 45 original Studio IDs. It returns a nonzero exit status for detected errors. It is a bounded checker for the Markdown conventions used by this project, not a complete CommonMark parser.

For a documentation-only staging overlay that omits unchanged source files, provide a complete baseline Git-tree JSON response:

```bash
python3 tools/check_documentation.py --root /path/to/staging-root --baseline-tree /path/to/baseline-tree.json
```

The optional tree lets unchanged baseline paths satisfy file references and checks pinned source paths against that supplied inventory. Anchors in files absent from the local overlay remain unverified and are reported as such. The checker does not fetch external URLs, build the game, run gameplay tests or establish factual correctness; those checks need their own evidence.

## 6. Freshness review

At a milestone boundary, inspect the current default-branch commit, workflow results, releases, open PRs and changed subsystems. Refresh the evidence ledger and update any stale “current” statements. Compare the game catalogue with definitions, especially objective labels, recipe inputs/outputs, shop direction and development defaults.

Review previous unresolved findings explicitly: failed CI, fresh-profile setup, multi-step economy checks, saved-location migration and source-versus-design naming. A later date in a document header is not a substitute for rechecking the claim.

## 7. Archiving and cleanup

Retain useful rationale and history. Remove a document only when its unique content is preserved elsewhere or it is demonstrably obsolete; update internal links and record the replacement. Close superseded planning PRs only after a concrete replacement exists. Preserve Git history rather than rewriting it to hide older decisions.

The three existing persistence, economy and character documents contain useful specialised material and remain linked. The old standalone Studio phase sequences are superseded by the R0–R10 roadmap; their STUDIO IDs are preserved in the backlog.

## 8. Contributor handoff

Before starting work, read the current overview, the relevant work package, its source references and its acceptance gate. Before handing it off, leave a short work-session record with files changed, commands/results, decisions, open problems and next action. Do not rely on another contributor remembering a private chat.

## 9. When something is uncertain

Label the uncertainty and state the next observation that would resolve it. Use “source-derived, not runtime-reproduced” for the documented economy calculation. The original CI uncertainty was resolved by the public run annotation: an account billing lock prevented jobs from starting. Preserve that evidence and the separate local results in the [verification record](BASELINE_VERIFICATION_2026-11-18.md). Clear uncertainty is more useful than confident but unsupported completion language.

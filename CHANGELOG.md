# Changelog

## 20 November 2026 - Project review and player-data safety

- Preserve recovery files when primary save validation fails.
- Include crafting cursor/grid ownership in persisted snapshots; reject unsafe saves before writing and offer a retry/explicit unsaved exit.
- Reject overflowing or unknown direct inventory additions, protecting bank withdrawals.
- Reconcile current tests, current-state guides and roadmap records; publish the [full review](docs/embervale/06-development/PROJECT_REVIEW_2026-11-20.md).
- Verify 102/102 Windows SDL CTest entries, 87/87 standalone headless entries and 42/42 Python editor tests. Animation integration/quality and human starter acceptance remain open.


This file records reviewable project changes going forward. Historical work is documented in the [development history](docs/embervale/01-current-state/DEVELOPMENT_HISTORY.md); it is not retroactively presented as a set of releases.

## Unreleased

### Mining animation and ore feedback

- Add eight-way pickaxe swings, bronze/iron/steel visuals and rock-chip contact effects.
- Face the active rock and show committed ore/Mining XP rewards, including depletion.
- Share gathering presentation and interruption handling with woodcutting.
- Add editable mining art, exports, preview replay and focused state/render checks.

See the [mining record](docs/embervale/06-development/MINING_ANIMATION_2026-11-20.md).

### Woodcutting animation and reward feedback

- Add eight-way chopping poses, bronze/iron/steel axe visuals and contact chips.
- Face the active tree and stop the swing on movement, cancellation, depletion or death.
- Show floating log/XP feedback from committed gathering results, including the final tree use.
- Add editable action art, reproducible exports and focused state/renderer coverage.

See the [woodcutting record](docs/embervale/06-development/WOODCUTTING_ANIMATION_2026-11-20.md).

### Eight-way default character animation

- Add southwest, southeast, northwest and northeast player sprites and walk cycles.
- Keep diagonal facing when stopping, with distance-based steps and stable cardinal turns.
- Extend the editable atlas and export workflow, with renderer and asset checks.

See the [animation record](docs/embervale/06-development/PLAYER_EIGHT_WAY_2026-11-20.md).

### Read-only starter-world foundation

- Add an optional SDL-free `EmbervaleWorldExport` executable and typed snapshot library.
- Export all 100×100 underlying ground cells, seven resources, three stations, ten spawn definitions and explicit initialization stages/order as deterministic JSON.
- Add a reviewed complete snapshot fixture, real-world terrain/lifecycle/path/interaction checks, and independent CLI JSON verification.
- Record a fresh local Linux baseline pass (81 tests) and changed-build passes (72 headless; 83 with client/graphics), with exact environments and source hashes.

- Document placement/save ownership, the export architecture decision, actual command usage and task progress. Runtime construction, player saves and existing gameplay are unchanged.

See [export usage](docs/embervale/04-studio/STARTER_WORLD_EXPORT.md) and the [verification record](docs/embervale/06-development/BASELINE_VERIFICATION_2026-11-18.md). This is a foundation change; runtime import and editing remain planned.

### Documentation and repository organisation

- Add a root project entry point and categorised Embervale documentation index.
- Document source-verified implementation, development history, proposed game/tool design and known verification limits.
- Establish the R0–R10 roadmap, an original inventory of 87 tasks, release gates, decisions, risk register and contributor templates.
- Add a precise proposed River Crossing tile diagram with a reproducible source script.
- Reconcile the economy ledger while preserving its balance targets and useful item inventory.
- Replace stale tracked local editor settings with portable onboarding and narrow ignore-rule corrections.

### Known baseline findings

- The original documentation review did not run a local build; the later foundation verification above supplies local build and test results.
- A source-derived multi-step shop/crafting profit cycle requires runtime reproduction and a separate balance correction if confirmed.
- Starter pacing requires a genuine fresh-player profile rather than the current boosted development defaults.

The documentation and organisation portion was delivered through [PR #3](https://github.com/Adzffs/GameEngine/pull/3). It does not claim a Studio release, geographic implementation or engine fix.

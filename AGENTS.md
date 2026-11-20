# Project work reference

Before choosing the next feature, read [current status](docs/embervale/01-current-state/CURRENT_STATUS.md), the [project review](docs/embervale/06-development/PROJECT_REVIEW_2026-11-20.md) and the current roadmap/backlog. Local animation and published Mining remain separate until REL-004. ART-005 polish is owner-deferred; do not describe it as visually accepted.

REL-004 was subsequently started in a separate local worktree, then the execution environment became unavailable before combined build/export results could be retrieved. Read the [integration handoff](docs/embervale/06-development/INTEGRATION_HANDOFF_2026-11-20.md) before resuming. Recover existing uncommitted work before reapplying patches; it has not been published or verified as an integrated build.

For tasks that create or change animation, read:

1. [Animation guide](docs/embervale/05-game/ANIMATION_GUIDE.md).
2. [Animation research and findings, 20 November 2026](docs/embervale/06-development/ANIMATION_RESEARCH_2026-11-20.md).
3. [Current art and audio integration status](docs/embervale/05-game/ART_AND_AUDIO.md), the relevant batch README and the current action's implementation/tests.

The dated research separates local prototype `688221b` from published GitHub source. Verify the checkout and available helpers before reusing its commands, counts or implementation assumptions. Future gathering work should preserve the shared Mining/Woodcutting presentation boundary.

Record new findings and verification in the action's dated report, and link it from the documentation index. Keep reusable lessons in the animation guide. Preserve gameplay authority, editable masters, directional equipment consistency and unrelated work.

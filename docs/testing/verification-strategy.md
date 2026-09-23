# Verification Strategy

## Evidence standard

For each claim, record the commit, test name or command, environment, result, and any limitation. A test file existing is not a test result. A passing focused test does not prove unrelated systems or full gameplay behavior.

## Test layers

1. **Unit tests:** deterministic rules such as inventory transactions, recipe matching, drop tables, quest transitions and save codecs.
2. **World integration tests:** authoritative action completion, command validation, resource awards, quest progress, loot and persistence interactions.
3. **Client integration tests:** user input, layout/hitbox alignment, feedback and reconciliation against authoritative results.
4. **Lifecycle tests:** save/load, restart, migration, cancellation, disconnect, stale result, full inventory and retry behavior.
5. **CI:** required headless and client builds/tests on supported platforms.
6. **Playtest evidence:** new player understanding, task completion, usability, balance and next-goal clarity.

## Current repository checks to complete

- GitHub Actions [run #53](https://github.com/Adzffs/GameEngine/actions/runs/35820586326) fails both Linux headless and Windows UCRT64 jobs. Inspect job logs and record root cause before closing M0.
- Verify current Mining Basics behavior and its v2 migration correction on `master` at `aa7525150621c688efdcb4a013d0863b51f7c73a`.
- Resolve World-level gathering quest progress and failure-path coverage noted in the independent review.
- Verify quest tracking leaves gathering RNG consumption and order unchanged.
- Expand or confirm command rejection coverage for invalid actor/target/NPC/session/dialogue/node/state/ID conditions.
- Verify generic quest definitions, ordered behavior, restore transactionality, exact historical save schemas and deterministic encoding.
- Verify the shop one-command-in-flight lock against duplicate, stale and wrong results.

## Later feature test checklists

### Grid crafting

Recipe shape recognition; rotations/mirroring policy; shapeless ingredient counts; ingredient movement/removal; craft output acceptance; full output inventory; window close; cancel; stale request; duplicate request; disconnect; no ingredient loss/duplication; same recipe source for UI and validation.

### Pets

Acquisition eligibility; duplicate acquisition; ownership; active/follower limits; combat/utility effects if any; save/load and migration; death/recovery if relevant; UI state; server authority; economy and alternative progression.

### Bosses and rare loot

Encounter phase transitions; attack telegraphs; defeat/victory; death and retry; server-only reward roll; exactly-once rewards; baseline kill value; rare drop probability/weight; inventory full; reconnect/retry; deterministic seed behavior; loot table content validation and long-run economy review.

### New player slice

Fresh save; understandable lead; preparation choices; useful gathering/crafting; readable boss; reward feedback; visible next goal; save/restart continuity; no developer explanation.

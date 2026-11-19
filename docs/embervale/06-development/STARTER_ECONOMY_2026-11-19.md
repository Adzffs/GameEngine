# Starter economy correction

**Date:** 19 November 2026\
**Reproduction baseline:** master `9af163de20acb78e346e12ed36105266436a7f95` (PR #29).\
**Work packages:** GAME-008 and the first bounded part of GAME-004.\
**Status:** Production replay and builds passed; regression suites and complete player pacing remain deferred.

## Confirmed trade and correction

The Log -> Wooden planks -> Wooden shield trade was reproduced through the
production World on the actual River Crossing blockout. Two cycles ran in the
same World, using normal approach movement, shop sessions and station actions.
Each cycle buys three Logs, crafts twelve planks, crafts two shields and sells
both shields. No Logs, planks or shields remain after either cycle.

| Measurement | Before | Corrected |
|---|---:|---:|
| Log buy / sell | 1 / 1 | 10 / 1 |
| Shield buy / sell | 30 / 12 | 20 / 12 |
| Three-Log purchase | 3 coins | 30 coins |
| Two-shield sale | 24 coins | 24 coins |
| Net coins per complete cycle | +21 | -6 |
| Recorded starting / first / second balance | 3 / 24 / 45 | 60 / 54 / 48 |
| Simulation ticks across both cycles | 267 | 267 |

The corrected replay starts with 60 explicit coins so both loss-making purchases
can execute. This is an isolated reproduction budget, not a fresh-player grant.
The earlier transaction path had no finite stock gate; repeat buying succeeded
in the same running World. Session validation and inventory transactions
alone did not prevent this conversion from creating coins.

Evidence: [before](economy/2026-11-19/timber-before.json),
[after](economy/2026-11-19/timber-after.json),
[current catalogue](economy/2026-11-19/starter-catalogue.json).

## Starter decisions

Gathered timber retains its four-plank yield and a shield still sells for twelve
coins. Purchased timber is now a convenience expense. A shield costs twenty
coins, equal to the ten-coin Gathering Basics reward plus the ten-coin Mining
Basics reward. Spending those rewards on food or keeping the money remains an
alternative to buying the shield. The Bronze sword purchase remains forty coins.

The report derives the following craft route from the existing definitions:

| Target | Materials |
|---|---|
| One Bronze sword and one Wooden shield | 2 normal Logs, 3 Copper ore, 3 Tin ore |
| Processing | 8 planks and 3 Bronze bars |
| Equipment inputs | Sword: 3 bars + 1 plank; shield: 6 planks |
| Remainder | 1 plank, no bars |

This is a material budget. It does not establish a player's gathering success,
navigation, discovery, bank use, combat experience or elapsed upgrade time.

## Production-cost validation

`EconomyBalanceModel::GetProductionCosts` starts from each shop's buy prices and
propagates the cheapest known material cost through intermediate recipes.
Fractional per-unit costs describe repeatable whole batches; they do not imply
that a player can purchase fractional Logs. For the shield, six planks cost an
amortized fifteen coins and sell for twelve: minus three coins per shield.
The two-shield runtime cycle accounts for the whole material balance.

`ContentValidator` now rejects shop-funded recipe cash-outs with zero or positive
margin, including the old one-coin Log and the eight-coin break-even Log price.
Existing malformed-shop checks run first. Improving production cycles that do
not converge within the bounded catalogue passes require review and fail
validation. This validation is part of the existing startup content checks.

Missing material prices remain unknown. A `null` recipe margin means that a
fully shop-funded material route or an output cash-out is unavailable; it is not
an anti-arbitrage pass. The current Bronze recipes need gathered ore, so they do
not get a fabricated zero-cost margin. Scope is the present single-output
recipe catalogue and one shop at a time. Cross-shop trade, future coproducts,
quests and combat rewards need their own analysis. This is not a general claim
that all future economy loops are safe.

## Reproduce the report

With the documented UCRT64 toolchain and a configured client build, run from the
repository root:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
cmake -S . -B build-client -DGAMEENGINE_BUILD_CONTENT_TOOLS=ON
cmake --build build-client --target GameEngine EmbervaleEconomyReport --parallel 6
.\build-client\EmbervaleEconomyReport.exe
.\build-client\EmbervaleEconomyReport.exe --replay-timber-cycle 60 2 --world content\worlds\river-crossing.blockout.json
```

No arguments prints catalogue JSON, starter quest budget, derived recipe margins
and the sword/shield material plan. `--help` documents bounded replay arguments.
Without `--world`, replay uses the compiled development layout. An insufficient
budget or rejected action fails the replay; it does not invent a successful
transaction. The optional target lives under `GAMEENGINE_BUILD_CONTENT_TOOLS`.

Replay creates a fresh in-memory character, seeds only the stated coin budget,
and never opens player saves. It uses the supported inventory-backed
`CraftRecipeCommand` path, not cursor-driven SDL crafting. The simulation is
advanced directly, so tick totals are not observed wall-clock player timings.

## Execution and remaining acceptance

| Check | Outcome |
|---|---|
| Baseline production trade on River Crossing | Reproduced twice; +21 coins each cycle |
| Corrected production trade on River Crossing | Replayed twice; -6 coins each cycle |
| Current shop validation and production-cost convergence | Passed in catalogue report |
| C++ core, SDL game and economy-report build | Passed |
| Dedicated River Crossing launcher build | Passed with content tools and tests disabled |
| Regression suites | Deferred at the owner's request; not run |
| Complete fresh-player upgrade / visible crafting acceptance | Not measured in this slice |

Deferred regression cases cover the old profitable price and the break-even
price. Existing shop/client expectations now follow the authored Log price.
These source changes are not reported as executed tests.

Next GAME-004 work is measuring the complete fresh-player journey, including
gathering attempts, travel, requirements, inventory/bank use, food pressure and
buying versus crafting. The 10-15 minute first-upgrade goal remains a target.
GAME-008 and GAME-004 retain `in_progress` while their broader acceptance remains
outstanding. The correction changes two shop buy prices and adds analysis; it
does not change recipe yields, quest rewards or player-save/world formats.

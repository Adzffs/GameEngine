# Embervale economy ledger

**Status:** Starter timber trade reproduced and corrected; complete player timings remain unmeasured.\
**Updated:** 19 November 2026.\
**Reproduction baseline:** `9af163de20acb78e346e12ed36105266436a7f95`; current values include the correction in this revision.\
**Related:** [Documentation home](embervale/README.md), [content catalogue](embervale/05-game/CONTENT_CATALOGUE.md), [game design](embervale/05-game/GAME_DESIGN.md).

This ledger records the purpose, acquisition and use of current items. Numerical definitions come from the current source linked below. Design targets describe intended outcomes, not completed playtest findings. The multi-step timber trade has been reproduced and corrected; see the [execution record](embervale/06-development/STARTER_ECONOMY_2026-11-19.md). Wider pacing remains open.

## Balance targets

- A new player should earn the first deliberate equipment upgrade within 10–15 minutes. This target has not been measured here.
- Each gathering tier should remain useful after the next tier unlocks.
- Processing should improve usefulness without permitting repeatable buy–craft–sell coin generation.
- Combat, gathering and crafting should each contribute something the other activities need.
- Include gathering attempts, inventory capacity, travel, banking and requirement discovery in upgrade time.
- Shops should support recovery and convenience; crafting should remain a worthwhile upgrade route.
- Rare rewards may accelerate progress but should not be the only practical source of an essential tool.

## Reading prices and sources

**Buy** means the player pays the shop. **Sell** means the player receives coins from the shop. A dash means the corresponding transaction is not offered by the current definition. Copper, Tin and Coal have **sell** prices but no buy prices; the previous ledger reversed this distinction. Source: [ShopDefinition.h](../src/Shop/ShopDefinition.h), [ShopDefinitionDatabase.cpp](../src/Shop/ShopDefinitionDatabase.cpp) and [World::TryProcessShopTransaction](../src/World/World.cpp).

“Source” below names authored acquisition routes. It does not claim every route has been replayed or timed. “Use” distinguishes a consumed crafting input from equipment that remains in the player's possession.

## Current item ledger

| Item | Tier | Sources | Current uses / sinks | Processing or acquisition | Shop buy / sell | Progression role and required follow-up |
|---|---:|---|---|---|---|---|
| Log | 1 | Normal tree; supplies shop | Plank conversion; Gathering Basics objective; shop sale | One Log makes four Wooden planks; base resource success 70%, one use, 10-tick respawn | 10 / 1 | Gathered timber remains useful; purchased timber costs more than the complete crafted resale |
| Oak log | 2 | Oak tree at Woodcutting 5 | Plank conversion | One Oak log makes six Wooden planks; base success 55%, three uses, 15-tick respawn | — / — | More efficient plank production; measure its usefulness before inventing another raw-material sink |
| Willow log | 3 | Willow tree at Woodcutting 10 | Plank conversion | One Willow log makes eight Wooden planks; base success 45%, five uses, 20-tick respawn | — / — | Highest current timber conversion output; measure total acquisition and processing time |
| Copper ore | 1 | Copper rock; Mossling weighted reward | Bronze bar input; Mining Basics objective; shop sale | One Copper plus one Tin makes one Bronze bar; resource base success 70% | — / 2 | Beginner paired input and a gathering-to-coins route |
| Tin ore | 1 | Tin rock; Mossling weighted reward | Bronze bar input; shop sale | Paired 1:1 with Copper; resource base success 65% | — / 2 | Completes the bronze loop; compare supply and travel with Copper |
| Iron ore | 2 | Iron rock; Mossling/Ashfang weighted reward; Stoneback guaranteed reward | Iron bar or Steel bar input | One ore per Iron bar; one ore plus two Coal per Steel bar; resource base success 50% | — / — | Meaningful choice between two metal tiers; both have existing equipment recipes |
| Coal | 3 | Coal rock; Mossling guaranteed reward; Stoneback weighted reward | Steel bar input; shop sale | Two Coal per Steel bar; resource base success 40% | — / 4 | Links gathering/combat to Steel and provides a coin alternative; unavailable as a shop purchase |
| Bronze bar | 1 | Furnace recipe from Copper and Tin | Bronze axe, Bronze pickaxe, Bronze sword | Three base ticks, Smithing 1, six XP per bar | — / — | Existing first metal equipment tier; measure complete acquisition time |
| Iron bar | 2 | Furnace recipe from Iron ore | Iron axe and Iron pickaxe | Three base ticks, Smithing 10, 13 XP per bar | — / — | Existing middle-tier gathering equipment; no longer an item with no equipment sink |
| Steel bar | 3 | Furnace recipe; Stoneback weighted reward; Cinder Warden guaranteed reward; alternate Heart of the Ash completion | Steel axe and Steel pickaxe | Four base ticks, Smithing 20, 20 XP per bar; one Iron ore plus two Coal | — / — | Crafted and combat-assisted routes; compare their total effort |
| Coins | 1 | Selected monster rewards; quest rewards; permitted shop sales | Shop purchases | Income includes combat, quests and conversion of saleable gathered/crafted items | Currency | Track income and spending by source; do not describe the economy as combat-only |
| Cooked meat | 1 | Ashfang guaranteed reward; shop; quest rewards including the alternate Shield or Shelter path | Consumed for five health; shop sale | Non-stackable food | 8 / 3 | Efficient shop healing but uses inventory slots; compare actual use under damage pressure |
| Healing tonic | 2 | Shop | Consumed for 12 health; shop sale | Stackable food | 20 / 6 | Inventory compression with slightly lower healing per coin than meat |
| Bronze axe | 1 | Fresh/development starting inventory; shop; furnace forging recipe | Equipped Woodcutting tool; shop sale | Recipe: two Bronze bars plus one Wooden plank | 25 / 10 | Recovery and basic access; preserve a realistic replacement path |
| Iron axe | 2 | Furnace forging recipe | Equipped Woodcutting tool | Two Iron bars plus one Wooden plank; Woodcutting 5 use requirement | — / — | Existing deterministic upgrade; measure time to acquire and usefulness |
| Steel axe | 3 | Furnace forging recipe; Cinder Warden weighted reward | Equipped Woodcutting tool | Two Steel bars plus one Wooden plank; Woodcutting 10 use requirement | — / — | Crafted route plus a possible combat accelerator |
| Bronze pickaxe | 1 | Fresh/development starting inventory; shop; furnace forging recipe | Equipped Mining tool; shop sale | Three Bronze bars plus two Wooden planks | 25 / 10 | Basic access and recovery; include plank costs when comparing routes |
| Iron pickaxe | 2 | Furnace forging recipe | Equipped Mining tool | Three Iron bars plus two Wooden planks; Mining 5 use requirement | — / — | Existing middle-tier upgrade; test with a genuine starter profile |
| Steel pickaxe | 3 | Furnace forging recipe | Equipped Mining tool | Three Steel bars plus two Wooden planks; Mining 10 use requirement | — / — | Existing high-tier upgrade; measure acquisition rather than assuming it is paced correctly |
| Bronze sword | 1 | Furnace forging recipe; shop | Equipped weapon; shop sale | Three Bronze bars plus one Wooden plank | 40 / 15 | First craftable sword; it is not the current Gathering Basics objective |
| Developer Godsword | Debug | Development/debug access | Equipped debug weapon | Deliberately extreme test stats | — / — | Keep out of normal player rewards, shops and release content |
| Wooden shield | 1 | Crafting table recipe; shop | Equipped shield; Shield or Shelter objective/completion path; shop sale | Six Wooden planks; two base ticks | 20 / 12 | Early craft or purchase using the combined twenty-coin basic quest rewards |
| Wooden planks | 1 | Normal, Oak and Willow conversion recipes | Shield: six; any axe: one; any pickaxe: two; Bronze sword: one | Stackable; outputs four/six/eight per respective log | — / — | Common cross-skill input and inventory compression; include all forging sinks |

Catalogue sources: [ItemDatabase.cpp](../src/Item/ItemDatabase.cpp), [ResourceDatabase.cpp](../src/World/Object/Resource/ResourceDatabase.cpp), [RecipeDatabase.cpp](../src/Recipe/RecipeDatabase.cpp), [DevelopmentRewardTables.cpp](../src/Reward/DevelopmentRewardTables.cpp), [QuestDefinitionDatabase.cpp](../src/Quest/QuestDefinitionDatabase.cpp), [ShopDefinitionDatabase.cpp](../src/Shop/ShopDefinitionDatabase.cpp), [Player.cpp](../src/Player/Player.cpp).

## Known balance risks

### Corrected timber trade (GAME-008)

The production World reproduced the old three-Log -> twelve-plank -> two-shield
cycle twice on River Crossing: **3 -> 24 -> 45 coins**. Commands used normal
approach movement, shop sessions and station actions, with no materials left
after either cycle. There was no finite stock gate preventing repeat purchases.

The Log buy price is now **10**, with sell price **1**. Buying three Logs costs
**30**, while the two shields still sell for **24**. The corrected runtime replay
records **60 -> 54 -> 48 coins**, a **six-coin loss per complete cycle**. The
sixty starting coins are an explicit isolated replay budget, not a player grant.
The shield buy price falls from thirty to **twenty** coins to match the combined
Gathering Basics and Mining Basics coin rewards. Yields and rewards are unchanged.

The shared model now follows intermediate recipe costs, and startup content
validation rejects nonnegative shop-funded production margins. The older direct
`GetCraftingMargin` helper remains available but cannot evaluate the shield's
unpriced plank input on its own. Regression cases for the former profitable
price and the break-even price are added but have not been executed.

[Before/after JSON, method, commands and remaining acceptance](embervale/06-development/STARTER_ECONOMY_2026-11-19.md).
This is production command replay, not a cursor-driven UI playthrough or a
measurement of a new player's complete progression time.

### Pacing and ongoing demand

Permanent tools and equipment provide one-time goals, while food creates repeated demand. The existence of these sinks does not establish a healthy long-term economy. Measure which materials accumulate, which purchases recur and which upgrade routes players avoid. The current catalogue supports both gathering-to-coins sales and combat-to-material rewards; examine the complete flow when tuning either side.

Keep Oak and Willow useful through their existing conversion advantage before adding new items. New sinks should create worthwhile choices, rather than simply consume resources because a spreadsheet shows excess stock.

## Starter-profile requirement

Fresh and development characters receive Bronze axe and Bronze pickaxe. The
development profile also grants boosted XP under the current debug configuration.
Use `--profile fresh --no-save` or Studio's Fresh player option for level-one,
zero-XP starting skills, no developer weapon and temporary progress. Existing
explicit saves restore their own state rather than becoming fresh characters.
See the [profile contract](embervale/05-game/FRESH_PLAYER_PROFILE.md).

The current source-derived sword-and-shield budget is **two normal Logs, three
Copper ore and three Tin ore**, leaving one plank. Alternatively, the two basic
quest rewards cover the twenty-coin shield purchase. Neither budget establishes
elapsed upgrade time. Before timing, record XP, items, coins, equipment, quest
state, recipe knowledge and save origin. The trade replay seeds its coin budget
explicitly and does not stand in for gathering those coins.

## Anti-arbitrage rule and validation scope

For each fully purchasable production chain, total output sell value should be less than total input buy cost. Count intermediate recipes, quantity conversions, alternative inputs and leftover outputs. A one-step calculation is useful but incomplete when ingredients can be crafted.

Use the following distinction in reports:

| Result | Meaning |
|---|---|
| Direct margin calculated | All immediate ingredients are purchasable and the output is saleable |
| Direct margin unavailable | The helper lacks a price for an input or output; this is not a passing anti-arbitrage result |
| Multi-step path evaluated | The model propagates known shop-funded costs through intermediate recipes; a complete replay names conversions and whole-batch remainders |
| Runtime reproduced | A recorded game/test sequence confirms the transactions and resulting inventory/coins |

The current validator covers each shop separately and the present single-output recipes. Missing gathered-input prices remain unknown; `null` is not a passing calculation. Cross-shop routes, future coproducts and reward loops need separate analysis. Nonconverging cost cycles fail validation for review.

Travel and action time belong in earnings-rate analysis, but they do not change the sign of a profit cycle. Where a deliberate production profit is desired in future, change the design policy explicitly and evaluate the consequences rather than describing it as an anti-arbitrage pass.

## Source-derived production ceilings

The authoritative clock interval is 600 ms, equivalent to 100 ticks per minute. The following values are calculated station-only ceilings, not measured player earnings or progression rates. They exclude gathering, navigation, inventory management, interaction overhead and interruptions. Sources: [Clock.h](../src/Time/Clock.h), [RecipeProductionPacing.h](../src/Recipe/RecipeProductionPacing.h), [EconomyBalanceModel.h](../src/Economy/EconomyBalanceModel.h).

| Activity | Entry-level ceiling | Mastery ceiling from current formula | XP ceiling | Interpretation |
|---|---:|---:|---:|---|
| Bronze bars | 33.3/min at Smithing 1 | 50/min from Smithing 11 | 200–300/min | Duration falls from three ticks to two |
| Steel bars | 25/min at Smithing 20 | 50/min from Smithing 40 | 500–1,000/min | Duration falls from four ticks to two; input supply remains important |
| Cooked meat | 0.625 health/coin | — | — | Five healing divided by eight-coin buy price |
| Healing tonic | 0.6 health/coin | — | — | Twelve healing divided by twenty-coin buy price |

## Next implementation and validation work

1. Execute the deferred economy regressions before balance signoff; retain the production before/after record.
2. Record complete gathering-to-equipment timings using the implemented fresh profile.
3. Verify the recipe UI exposes requirements, output, duration, capacity and access clearly.
4. Compare crafting and buying the first sword/shield, including coin sources and travel.
5. Measure tool-upgrade time, food consumption, contention and banking on River Crossing.
6. Compare crafted and combat-assisted Steel acquisition before adding tiers or rewards.
7. Extend analysis when actual multi-shop or coproduct content requires it; preserve unknown-versus-passing results.
8. Update this ledger when definitions change and separate measured outcomes from targets.

## Update history

The 18 November 2026 documentation revision corrects reversed ore/fuel shop prices, recognises existing forging recipes, completes plank and equipment relationships, broadens coin-source accounting, removes the incorrect Bronze sword tutorial requirement and records the development-XP caveat. It also adds the source-derived profit-cycle finding. Gameplay code and balance values are unchanged by these documentation edits.

The 19 November 2026 gameplay revision reproduces and corrects GAME-008, lowers the first shield buy price, adds intermediate production-cost validation and publishes JSON evidence. Builds and production replay passed; test suites and complete starter pacing remain deferred.

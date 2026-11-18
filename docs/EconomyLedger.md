# Embervale economy ledger

**Status:** Source-reviewed balance ledger; balance targets and player timings remain unverified.  
**Updated:** 18 November 2026.  
**Source baseline:** `e9d32932e8f9b27e96283313cca320058792a81f`.  
**Related:** [Documentation home](embervale/README.md), [content catalogue](embervale/05-game/CONTENT_CATALOGUE.md), [game design](embervale/05-game/GAME_DESIGN.md).

This ledger records the purpose, acquisition and use of current items. Numerical definitions come from the pinned source linked below. Design targets describe intended outcomes, not completed playtest findings. The current source contains an apparent multi-step shop/crafting profit cycle; see **Known balance risks** before treating the economy as ready.

## Balance targets

- A new player should earn the first deliberate equipment upgrade within 10–15 minutes. This target has not been measured here.
- Each gathering tier should remain useful after the next tier unlocks.
- Processing should improve usefulness without permitting repeatable buy–craft–sell coin generation.
- Combat, gathering and crafting should each contribute something the other activities need.
- Include gathering attempts, inventory capacity, travel, banking and requirement discovery in upgrade time.
- Shops should support recovery and convenience; crafting should remain a worthwhile upgrade route.
- Rare rewards may accelerate progress but should not be the only practical source of an essential tool.

## Reading prices and sources

**Buy** means the player pays the shop. **Sell** means the player receives coins from the shop. A dash means the corresponding transaction is not offered by the current definition. Copper, Tin and Coal have **sell** prices but no buy prices; the previous ledger reversed this distinction. Source: [ShopDefinition.h](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Shop/ShopDefinition.h), [ShopDefinitionDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Shop/ShopDefinitionDatabase.cpp) and [World::TryProcessShopTransaction](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/World.cpp).

“Source” below names authored acquisition routes. It does not claim every route has been replayed or timed. “Use” distinguishes a consumed crafting input from equipment that remains in the player's possession.

## Current item ledger

| Item | Tier | Sources | Current uses / sinks | Processing or acquisition | Shop buy / sell | Progression role and required follow-up |
|---|---:|---|---|---|---|---|
| Log | 1 | Normal tree; supplies shop | Plank conversion; Gathering Basics objective; shop sale | One Log makes four Wooden planks; base resource success 70%, one use, 10-tick respawn | 1 / 1 | Basic timber input; resolve the profitable shield conversion chain before treating this price as stable |
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
| Bronze axe | 1 | Development starting inventory; shop; furnace forging recipe | Equipped Woodcutting tool; shop sale | Recipe: two Bronze bars plus one Wooden plank | 25 / 10 | Recovery and basic access; preserve a realistic replacement path |
| Iron axe | 2 | Furnace forging recipe | Equipped Woodcutting tool | Two Iron bars plus one Wooden plank; Woodcutting 5 use requirement | — / — | Existing deterministic upgrade; measure time to acquire and usefulness |
| Steel axe | 3 | Furnace forging recipe; Cinder Warden weighted reward | Equipped Woodcutting tool | Two Steel bars plus one Wooden plank; Woodcutting 10 use requirement | — / — | Crafted route plus a possible combat accelerator |
| Bronze pickaxe | 1 | Development starting inventory; shop; furnace forging recipe | Equipped Mining tool; shop sale | Three Bronze bars plus two Wooden planks | 25 / 10 | Basic access and recovery; include plank costs when comparing routes |
| Iron pickaxe | 2 | Furnace forging recipe | Equipped Mining tool | Three Iron bars plus two Wooden planks; Mining 5 use requirement | — / — | Existing middle-tier upgrade; test with a genuine starter profile |
| Steel pickaxe | 3 | Furnace forging recipe | Equipped Mining tool | Three Steel bars plus two Wooden planks; Mining 10 use requirement | — / — | Existing high-tier upgrade; measure acquisition rather than assuming it is paced correctly |
| Bronze sword | 1 | Furnace forging recipe; shop | Equipped weapon; shop sale | Three Bronze bars plus one Wooden plank | 40 / 15 | First craftable sword; it is not the current Gathering Basics objective |
| Developer Godsword | Debug | Development/debug access | Equipped debug weapon | Deliberately extreme test stats | — / — | Keep out of normal player rewards, shops and release content |
| Wooden shield | 1 | Crafting table recipe; shop | Equipped shield; Shield or Shelter objective/completion path; shop sale | Six Wooden planks; two base ticks | 30 / 12 | Existing early craft; its sell value contributes to the source-derived profit cycle |
| Wooden planks | 1 | Normal, Oak and Willow conversion recipes | Shield: six; any axe: one; any pickaxe: two; Bronze sword: one | Stackable; outputs four/six/eight per respective log | — / — | Common cross-skill input and inventory compression; include all forging sinks |

Catalogue sources: [ItemDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Item/ItemDatabase.cpp), [ResourceDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Object/Resource/ResourceDatabase.cpp), [RecipeDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Recipe/RecipeDatabase.cpp), [DevelopmentRewardTables.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Reward/DevelopmentRewardTables.cpp), [QuestDefinitionDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Quest/QuestDefinitionDatabase.cpp), [ShopDefinitionDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Shop/ShopDefinitionDatabase.cpp), [Player.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Player/Player.cpp).

## Known balance risks

### Multi-step Log → planks → shield profit cycle

**Status: SOURCE-DERIVED arithmetic; runtime reproduction required.** This is tracked as `GAME-008` in the proposed backlog. No economy fix is included in this documentation update.

The definitions imply the following material-complete cycle:

1. Begin with at least **three coins** and enough inventory capacity. Open a valid supplies-shop session with Mara.
2. Buy three Logs at one coin each: spend **3 coins**.
3. At the crafting table, perform the supported Log-to-plank craft three times: obtain **12 Wooden planks**.
4. Craft two Wooden shields using six planks each: consume all 12 planks.
5. Return to Mara, open a valid shop session and sell both shields at 12 coins each: receive **24 coins**.
6. The inferred net gain is **21 coins per cycle**, excluding travel/action time, with no leftover crafting inputs.

The current transaction code checks session validity, quantity, price, integer bounds and an atomic inventory transaction. No finite stock gate is present in the inspected `TryProcessShopTransaction` path. That inspection and the arithmetic do not replace a complete gameplay replay through recipe discovery, crafting and shop access. Sources: [World.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/World.cpp), [recipes](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Recipe/RecipeDatabase.cpp), [shop values](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Shop/ShopDefinitionDatabase.cpp).

The direct `EconomyBalanceModel::GetCraftingMargin` helper does not prove this cycle safe. It checks the immediate recipe ingredients and returns no result when an input is not directly purchasable. Wooden planks have no shop entry, so checking the shield recipe alone cannot evaluate the earlier Log conversion. Add a multi-stage reproduction and appropriate regression coverage before selecting a correction. Source: [EconomyBalanceModel.h](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Economy/EconomyBalanceModel.h).

Possible corrections include changing source availability, prices, conversion economics or trade rules. Choose one using the intended recovery and crafting experience; do not silently alter numbers in documentation or assume extra travel makes a positive repeatable cycle acceptable under the stated design rule.

### Pacing and ongoing demand

Permanent tools and equipment provide one-time goals, while food creates repeated demand. The existence of these sinks does not establish a healthy long-term economy. Measure which materials accumulate, which purchases recur and which upgrade routes players avoid. The current catalogue supports both gathering-to-coins sales and combat-to-material rewards; examine the complete flow when tuning either side.

Keep Oak and Willow useful through their existing conversion advantage before adding new items. New sinks should create worthwhile choices, rather than simply consume resources because a spreadsheet shows excess stock.

## Starter-profile requirement

Current development defaults grant Bronze axe and Bronze pickaxe. With `ENABLE_TEST_PLAYER=true`, they also grant **2,000 Woodcutting XP, 5,000 Mining XP and 5,000 Smithing XP**. These are debug conveniences, not evidence that a level-one player can reach an upgrade in the target time. Sources: [DevelopmentConfig.h](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Core/DevelopmentConfig.h) and [Player.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Player/Player.cpp).

Before timing the opening loop, record the player's starting XP, items, coins, equipment, quest state, recipe knowledge and save origin. Use a separately defined starter test profile without accidental development boosts. Preserve the current development profile for regression tests that depend on it; changing production defaults is a separate implementation decision.

## Anti-arbitrage rule and validation scope

For each fully purchasable production chain, total output sell value should be less than total input buy cost. Count intermediate recipes, quantity conversions, alternative inputs and leftover outputs. A one-step calculation is useful but incomplete when ingredients can be crafted.

Use the following distinction in reports:

| Result | Meaning |
|---|---|
| Direct margin calculated | All immediate ingredients are purchasable and the output is saleable |
| Direct margin unavailable | The helper lacks a price for an input or output; this is not a passing anti-arbitrage result |
| Multi-step path evaluated | The report names every conversion and the complete material balance |
| Runtime reproduced | A recorded game/test sequence confirms the transactions and resulting inventory/coins |

Travel and action time belong in earnings-rate analysis, but they do not change the sign of a profit cycle. Where a deliberate production profit is desired in future, change the design policy explicitly and evaluate the consequences rather than describing it as an anti-arbitrage pass.

## Source-derived production ceilings

The authoritative clock interval is 600 ms, equivalent to 100 ticks per minute. The following values are calculated station-only ceilings, not measured player earnings or progression rates. They exclude gathering, navigation, inventory management, interaction overhead and interruptions. Sources: [Clock.h](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Time/Clock.h), [RecipeProductionPacing.h](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Recipe/RecipeProductionPacing.h), [EconomyBalanceModel.h](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Economy/EconomyBalanceModel.h).

| Activity | Entry-level ceiling | Mastery ceiling from current formula | XP ceiling | Interpretation |
|---|---:|---:|---:|---|
| Bronze bars | 33.3/min at Smithing 1 | 50/min from Smithing 11 | 200–300/min | Duration falls from three ticks to two |
| Steel bars | 25/min at Smithing 20 | 50/min from Smithing 40 | 500–1,000/min | Duration falls from four ticks to two; input supply remains important |
| Cooked meat | 0.625 health/coin | — | — | Five healing divided by eight-coin buy price |
| Healing tonic | 0.6 health/coin | — | — | Twelve healing divided by twenty-coin buy price |

## Next implementation and validation work

1. Reproduce `GAME-008` through actual supported commands; choose and test a correction for the multi-stage profit cycle.
2. Establish an explicit starter profile and record complete gathering-to-equipment timings.
3. Verify the existing recipe UI exposes requirements, output, duration, capacity and access clearly. Recipes already present in source should not be listed as unimplemented.
4. Measure crafting and buying alternatives for the first sword and shield; include coin sources and travel in both comparisons.
5. Measure tool-upgrade time, food consumption, resource contention and banking frequency on the revised map.
6. Compare crafted and combat-assisted Steel acquisition before adding further tiers or rewards.
7. Expand the balance model to cover meaningful multi-stage chains and distinguish unavailable calculations from passing checks.
8. Update this ledger whenever the referenced definitions change; attach a new source commit and separate measured outcomes from targets.

## Update history

The 18 November 2026 documentation revision corrects reversed ore/fuel shop prices, recognises existing forging recipes, completes plank and equipment relationships, broadens coin-source accounting, removes the incorrect Bronze sword tutorial requirement and records the development-XP caveat. It also adds the source-derived profit-cycle finding. Gameplay code and balance values are unchanged by these documentation edits.

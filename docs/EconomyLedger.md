# Embervale economy ledger

This ledger is the balancing contract for the starter region. It records what every current item is for, where it enters the economy, and how it leaves. Values marked **gap** describe content that exists in code but does not yet have a healthy player-facing role.

## Balance targets

- A new player should earn the first deliberate equipment upgrade within 10–15 minutes.
- Each gathering tier should remain useful after the next tier unlocks.
- Processing should improve usefulness, not create free money through shop arbitrage.
- Combat, gathering, and crafting should each contribute something the other two need.
- A full inventory and the trip to a station or bank are part of the cost; no route should be balanced from action time alone.
- Shops provide recovery and convenience. Crafted upgrades should be preferable to buying every upgrade outright.
- Rare monster rewards may accelerate progression, but must not be the only source of an essential tool.

## Current item ledger

| Item | Tier | Sources | Current sinks | Processing / acquisition | Shop value | Pressure and progression role | Required follow-up |
|---|---:|---|---|---|---|---|---|
| Log | 1 | Normal tree; shop; starting-area gathering | 1 -> 4 Wooden planks | 70% gather roll, 1 use, 10-tick respawn | Buy 1 / sell 1 | Stackable only after processing, so it teaches inventory pressure and the workshop loop | Keep as the universal basic timber input |
| Oak log | 2 | Oak tree at Woodcutting 5 | 1 -> 6 Wooden planks | 55% roll, 3 uses, 15-tick respawn | Not traded | More efficient plank production for shields and forged handles | Add a distinct long-term sink only when it serves a new decision |
| Willow log | 3 | Willow tree at Woodcutting 10 | 1 -> 8 Wooden planks | 45% roll, 5 uses, 20-tick respawn | Not traded | Best plank efficiency and inventory compression | Add a distinct long-term sink only when it serves a new decision |
| Copper ore | 1 | Copper rock; Mossling; shop | Bronze bar, 1 per bar | 70% gather roll; paired 1:1 with Tin | Buy 2 | Early mining half-input; unstacked travel cost | Add sensible sell value only after anti-arbitrage checks |
| Tin ore | 1 | Tin rock; Mossling; shop | Bronze bar, 1 per bar | 65% gather roll; paired 1:1 with Copper | Buy 2 | Completes the first metal loop | Same as Copper |
| Iron ore | 2 | Iron rock; Mossling; Ashfang; Stoneback | Iron bar 1:1; Steel bar 1:1 | 50% gather roll; competes between Iron and Steel | Not traded | Useful branch point, but bars currently have no equipment sink | Add smithing recipes and later a modest sell value |
| Coal | 3 | Coal rock; Mossling; Stoneback; shop | Steel bar, 2 per bar | 40% gather roll; 2 needed per Steel bar | Buy 4 | Controls Steel pace and connects combat drops to crafting | Retain as the main Steel bottleneck |
| Bronze bar | 1 | Furnace: 1 Copper + 1 Tin | Bronze axe, pickaxe, and sword | 3 ticks, 6 Smithing XP; gear costs 2–3 bars | Not traded | First metal equipment tier and a deterministic combat upgrade | Playtest whether the 4–6 ore cost reaches the 10–15 minute target |
| Iron bar | 2 | Furnace: 1 Iron ore | Iron axe and pickaxe | 3 ticks, Smithing 10, 13 XP; gear costs 2–3 bars | Not traded | Deterministic mid-tier gathering upgrades | Add another long-term sink only after upgrade pacing is measured |
| Steel bar | 3 | Furnace: 1 Iron ore + 2 Coal; Stoneback/Cinder Warden drop | Steel axe and pickaxe | 4 ticks, Smithing 20, 20 XP; gear costs 2–3 bars | Not traded | Prestige gathering tier with both crafted and combat-assisted routes | Retain high input cost and compare both acquisition routes |
| Coins | 1 | Ashfang and Cinder Warden drops | Food and starter equipment purchases | Combat-paced; no gathering conversion | Currency | Gives combat an economic identity | Add useful repeat purchases before adding more coin sources |
| Cooked meat | 1 | Ashfang guaranteed drop; shop | Consumed for 5 health | Immediate consumable | Buy 8 / sell 3 | Repeatable combat sink and recovery tool | Tune healing against encounter damage after playtest telemetry |
| Bronze axe | 1 | Development starting inventory; shop | Equipped tool | Permanent | Buy 25 / sell 10 | Baseline Woodcutting access | Remove from shop only if a safe replacement path exists |
| Iron axe | 2 | Forge: 2 Iron bars + 1 Wooden plank | Equipped tool | Permanent; Woodcutting 5 requirement | Not traded | Deterministic mid-tier Woodcutting upgrade | Measure upgrade time |
| Steel axe | 3 | Forge: 2 Steel bars + 1 Wooden plank; Cinder Warden rare drop | Equipped tool | Permanent; Woodcutting 10 requirement | Not traded | Deterministic route plus an exciting accelerating drop | Compare crafted and combat acquisition times |
| Bronze pickaxe | 1 | Development starting inventory; shop | Equipped tool | Permanent | Buy 25 / sell 10 | Baseline Mining access | Keep as recovery purchase |
| Iron pickaxe | 2 | Forge: 3 Iron bars + 2 Wooden planks | Equipped tool | Permanent; Mining 5 requirement | Not traded | Earned mid-tier Mining upgrade | Measure time-to-upgrade in a fresh playthrough |
| Steel pickaxe | 3 | Forge: 3 Steel bars + 2 Wooden planks | Equipped tool | Permanent; Mining 10 requirement | Not traded | Earned high-tier Mining upgrade | Measure time-to-upgrade in a fresh playthrough |
| Bronze sword | 1 | Forge: 3 Bronze bars + 1 Wooden plank; shop | Equipped weapon; Gathering Basics quest requirement | Permanent | Buy 40 / sell 15 | First visible combat upgrade with coin and crafting routes | Compare the two routes in playtesting |
| Developer Godsword | Debug | Development/debug grant only | Equipped weapon | Permanent | Not traded | Test-only power item | Never place in the live economy |
| Wooden shield | 1 | 6 Wooden planks at Crafting Table; shop | Equipped shield | 1.5 Logs after plank conversion | Buy 30 / sell 12 | Healthy early cross-skill craft with a shop fallback | Preserve; use as the reference for future crafted gear |
| Wooden planks | 1 | 1 Log -> 4 at Crafting Table | Wooden shield, 6 | 1 tick; stackable | Not traded | Compresses inventory and feeds construction-style recipes | Add tiered timber variants only when they have at least two sinks |

## Confirmed imbalances

1. **The shop is mostly a recovery shop, not a repeat-visit economy.** Cooked meat is its only recurring purchase.
2. **Combat and crafting need more ongoing interdependence.** Ore and bar drops now accelerate equipment, but permanent gear is only a one-time sink.

## Implementation order

1. Add deterministic Bronze, Iron, and Steel equipment recipes using existing materials and stations.
2. Remove advanced pickaxes from normal player starting inventory once their recipes are usable.
3. Make the recipe interface show requirements, output, duration, and whether the player can craft it.
4. Give Oak and Willow at least one meaningful sink each; avoid adding more raw materials until then.
5. Add telemetry-backed balance tests for upgrade time, food use, currency gain, and station travel.
6. Revisit shop stock and prices only after crafted upgrade costs are playable end to end.

## Anti-arbitrage rule

For every shop-traded crafted item:

`output sell value < total ingredient buy value`

Travel time and action time may justify a better return than selling raw gathered materials, but buying ingredients, crafting them, and immediately selling the output must never generate coins.

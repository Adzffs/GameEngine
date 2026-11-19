# Starter upgrade journey

**Date:** 19 November 2026\
**Base:** master `e6a77dbc235df91d74190a6dc6f21487022f5ca3` (PR #30).\
**Work package:** GAME-004.\
**Status:** Tutorial and guide implemented; production route and builds passed. Human pacing, visible interaction and regression suites remain deferred.

## Player-facing change

The tutorial previously advanced after one Copper/Tin pair and one Bronze bar,
although the sword needs three bars. The equipment step also accepted a
gathering tool in the weapon slot. These conditions could send a fresh player
to the next activity without the equipment inputs the next instruction needed.

The tutorial now derives material requirements from the starter recipes. The
ore goal prepares enough for the full sword, the bar goal requires all three
bars, and the equipment step requires the Bronze sword. Meeting the guide
requires Mara's dialogue session. A different villager's dialogue no longer
satisfies that introduction.

The next-step card displays the current objective, material progress, a useful
instruction and a destination from the loaded world's live resources, stations
and actors. It points out an unequipped axe/pickaxe before gathering. Press **H**
or left-click the card to open the material plan and quest alternatives. Card
clicks do not issue movement or resource commands; existing shop, dialogue,
station and context-menu input precedence is retained.

Destination coordinates and compass directions describe location, not a
computed walking route. Active resources take priority over depleted resources.
Missing targets produce an unavailable hint. The guide does not assume the
compiled development map's positions or the River Crossing candidate's positions.

## Materials and choices shown before travel

| Goal | Raw materials | Processing / use |
|---|---|---|
| Bronze sword | 1 normal Log, 3 Copper, 3 Tin | Make planks, smelt 3 bars, forge with 1 plank |
| Sword plus optional Wooden shield | 2 normal Logs, 3 Copper, 3 Tin | 8 planks; sword uses 1 and shield uses 6; 1 remains |
| Gathering Basics reward | 10 normal Logs gathered after accepting | 10 coins; processing the Logs does not erase quest progress |
| Mining Basics reward | 5 Copper gathered after accepting | 10 coins; smelting ore does not erase quest progress |
| Buy a shield | Both basic quest coin rewards | 20 coins at Mara; choose this or keep coins for supplies |

The shield is optional in the tutorial. Its material budget does not turn the
first Log step into a requirement to gather two Logs. Quests retain their own
acceptance and progress rules; the tutorial does not auto-accept quests or grant
retroactive progress for items already owned.

Prepared bars count toward the ore budget, and an existing sword satisfies the
earlier material milestones. Inventory, bank, equipment and actor-matched
crafting cursor/grid holdings are recognised. Banked ingredients still require
withdrawal to craft, and the guide points out that step. This avoids directing a
player to gather replacements merely because they banked or processed materials.

Tutorial step IDs, the ten-step sequence, skip behaviour and save/world formats
are unchanged. Existing mid-tutorial saves keep their step and use the corrected
readiness conditions. Prices, recipe yields, quest rewards and gathering rates
are unchanged in this slice.

## Reference production route

The optional `EmbervaleEconomyReport --replay-starter-upgrade` mode creates a
fresh in-memory character with no seeded coins, materials or XP. It accepts both
basic quests, equips the normal tools, walks to resources and stations, gathers
through production success/failure rolls, makes/equips a shield and sword, banks
spare Logs, collects quest rewards and buys two Cooked meat.

The captured River Crossing route starts from the actual selected spawn. Seeds
are combat `1337`, rewards `7331`, gathering `9001`. It gathers **10 normal Logs,
5 Copper and 3 Tin** in **31 completed gathering attempts**, including failures.
It ends at the `SAFE_COMBAT` tutorial step, with both upgrades equipped,
**8 Logs banked, 2 Cooked meat and 4 coins**. Twenty quest coins fund sixteen
coins of food. No enemy fight, damage or food consumption is included.

| Phase | Ticks in phase | Cumulative ticks |
|---|---:|---:|
| Approach Mara and accept quests | 11 | 11 |
| Equip axe and gather Logs | 164 | 175 |
| Reach table, craft planks/shield and equip shield | 70 | 245 |
| Equip pickaxe, reach quarry and gather Copper | 73 | 318 |
| Gather Tin | 24 | 342 |
| Reach furnace and smelt bars | 28 | 370 |
| Forge and equip sword | 6 | 376 |
| Reach bank and deposit spare Logs | 50 | 426 |
| Return, claim rewards and buy food | 25 | 451 |

At the current 600 ms clock interval, the shield checkpoint corresponds to
**2 min 27 sec of nominal simulation time**, the sword to **3 min 45.6 sec**,
and the prepared return to **4 min 30.6 sec**. These are tick-derived values for
one scripted route. The replay advances ticks directly and excludes reading,
discovery, cursor handling, human decision time and combat. They are neither a
stopwatch playtest nor a universal fastest route. The 10-15 minute first-upgrade
design target remains unverified; no artificial delays were added to match it.

[Machine-readable route, phase objectives and balances](economy/2026-11-19/starter-upgrade.json).
The earlier [economy correction](STARTER_ECONOMY_2026-11-19.md) remains separately
recorded.

## Run the reference route

From the repository root with the documented UCRT64 toolchain and configured
content-tools build:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build build-client --target GameEngine EmbervaleEconomyReport --parallel 6
.\build-client\EmbervaleEconomyReport.exe --replay-starter-upgrade --world content\worlds\river-crossing.blockout.json
```

Without `--world`, the mode uses the compiled development layout. It uses the
supported inventory-backed craft command and normal furnace recipe commands;
it does not automate the SDL crafting cursor. No player files are opened or
written. Rejected commands, missing required services, dead actors or bounded
tick exhaustion fail the report. The no-argument catalogue report now shares
the same material-plan model as the tutorial and guide.

## Execution and remaining acceptance

- Game/core/report and dedicated River Crossing launcher builds passed.
- Production gathering, crafting, equipment, bank, quest reward and food purchase
  completed on the River Crossing snapshot, including current guidance at phase
  checkpoints.
- Existing tutorial regression source now covers insufficient ore, insufficient
  bars and a gathering tool in the weapon slot. Regression suites were not run,
  at the owner's request.
- Visible guide layout, click/hover routing, bank/cursor edge cases, historical
  save acceptance and the complete combat/recovery journey remain unverified.
- GAME-004 remains `in_progress` until uncoached progression, actual travel and
  inventory choices, combat food pressure and elapsed upgrade timing are assessed.

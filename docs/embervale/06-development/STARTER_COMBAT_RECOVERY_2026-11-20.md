# Starter combat, food and recovery verification

**Date:** 20 November 2026.  
**Published code:** [0a86628](https://github.com/Adzffs/GameEngine/commit/0a866280849aaca3c907cfcd77ba90883636d8a2).  
**Baseline:** [f879b88](https://github.com/Adzffs/GameEngine/commit/f879b88aa39f90670f64fdb9bc70c29471cf94e8).  
**Tasks:** GAME-004 / GAME-005; supporting GAME-001 / GAME-003 / GAME-006.  
**Outcome:** The automated starter route now covers combat, food, loot banking, return to Mara, deliberate death, respawn and recovery across save/reload. A reproduced tutorial progression bug and two misleading hints were corrected.

This is deterministic software verification on the current published source. It is not an uncoached human or visible Windows playthrough, an animation approval or completion of the whole starter story chain. The original Windows melee/polish work remains unavailable under REL-004.

## Reproduced defect and correction

A fresh player legitimately banks spare Logs and claims Gathering Basics before combat, funding food. The tutorial previously treated any bank contents as completion of its later deposit lesson, then treated the already-claimed quest as completion of returning to Mara. It advanced from the first kill to COMPLETE while the character was still away.

The diagnostic replay observed both later checkpoints being bypassed. With the final replay harness and the original World implementation substituted in an isolated copied core archive, loading the awaiting-bank checkpoint fails with: loaded tutorial checkpoint advanced without its required action. This focused probe uses current compatible headers and other core objects; it is not described as a full baseline build.

The correction requires:

- An accepted deposit or deposit-all that actually changes that actor's bank revision during the current world tick. Historic contents, opening a bank, withdrawals, invalid/empty actions, another player's deposit and a previous tick cannot complete the lesson.
- A live Mara dialogue together with the completed Gathering Basics reward to complete the return lesson. Claiming that reward early remains valid.
- The same pending requirements after loading the existing version-13 player format. Six round-trip checks include idle updates of restored awaiting-bank and awaiting-Mara checkpoints.

The bank deposit marker is transient and cleared before each command batch. There is no save schema change or reset of already-completed tutorial profiles.

Guidance now says that monster rewards are added directly to the bag. Ground pickup is exercised through an explicit player drop of an awarded item type. The return hint also recognizes a previously claimed Gathering Basics reward and asks the player to talk to Mara rather than claim it again.

Sources: [tutorial transitions and command processing](../../../src/World/World.cpp), [guidance](../../../src/Tutorial/StarterJourney.cpp), [18-assertion checkpoint regression](../../../tests/StarterTutorialCheckpointTests.cpp).

## What the replay does

The new explicit-world mode reuses the existing unfunded preparation route. It gathers its materials, crafts/equips the shield and sword, banks eight Logs, earns twenty quest coins and buys two meat for sixteen coins. It then:

1. Saves/reloads the prepared character and verifies canonical bytes, owned quantities and unchanged live state.
2. Exercises actual food healing and rejects full-health food use without consuming an item.
3. Approaches and defeats the authored passive Mossling through normal combat commands, using food when appropriate.
4. Verifies the committed inventory loot, drops/reclaims an owned item of an awarded type, banks the complete earned loot and physically returns to Mara.
5. Checks save/reload while waiting for the bank and Mara checkpoints, then after completing the tutorial.
6. Deliberately enters Ashfang's territory and stands without attacking or continuing to eat until normal enemy damage causes death.
7. Verifies that saving the dead character rejects while preserving the last living checkpoint, then waits for the scheduled safe-point respawn.
8. Saves/reloads the living player with the protected pile still pending, walks back in that loaded session, reclaims it once and returns safely to Mara.
9. Checks another save/reload after recovery and verifies bank, equipment, tutorial progress and owned quantities.

Gameplay inputs use authoritative commands. The replay does not grant damage, stats, items, coins or XP to arrange outcomes. Save/load uses the normal World persistence APIs. Tick bounds stop stalled scenarios.

All generated saves live in a newly owned private temporary directory and are removed afterward. The command cannot select an existing player save. Existing-player-file guards, cleanup, paths containing spaces and invalid arguments are covered by actual CLI tests.

## Measured result on River Crossing

These are cumulative simulation ticks on the two primary play sessions. The continued counter sums them across a world restart. Auxiliary save-check worlds are outside that route counter.

| Milestone | Cumulative ticks | Observed result |
|---|---:|---|
| Equipment and food ready | 454 | Same output as the previous upgrade-only replay |
| Food checks complete | 456 | Current health 105/105 |
| Approach and defeat Mossling | 677 | 20 damage applied, 7 damage taken; minimum sampled health 100/105 |
| Drop/reclaim owned loot type | 679 | Item totals conserved; tutorial still waits for banking |
| Bank earned combat loot | 722 | Two earned items deposited; tutorial still waits for Mara |
| Return to Mara | 741 | Normal tutorial completes here |
| Deliberate death exercise | 2,211 | Actual Ashfang kill creates protected items and recovery metadata |
| Safe-point respawn | 2,215 | Exact four-tick delay; full equipped health restored |
| Reclaim persisted pile | 2,293 | Loaded owner receives the items once |
| Return safely after recovery | 2,370 | Progress and ownership survive the final round trip |

The normal tutorial segment ends at 741 ticks, nominally 444.6 seconds at 600 ms per tick. The longer 2,370-tick run includes an intentional death exercise and recovery; it is not the normal completion target. Reading, discovery, mouse handling and real elapsed time remain unmeasured.

Two meat are consumed and restore ten health: five fill the equipped-health gap (100 current versus 105 maximum before combat), and five heal actual combat injury. The first Mossling causes seven total damage. This is a concrete balance observation for GAME-004, not evidence that challenge, food pressure or repetition feels right to a player.

The protected pile contains two Copper ore, four Coins, one Bronze axe, one Bronze pickaxe and one Wooden plank: nine units across five types. All are conserved through death, respawn, the loaded pile, one claim and subsequent updates. The sword/shield and bank contents also remain intact.

See the [complete final replay](starter-journey/2026-11-20/journey.json), [early exploratory baseline](starter-journey/2026-11-20/exploratory-before.json) and [focused original-World probe](starter-journey/2026-11-20/unfixed-world-probe.json). The exploratory file used an earlier local replay with four save checks; the final harness has six and adds explicit loaded-checkpoint idle checks.

## Persistence acceptance exercised

| Checkpoint | Verified scope |
|---|---|
| Prepared | Position, health, skills, inventory/equipment, bank, quest/tutorial and recipe state survive canonical save/reload |
| Awaiting bank | Existing bank contents do not bypass the loaded pending lesson |
| Awaiting Mara | A previously completed quest does not bypass the loaded physical return |
| Post-combat | Earned XP/loot, bank transfer and completed tutorial persist |
| Pending recovery | The living respawned actor's pile, remaining lifetime and owner mapping survive loading |
| Recovered | The claimed pile is gone; recovered ownership and progression persist without duplicate items |

Each checkpoint compares the first save with a canonical re-save of the loaded character, then re-saves the original live character to verify that saving did not mutate ownership. Dead saves intentionally remain rejected; that rejection is checked against byte-identical previous checkpoint content.

The loaded primary session starts a fresh world and restarts its AI/combat/reward streams. Player saves do not persist the entire simulation, enemy state or random stream position. The report records this boundary rather than treating the route as one uninterrupted world.

## Executed verification

| Check | Result |
|---|---:|
| Linux client/SDL configuration | 109/109 CTest entries: 91 headless, 7 client, 11 graphics |
| Separate client/graphics-disabled build | 91/91 CTest entries |
| Python editor units | 42/42 tests |
| Actual editor/CLI bridge | 12 cases, included in one CTest entry above |
| Tutorial checkpoint regression | 18 assertions pass |
| Server command regression | 51 assertions pass |
| New actual replay CLI suite | 5 tests pass, included in one CTest entry above |
| Repeatability | Final JSON is byte-identical across repeated processes and client/headless tool builds |
| Upgrade-only compatibility | JSON remains byte-identical to the previous 454-tick result |
| Unfixed World probe | Expected checkpoint failure reproduced with the final harness |
| Documentation consistency | 97 Markdown files, 1,190 links and 96 synchronized task records; no dependency cycles |

Environment: Linux, GCC 14.2.0, CMake 4.4.5, Ninja 1.13.2 jobserver build, SDL 3.4.12 and Python 3.12.14. Client tests use dummy audio/video and software rendering. No requested automated check was skipped.

The [results file](starter-journey/2026-11-20/results.json) records exact tested/published source blobs and scope limits.

## Run it again

Build with content tools enabled, then select the authored candidate:

~~~bash
cmake --build build-headless --target EmbervaleEconomyReport --parallel 4
./build-headless/EmbervaleEconomyReport --replay-starter-journey \
  --world content/worlds/river-crossing.blockout.json

ctest --test-dir build-headless --output-on-failure \
  -R 'StarterJourneyReplayCliTests|StarterTutorialCheckpointTests|ServerCommandQueueTests'
~~~

On Windows, use the same arguments with the compiled executable's .exe path. The new full-journey mode requires an explicit world. Existing catalogue, timber and upgrade-only modes retain their earlier scope. Only the new journey mode creates and checks disposable saves.

Implementation: [shared bounded command driver](../../../src/Tools/AuthoritativeReplay.h), [continuation/report](../../../src/Tools/StarterJourneyReplay.cpp), [CLI entry point](../../../src/Tools/EconomyReportMain.cpp), [five CLI cases](../../../tests/StarterJourneyReplayCliTests.py).

## Remaining acceptance

GAME-005 is in progress. An uncoached human/visible Windows run must still record actual elapsed time, navigation/confusion, combat and food feel, recovery clarity, input behavior and native save-failure choices. GAME-004 retains tuning decisions; GAME-006 and the complete story chain are not finished by this tutorial replay.

REL-004 still needs the original Windows melee/polish sources and assets. ART-005 remains owner-deferred. Audible/device review of the first sound pass remains under ART-004. Preserve these distinctions when choosing the next task.

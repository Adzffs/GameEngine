# Starter quest and dialogue continuity

**Date:** 19 November 2026\
**Base:** master `b355bc912be781673cea5ee8ab233750497001ab` (PR #31).\
**Work packages:** GAME-007 and the narrative part of DOC-002.\
**Status:** Implemented; builds and content authoring validation passed. Visible interaction, regression suites and the full quest-chain playthrough remain deferred.

## Canonical names and actual quest rules

The elite encounter is **Cinder Warden**, matching the existing NPC definition
and Mara's stated concern. Heart of the Ash and Brann's dialogue now use that
name. The former alpha/trophy wording is retired from current gameplay copy.
No new monster or trophy item is introduced.

Quest completion rewards recorded progress. Shield or Shelter does not take a
shield from inventory, and Heart of the Ash does not require an inventory trophy.
The updated conversations describe reporting work and choosing a reward contact.
Both contacts complete the same quest and unlock the same successor. The save
records completion, without a separate record of the chosen reward branch.

| Quest | Accept / claim | Objective after accepting | Prerequisite | Quest reward options |
|---|---|---|---|---|
| Gathering Basics | Mara Vale | Gather 10 normal Logs | None | 10 Coins |
| Mining Basics | Mara Vale | Gather 5 Copper ore | None | 10 Coins |
| Tracks in the Ash | Mara Vale | Speak to Sera Flint, then report to Mara | None | 15 Coins |
| Shield or Shelter | Accept with Torren Ash; claim with Torren or Nessa Thorne | Craft 1 Wooden shield | Tracks in the Ash | Torren: 25 Coins **or** Nessa: 3 Cooked meat |
| Fangs at the Gate | Sera Flint | Defeat 3 Ashfangs, then report to Sera | Shield or Shelter | 5 Cooked meat |
| Heart of the Ash | Accept with Mara; claim with Mara or Brann Deepwell | Defeat 1 Cinder Warden | Fangs at the Gate | Mara: 75 Coins **or** Brann: 2 Steel bars |

Gathering, crafting, conversation and kill progress are event based. Owning an
item or having completed an action before acceptance does not add quest progress.
Ashfang defeats across respawns can satisfy the count; the dialogue no longer
promises three simultaneously roaming enemies. Quest rewards are separate from
ordinary monster drops. The six quest IDs, quantities, prerequisites, rewards,
session validation and completion mechanics are retained.

## Conversations and directions

Mara explains preparation, acceptance timing and the route from Tracks in the
Ash to Torren and Sera. Torren and Nessa explain their reward alternatives while
making clear that the player keeps the crafted shield. Elia directs players to
the bank marker and explains withdrawal before crafting. Nessa distinguishes
normal Logs from later timber tiers. Brann names the Cinder Warden and his Steel
bar alternative; Sera explains the report and the three-kill task.

Directions use named people and services rather than fixed compass directions
for the old development layout. Live objective hints resolve the actual loaded
NPC/monster positions and show both contacts for a ready reward choice. An
available enemy takes precedence over a defeated one. A defeated target is
labelled as such, without promising that an edited map enables its respawn.
Coordinates and compass hints describe locations, not a computed route.

The six characters keep their existing roles and personal goals. Elia's missing
ledger remains future character intent; the current dialogue gives usable bank
instructions rather than offering an unimplemented investigation task.

## Quest guidance in the game

- After completing or skipping the tutorial, the next-step card becomes a
  **Next quest** card while eligible work remains.
- Ready rewards and accepted work take priority. Otherwise, an available story
  lead takes priority over optional unstarted basic tasks. Locked and completed
  quests are not offered again. The first unstarted story lead is Tracks in the
  Ash, accepted with Mara.
- **H** or a left-click on the card opens the current quest's objective, progress,
  contacts, reward alternatives and rules. The equipment guide also names the
  next suggested quest after the Bronze sword is equipped.
- Hover a row in the Quest tab to read its full details with wrapped text. This
  exposes information that the compact rows previously clipped.
- Accept buttons show available rewards. Complete buttons show the reward for
  the NPC currently being addressed, including alternate contacts.
- Choice quests use a neutral choice label in the compact journal. Completed
  details identify the quest as complete and show its possible rewards without
  asserting which branch the player previously chose.
- The Shield or Shelter hint prepares the full six-plank budget, counts carried
  and banked timber, and directs withdrawal or further gathering before travel
  to the table.

These hints are read-only recommendations. They do not accept quests, move the
player or choose a reward. Existing dialogue/shop/station/context-menu input
precedence is retained. Tutorial and player-save/world formats are unchanged.

## Evidence and reproduction

The no-argument economy report now validates the full compiled content catalogue
and includes all six quest guides generated by the same presentation model used
in the client. Invalid content, shop validation or cost convergence produces a
nonzero exit. Existing price, recipe and replay fields remain available.

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build build-client --target GameEngine EmbervaleEconomyReport EmbervaleWorldValidate --parallel 6
.\build-client\EmbervaleEconomyReport.exe
.\build-client\EmbervaleWorldValidate.exe content\worlds\river-crossing.blockout.json
.\Play-RiverCrossing.cmd --build-only
```

[Compiled quest guides and current catalogue](narrative/2026-11-19/quest-guides.json)
and [authoring review](narrative/2026-11-19/authoring-review.json) record zero
content-validation errors, six quests, and matching Sera Flint / Ashfang / Cinder
Warden objective names. The longest authored dialogue is **154 characters**,
within the existing 184-character two-line budget. The selected River Crossing
snapshot validates with 31 placements and spawn `(5,28)`.

Core, SDL game, report, world validator and dedicated launcher builds passed.
Existing dialogue/journal expectations are updated, and deferred regression
source covers reward-choice visibility and recommendation priority. Test suites
were not run at the owner's request. No new full quest-chain or visible UI
acceptance is claimed, and prior test totals are not attributed to this slice.

GAME-007 remains `in_progress` for full journey/visible acceptance. DOC-002 now
records the naming decision and reconciled current findings; the original
source-review pack and historical verification records remain separately dated.
The next implementation area is the small asset identity / sprite import / art
pilot, with broader release gates retained.

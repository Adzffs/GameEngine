# River Crossing blockout implementation

**Date:** 19 November 2026\
**Base:** master `cab46ac992f63a3e591db0d0f67b57bac97e5be0` (fresh-player profile, PR #27).\
**Scope:** GAME-002 and the initial placement slice of GAME-003.\
**Owner direction:** Continue implementation with tests deferred for now.

## Delivered candidate

[River Crossing](../../../content/worlds/river-crossing.blockout.json) is a separate
loadable/editable 100×100 world candidate with arrival at `(5,28)`. Its
[overview](../../../content/worlds/river-crossing.overview.svg) is generated from
the actual JSON, rather than the earlier planning diagram.

The candidate implements the five river contour bands, two three-tile-wide land
causeways, both service loops, the riverside shortcut, workshop frontage, quarry
branch and the short NPC/station access connectors in the geography plan.
Road painting preserves water; only the two explicit causeways replace it.
Seven walkable stone pads reserve the proposed buildings. A stone quarry yard,
passive encounter clearing and narrow optional southeast encounter trail give
the blockout distinct working areas without adding unsupported walls/elevation.

| Placement group | Delivered |
|---|---|
| Normal/oak/willow trees | 8 / 3 / 1 |
| Copper/tin/iron/coal rocks | 2 / 2 / 1 / 1 |
| Stations | Bank `(33,27)`, furnace `(68,50)`, crafting table `(73,50)` |
| Friendly NPCs | All six existing identities at their planned district anchors |
| Monsters | Mossling `(27,68)`, Ashfang `(84,82)`, Stoneback `(91,80)`, Cinder Warden `(93,92)` |

All 20 original placement IDs/bindings and lifecycle stage/order keys are
retained. Eleven additional resources use schema-2 allocation IDs 1–11 and
persist `next_placement_id: 12`. The candidate has 31 placement records. Existing
spawn type, wander, cap, aggression, combat, reward and respawn settings remain
those of the compiled definitions. Delayed actors still initialize through
first player registration, not by being duplicated in the editor.

The runtime still uses the bounded single-region contract and its existing
`embervale.development_starter` token. The candidate is selected by its filename;
no new region/save schema is invented. Launch with a fresh temporary character,
as documented in the [world pack](../../../content/worlds/README.md).

## Authoring and presentation

`EmbervaleWorldExport --river-crossing` constructs a private complete candidate,
then applies `StarterWorldImport::ValidateForRuntime` before emitting JSON. This
is production content generation, not a test-suite pass. Default export and the
original fixture/world remain unchanged. No new runtime parser dependency or
SDL dependency is introduced into the content exporter.

Imported maps hide the compiled area's five fixed pixel-position labels so the
old village/mine/workshop labels do not appear in unrelated locations. Compiled
worlds retain those labels. Resource/station/character hover names still come
from their existing definitions. Mara's gathering direction now refers to the
woodland/quarry paths without assuming that resources are next to her.

The standalone standard-library Python overview renderer draws every terrain
run and placement from the snapshot, with source-ID/coordinate hover titles.
The original proposal diagram remains a separately labelled planning reference.

## Execution evidence

| Work | Result |
|---|---|
| CMake and game/export/edit-tool build | Passed, including the final presentation/dialogue rebuild |
| `EmbervaleWorldExport --river-crossing` | Generated canonical runtime-validated JSON; exit 0 |
| Candidate | 20,983 bytes; 31 placements; 100×100; schema 2 |
| Candidate SHA-256 | `8a04f8fdc6b02f33f1cdcc5c0f71323deb2c9def39bfc3ca29fcd6ac951df0fb` |
| Underlying terrain | 7,762 grass; 985 dirt; 570 stone; 683 water tiles |
| Overview generation | Completed from the authored snapshot |
| Unit/integration/Python test suites | Not run, at the owner's request |
| Visible Studio/game, combat/AI and pacing acceptance | Deferred; not run |

Build command in the existing Windows UCRT64 configuration:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build build-client --target GameEngine EmbervaleWorldExport EmbervaleWorldMove --parallel 6
```

The authoring validator checks dimensions, identities, terrain, overlap,
walkability, actor reachability and cardinal interaction approaches for the
complete candidate. It does not establish actual quest completion, dynamic AI
behaviour, route quality, recovery or the intended first-upgrade time. No old
test totals are attributed to this revision and no release gate is closed.

## Remaining acceptance and next work

GAME-002/003 remain `in_progress` for deferred visible/dynamic acceptance. Run the
fresh tutorial, both crossings, every service/resource approach, combat and return
routes, then inspect respawn/recovery and save/reload using an isolated save.
Record route times and first-upgrade pacing before claiming a balanced starter
area. The existing quest wording mismatch between “Ashfang alpha” and Cinder
Warden remains part of DOC-002; spawn identity and quest targets were retained.

The next gameplay work is the starter resource/upgrade/economy loop (GAME-008 and
GAME-004). Art/building shells and structured player observations remain later
steps rather than prerequisites for loading this first flat candidate.

# Playable world candidates

## River Crossing

[World snapshot](river-crossing.blockout.json) · [Map overview](river-crossing.overview.svg) ·
[Geography and placement plan](../../docs/embervale/05-game/RIVER_CROSSING.md)

This first flat starter blockout has two land causeways, connected service loops,
walkable building pads, a woodland, workshop, quarry and optional encounter trail.
It contains 18 resources, three stations, six named NPCs and four monster homes.
Arrival is `(5,28)`. Mara is at `(14,25)`, the bank at `(33,27)`, and the workshop
stations at `(68,50)` and `(73,50)`. The existing compiled world remains available.

Launch a temporary fresh character from the repository root on Windows:

```powershell
.\build-client\GameEngine.exe --world content\worlds\river-crossing.blockout.json --profile fresh --no-save
```

Open the same file in Studio:

```powershell
python tools\embervale_world_editor.py content\worlds\river-crossing.blockout.json --tool build-client\EmbervaleWorldMove.exe
```

Studio defaults to **Fresh player** for **Play this draft**. Edits save to a new
sibling draft. Choose a separate map-specific save explicitly only when keeping
progress is wanted; `--profile fresh` never resets an existing selected save.

The file uses schema 2 and the current bounded `embervale.development_starter`
region identifier. River Crossing is the named candidate file, not a new region
transition system or player-save migration. Building pads are walkable ground,
and causeways are ordinary passable land. Art, walls, elevations and balance
acceptance remain later work.

## Authoring source

`EmbervaleWorldExport --river-crossing` builds the candidate and applies the
same full-map content validator used at game startup before writing canonical
JSON to stdout. Its source is
[RiverCrossingBlockout.cpp](../../src/Tools/RiverCrossingBlockout.cpp).
The default exporter still captures the original compiled development world.

To create another candidate without replacing the checked-in file, run in a
shell that preserves native UTF-8 output:

```sh
./build-client/EmbervaleWorldExport --river-crossing > river-crossing-draft.json
```

The overview is rendered directly from the checked-in terrain and placements:

```sh
python tools/render_world_overview.py content/worlds/river-crossing.blockout.json content/worlds/river-crossing.overview.svg --title "River Crossing"
```

The current canonical candidate SHA-256 is
`8a04f8fdc6b02f33f1cdcc5c0f71323deb2c9def39bfc3ca29fcd6ac951df0fb`.
Generation/content validation and builds passed. Test suites, visible gameplay,
AI behaviour and pacing acceptance remain deferred at the owner's request.
[Implementation record](../../docs/embervale/06-development/RIVER_CROSSING_BLOCKOUT_2026-11-19.md).

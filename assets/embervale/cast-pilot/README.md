# Player, Mara and furnace pilot

**Created:** 19 November 2026. **Stage:** integrated static art and player walking.

![Four player directions, Mara and furnace at 2x native size](previews/cast-pilot-2x.png)

![Four-pose player walk preview](previews/walk-cycle.gif)

The player has a chestnut-haired, blue-teal traveller outfit with empty hands.
Mara keeps her canonical blue warden coat and gold sash, with tied-back hair and
a grey streak. The furnace uses slate stone, a short chimney and a lit arch.
All share upper-left lighting, a restrained palette and clean alpha with the
[forest kit](../forest-pilot/README.md).

| Editable master | PNG | Contents |
|---|---|---|
| [player.ase](source/player.ase) | [player.png](textures/player.png) | 128x192 atlas; four directions and four poses per direction |
| [mara.ase](source/mara.ase) | [mara.png](textures/mara.png) | Static 32x48 sprite |
| [furnace.ase](source/furnace.ase) | [furnace.png](textures/furnace.png) | Static 48x64 sprite |

Masters use one image layer and one editor frame. The player's sixteen runtime
frames are cells in an editable atlas. Rows are **south, west, east, north**;
columns are **neutral, first stride, neutral, opposite stride**. Each cell is
32x48 with pivot **(16, 40)**. Mara uses the same pivot. The furnace pivot is
**(24, 56)**. Every pivot sits at **(16, 24)** in the existing 32-pixel world tile.
The image dimensions do not change gameplay occupancy.

The game chooses the player's facing from visual travel and advances the walk
cycle by distance, with one cycle per tile. It returns to a neutral pose on
stopping. The GIF is a four-frame art-board preview, with all directions shown
together; its fixed playback rate is not the game simulation clock.

## Edit and export

Save edits in the masters, then run from the repository root:

```powershell
python tools/art/build_cast_pilot.py --editor "C:\Program Files\Asprite\libresprite.exe"
```

Normal export preserves all three masters, refreshes PNGs/metadata/previews and
updates the shared C++ geometry binding. Both art batches use one texture cache.
CMake copies the six runtime PNGs beside the executable; restart after export
and build to see saved changes. F6 still switches to basic shapes.

`--regenerate` intentionally replaces the masters using the retained
[original generated reference](reference/cast-reference.png) and LibreSprite
authoring script. Preserve manual edits before using that option. LibreSprite
performs all visible artwork processing, palette cleanup, pose drawing and GIF
export; Python only supplies empty canvases and metadata/commands.

[manifest.json](manifest.json) records 18 stable frame IDs, checksums, pivots,
palette, facing order and provenance. No third-party image input was used and
no new asset licence is asserted beyond the repository distribution policy.

## In the game

See the [integration record](../../../docs/embervale/06-development/CAST_RENDERING_2026-11-19.md)
for actual River Crossing captures, build evidence and review limits. Mara's
painted head and the furnace's opaque pixels can be clicked as their existing
interaction targets. Collision, dialogue, recipes, equipment and saves retain
their current rules. The base player outfit does not add equipment overlays,
combat/death poses or new character customisation. Other NPCs and stations keep
their existing art until their own batch is authored.

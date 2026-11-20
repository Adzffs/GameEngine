# Equipped weapons and eight-way melee

This batch retains the current player outfit and adds equipped walking, punches
and melee swings. Its editable LibreSprite masters export through
`tools/art/build_melee_pilot.py`; the runtime uses only the two PNG atlases.

| Atlas | Size | Contents |
|---|---|---|
| `source/player.ase` | 1536 x 3840 | Unarmed, sword, two-handed, axe and pickaxe bodies; eight directions; eight standing/walk and eight attack poses |
| `source/equipment.ase` | 1536 x 6912 | Bronze sword, developer godsword, three axe tiers, three pickaxe tiers and wooden shield; matching directions/poses |

Every cell is 96 x 96 native pixels with pivot `(48,64)` and ground point
`(16,24)` in a 32-pixel world tile. The manifest records frame rectangles,
provenance and checksums. The current manifest
contains 640 body cells and 1,152 equipment cells. The shared `player_rig.js`
keeps both wrists within 9.5 native pixels of their shoulders. Equipment uses
those exact joints, and the final equipped recovery matches standing.

Default export preserves saved masters. Use `--regenerate` explicitly to replace
them from the current eight-way cast master and the authoring script:

```powershell
python tools/art/build_melee_pilot.py --editor "C:\Program Files\Asprite\libresprite.exe"
python tools/art/build_melee_pilot.py --editor "C:\Program Files\Asprite\libresprite.exe" --regenerate
```

Validation rejects incorrect dimensions, nonbinary alpha, incomplete upper/lower
bodies, empty cells and clipped artwork. Default export reproduces runtime PNGs,
metadata, the manifest, the renderer binding and previews:

- `previews/melee-cycle.gif`: eight attack poses across all eight directions,
  with authored delays and a shortened guard hold for the preview.
- `previews/melee-contact.png`: guard, coil, loaded, contact, overshoot,
  follow-through, return and settle columns; S/W/E/N/SW/SE/NW/NE rows.
- `previews/equipment.png`: supported equipment in manifest order.

Runtime layer ordering follows facing. The shield remains on the off hand with
one-handed weapons and is carried behind the body with the two-handed godsword.
Unarmed attacks use the punch body without an invented weapon. See the
[research, timing choices and verification](../../../docs/embervale/06-development/ANIMATION_RESEARCH_2026-11-20.md).

These are original Codex-assisted LibreSprite additions to the current cast
master. Its source hash is retained in the manifest. No separate asset licence
grant is declared; repository distribution policy applies.

# Bounded starter-world terrain paint — 18 November 2026

**Stack:** `studio/validated-terrain-paint` on `studio/visual-map-editor` (draft). **Planning:** STUDIO-020 / STUDIO-022.

## Player/editor capability

The first shared **single-tile base terrain** command is implemented. `PaintTerrain` accepts `G` grass, `D` dirt, `S` stone and `W` water; it validates source and resulting content through the same complete starter-world runtime validator used by import and stable-ID placement moves. Rejected edits cannot modify the caller. Grass/dirt/stone are walkable in the current game and water blocks travel; the validator rejects invalid occupancy, player spawn, unreachable actors, missing approaches and other runtime constraints.

The content tool now exposes a dry run:

```sh
EmbervaleWorldMove --paint source.json 30 30 D
```

For an explicitly selected fresh **sibling** draft, pass `--expected-hash <exact-source-SHA256> --write-draft <new-sibling.json>`. This routes through the existing byte-rechecking, create-only publisher; it cannot overwrite the source, an existing draft or a player save. Default mode never writes a file.

## Verification boundary

The branch adds direct C++ paint candidate regression assertions to `StarterWorldEditTests` and a new `StarterWorldPaintCliTests` scripted scenario covering dry run, no-op, bad symbols, blocked placements/spawn, out-of-bounds, valid guarded publication, existing target and stale hash. The smoke scripts and compilation must be run against the full stacked checkout before merge. The repository cannot be cloned from this execution host (GitHub DNS resolution); **the full build, registered C++ tests, real game and Windows playtest were not executed**. This is a reviewable draft, not a release verification claim.

## Next

Extend shared history and terminal editing with reversible terrain paint batches, return authoritative painted rows for the graphical viewport, then add the GUI brush. Keep current move/undo/save support, stable IDs, spawn lifecycle, and imported-player recovery unchanged.

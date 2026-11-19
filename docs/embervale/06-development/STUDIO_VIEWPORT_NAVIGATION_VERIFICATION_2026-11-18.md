# Studio viewport navigation verification — 18 November 2026

**Branch:** `studio/visual-map-editor`, stacked after the terminal edit session. **Scope:** Zoom, fit-to-view and accurate map-coordinate selection only; this does not enable terrain painting or publication.

## Implemented

The Python/Tk editor now exposes −, + and Fit controls, Ctrl+wheel and keyboard zoom, pointer tile coordinates, and a 4–64 px/tile bound. Pointer-anchored zoom preserves the world coordinate beneath the cursor. A fitted 100×100 world is rendered with suitably small resource/NPC markers and a white player-spawn square; the GUI still delegates moves, history and guarded draft saving to the C++ CLI.

## Evidence and limits

- **Executed locally:** 3/3 isolated Python 3 unit tests for `tile_at`, `scroll_fraction` and `fitted_tile` passed. These check tile edges, negative/outside coordinates, clamped scroll origins, narrow worlds and minimum/maximum zoom scales. The exact pure helper definitions and assertions were used for this isolated check.
- **Not executed:** a complete project checkout, the full Python bridge suite on the updated module, the real C++ content-tool build, a real fixture-based GUI run, Windows behaviour or an interactive game playtest. The execution host cannot resolve GitHub domains to clone the repository; GitHub source was inspected and updated through the connected repository API instead.

Keep the stacked PR in draft. Re-run `python -m unittest discover -s tests -p test_world_editor_bridge.py -v` from the repository root, then launch the real fixture with `EmbervaleWorldMove`, test zoom/selection at each scale, move/undo/redo/save a draft, reopen the draft and playtest it with `GameEngine --world <draft> --no-save` on a desktop build. No current game-save or world-import rules were modified by this navigation change.

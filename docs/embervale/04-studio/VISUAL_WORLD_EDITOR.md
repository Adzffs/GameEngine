# Visual starter-world editor shell (STUDIO-030 first slice)

**Status:** Experimental stacked development branch, not integrated into `master`.
**Scope:** Select and move existing resource/workstation placements in a real mouse-driven grid, with read-only inspection, undo/redo and create-only draft saving. This does not claim the full STUDIO-030 acceptance story.

## Run

Build the content tools, then from the repository root run:

```sh
python3 tools/embervale_world_editor.py tests/fixtures/content/development-starter.snapshot.json --tool build-headless/EmbervaleWorldMove
```

On Windows, use `python` and the path to the built `EmbervaleWorldMove.exe`.
Python 3 and its standard-library Tk desktop module are required; no Python packages are installed by the tool. The snapshot must be a valid existing development starter-world JSON file.

The viewport displays the complete terrain grid with scrollbars and placement markers. Use the **− / + / Fit** toolbar controls or **Ctrl+mouse wheel** to zoom (Ctrl+0 fits the map). The tile coordinate under the pointer is shown in the toolbar. Zoom preserves the authored tile under the mouse cursor, or the viewport centre for toolbar/keyboard zoom; scrollbars and tile selection continue to use world coordinates. Very small tiles display the player spawn as a white square rather than an unreadable @ glyph. Click a placement or select it from the sidebar to inspect its stable ID, definition and tile. Click an empty destination tile to propose a move. The shared C++ `MovePlacement`, runtime validator and `History` determine whether it succeeds; invalid destinations leave the source and current editor state unchanged. Use Ctrl+Z / Ctrl+Y or the buttons for undo / redo. **Save new draft** opens a filename chooser and publishes only a fresh sibling JSON file through the existing guarded C++ writer. Filenames containing spaces are supported by the terminal session. The original snapshot and player saves are never changed by the GUI.

## Authority, revision and boundaries

This is a deliberately thin standard-library Python/Tk shell. It reads the source JSON to draw *base terrain and metadata only*. It neither edits JSON nor determines authoritative validity. Every edit, undo, redo and save is passed as commands to `EmbervaleWorldMove --session`; the entire bounded command history is replayed in a short-lived process and the GUI re-reads both the C++ placement list and the **actual terrain rows at its history cursor** before repainting. Python never guesses inverse terrain from recorded commands. A rejected command does not get appended to the history. To avoid a fragile streaming-text subprocess protocol, the first shell uses a bounded history of 256 commands with a 20-second invocation timeout.

A new optional `--expected-hash <sha256>` flag on `EmbervaleWorldMove --session` compares the **exact source bytes** with the initial snapshot revision before *any* replayed command can execute. On a conflict the session returns exit code 4; the user must reopen the source rather than silently editing a different revision. The C++ draft writer independently rechecks the same exact source hash under its cooperative lock, and continues to reject existing files. This does not make the filesystem lock mandatory for outside writers or guarantee power-loss durability.

The UI now has a click-to-paint Grass/Dirt/Stone/Water brush. Accepted brush actions use the C++ session's paint command and share history with placement moves; rejected brush actions never modify the displayed map. The UI does not currently edit actor spawns, auto-save, preserve undo across process restarts, handle multiple regions or launch an isolated game playtest. It uses a simple logical grid, not game sprites. This initial desktop client is intended to exercise STUDIO-030's selection/inspection/move path and reuse of shared command authority; the broader acceptance criteria remain open.

## Navigation and coordinate safety

The map viewport uses the same pure pixel-to-tile, clamped-scroll and fitted-scale functions for mouse selection and zoom. Scale is bounded to 4–64 pixels per tile. Selecting a placement and moving it still use only stable IDs and the C++ move validator; zoom cannot alter the underlying content or command history. A 100×100 map can be fitted into a typical desktop viewport without changing its JSON dimensions. Zoom and navigation do not alter the brush or the validated terrain-editing operations.

## Verification and remaining gate

On 18 November 2026, source for this branch was developed and checked in a local isolated workspace:

- `python -m py_compile tools/embervale_world_editor.py` passed.
- `python -m unittest discover -s tests -p 'test_world_editor_bridge.py' -v` passed **6/6**. These are mocked-backend contract tests, not actual C++ or gameplay tests: they cover move/undo/redo replay, rejected operation rollback, source revision conflict, space-containing output paths, save protection and bounded history.
- A Tk window was launched under `xvfb-run` against the mock tool without startup errors, then terminated by a 4-second smoke-test timeout. This is not a human interaction or rendering-quality test.
- The **full repository build, real `EmbervaleWorldMove` integration, client playtest and Windows Tk experience were not executed** because repository cloning is not available in this execution environment (GitHub DNS resolution).

A subsequent navigation slice adds 3 pure viewport-math regression tests. The pixel/tile boundary, scroll clamping and fit-scale cases passed in an isolated Python 3 run; they are not a GUI interaction test. The complete updated bridge module and real game have not been run as a combined build in this environment. See [navigation evidence](../06-development/STUDIO_VIEWPORT_NAVIGATION_VERIFICATION_2026-11-18.md).

Before merging: compile the complete stacked branch, run `ctest --test-dir build-headless --output-on-failure -R StarterWorld`, run the six Python bridge checks from repository root, and manually open the real fixture with the built mover. Move a tree, undo/redo, reject a blocked tile, save a sibling path containing spaces, validate and playtest the exact saved content with an isolated player save. Keep draft status until all gates are satisfied.

See [the history design](BOUNDED_UNDO_REDO.md), [guarded drafts](GUARDED_DRAFT_REVISIONS.md) and [editor workflow acceptance](EDITOR_WORKFLOWS.md).

## Terrain brush stabilization (18 November 2026)

The first brush push contained a Python indentation defect in the initial terrain-history setup, which prevented importing the graphical editor module. This follow-up restores that initialization inside the snapshot parsing block. The mocked command-session fixture now models real terrain paints, undo, redo and branch truncation, with three focused bridge tests for accepted/no-op paints, interleaved moves and paints, and transactional rejection. The source snapshot remains read-only, and the C++ editing session remains authoritative.

**Evidence limitation:** The updates were reviewed against the checked-in Python and C++ session source, but the full Python test suite, actual desktop interactions and combined C++ executable were not executed from this environment because repository checkout is unavailable. These checks remain required before merging PR #16.


## Authoritative terrain viewport follow-up (18 November 2026)

The graphical editor no longer maintains a parallel Python terrain history. The C++ session now exposes a read-only `terrain` command, delimited by `Terrain rows (GDSW):` and `End terrain rows`. After every successful command, the Python bridge requests and validates the complete row grid, updates the viewport from those rows and rejects missing, truncated, invalid-glyph or source-disagreeing data. Undo/redo and a new branch after undo are displayed from the C++ history cursor, not a guessed inverse. The source file, stable IDs, player data and draft protections remain unchanged.

The new Python mock cases cover missing/malformed/mismatching terrain responses, and the C++ interactive-session script now requests and checks terrain output. This stack remains draft until combined C++ and Python tests and a real graphical playtest are completed. No complete checkout/build or actual game execution was available in this run.

# Visual starter-world editor shell (STUDIO-030 first slice)

## Inspection overlays

Toggle **Static collision** to mark water and resource/station tiles with red
outlines and diagonal strokes. This matches the starter importer static-blocking
rules; it does not predict moving actors or prove route reachability. Toggle
**Spawns** to mark the player, NPC and monster starting tiles in cyan, with text
labels at larger zoom levels. Both overlays use the current verified terrain
and placement response, so moves, paints and undo/redo update them on repaint.
They are read-only display options and do not enter history or saved content.

## Play this draft

Build the SDL client and launch Studio with `--tool build-client/EmbervaleWorldMove.exe`
on Windows (omit `.exe` elsewhere). **Play this draft** prompts to save unsaved
changes, validates a private temporary snapshot through the C++ importer, then
launches the adjacent `GameEngine` with `--world <private-copy> --no-save`.
Use `--game <executable>` when the game lives elsewhere. Closing or cancelling
the save dialog keeps editing without starting a game. The private snapshot
remains available until the game exits, then is removed. A running playtest
disables another launch and Studio asks you to close the game before exiting.
Player progress is temporary; the source world and player saves are untouched.

On 19 November 2026 the button was exercised visibly on Windows against the
real SDL client: it launched the saved world, showed the moved tree, disabled
duplicate launch, and returned to its ready state after normal game exit.
The seven real bridge regression scenarios and 36 Python contract/viewport
tests passed. The new scenario verifies C++ validation of the private copy,
exact launch arguments, dirty-state rejection, duplicate rejection, stale
saved-world rejection and temporary snapshot cleanup. The actual game launch
is mocked only in that regression; the visible button smoke used the real game.

## File state and reopening

The title shows an asterisk and the toolbar says **Unsaved changes** when the
current authoritative world differs from the last verified save. Undoing back
to that world clears the indicator. A failed or cancelled save does not clear
it. Closing or opening another draft offers Save / Discard / Cancel; cancelling
the save keeps the current world open. **Open draft…** validates a selected
world before replacing the editor session. **Reopen last saved** starts a fresh
session from the verified draft, with the same unsaved-change protection.
Draft saves remain create-only sibling files; reopening starts a new history.

Visible Windows acceptance on 19 November 2026 at `14a062a`: moving a tree
set the title asterisk and unsaved indicator; Cancel in the close warning
preserved the edit, and undo to the original world cleared the indicator.
Open draft loaded the earlier user-saved world with its tree at (22,11).
The native Save dialog published `file state acceptance.json`, and Reopen
last saved loaded that file with the placement preserved and zero history.
After another move, choosing Yes in the close warning opened the Save dialog;
cancelling that dialog kept the editor open, dirty, with the moved tree intact.
The 36 Python contract/viewport tests and six real bridge cases also passed.

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

## Placement parity safeguard (18 November 2026)

The graphical bridge treats the C++ editor's placement list and terrain rows as authoritative, but the initial source file must agree with the backend *before editing starts*. The source's stable IDs, kinds and coordinates are now matched exactly against the C++ startup listing. Subsequent edits must preserve each ID and kind, keep every placement in bounds, and leave read-only NPC/monster spawn coordinates unchanged. Backend output that violates these invariants is refused before it changes the visible map or the Python replay list. This is a fail-closed view consistency check, not an alternative content validator. Resource/station moves and painting are still validated by the C++ runtime rules.

Five mocked protocol regressions were added for mismatched startup placement coordinates/kinds and unexpected spawn/out-of-bounds positions after a command. Small isolated Python checks passed, but the complete committed bridge tests and the real C++/Tk integration are not yet executed in this environment. See [the dated evidence record](../06-development/STUDIO_AUTHORITATIVE_TERRAIN_VIEW_2026-11-18.md).

## Real-executable / Python bridge integration gate (18 November 2026)

When Python 3 is present at CMake configuration, the optional local test
`StarterWorldVisualBridgeIntegration` runs the actual built
`EmbervaleWorldMove` through `StudioBridge`, rather than the fake command
backend used by the existing Python contract tests. Each case copies the
real 100×100 starter fixture into a private temporary directory.

The test checks initial stable-ID and terrain parity, tree movement,
terrain painting and mixed undo/redo, rejected water at spawn/occupied
objects, fresh sibling draft saving with spaces in the filename, runtime
hash validation and reopening of that draft, existing-file protection
and stale exact-byte source rejection. It does not touch the canonical
fixture, user player saves or any live game world.

After building locally, execute:

```sh
ctest --test-dir build-headless --output-on-failure -R StarterWorldVisualBridgeIntegration
```

Missing Python means the optional test is *not registered*, not that it
passed. This is not a GUI interaction test or an actual game playtest.
The test source passed local Python syntax checks, but the combined
executable test remains unrun until a full repository checkout is
available. See the dated evidence record in 06-development.

## Response-state contract hardening (18 November 2026)

Successful move/paint responses must agree with the requested stable ID,
coordinate and ground glyph, without changing unrelated placements or terrain.
Undo/redo must advance or rewind the backend history cursor by exactly one
entry and must change world state. Unexpected backend replies are rejected
before the visual editor commits the new view or command. This is a protocol
consistency check on top of C++ runtime validation, not a Python validator.
Four new mocked backend regressions are checked in, with an isolated extracted
checker test passed; full integrated Python/C++/GUI verification is still open.

## Protected draft receipt verification — 18 November 2026

After the C++ editing session reports a successful create-only sibling save, the
Python bridge now requires exactly one acknowledgement for the **requested**
absolute target path and exactly one valid `draft-sha256` receipt. It checks
that the target is a regular file, is not a symlink, remains within the 2 MiB
editor input limit, and that its actual bytes match the C++ SHA-256 receipt.
A missing, malformed, unexpected or mismatched saved artifact is reported as
a failed save, not displayed as success. The bridge does **not** delete or
rewrite a suspect draft; inspect that file before retrying with a fresh name.

Mock-backed regressions cover mismatched hashes, wrong acknowledgement paths,
and files the backend claims to save but does not create. No player or live
world persistence path is changed. The complete checked-in Python tests,
real C++ executable integration and desktop GUI validation still need to run
before merging this stacked feature.


## Saved draft semantic readback — 18 November 2026

A matching receipt hash proves which bytes were written, **not** that the bytes
represent the world currently visible in the editor. On a successful guarded
save, StudioBridge now compares the **same bytes** used for receipt verification
against the original snapshot plus the current authoritative C++ terrain rows
and stable-ID placement coordinates. It preserves the original player spawn,
definition bindings, initialization/spawn properties, region and other metadata.
Canonical placement ordering is ignored; unexpected metadata or map changes
cause a failed save. Suspicious drafts are left on disk for inspection rather
than silently deleted.

Four new fake-backend regressions cover correct move+paint readback and
self-consistently hashed drafts containing wrong terrain, placement or player
spawn. The targeted eight-case standalone pure-helper exercise passed under
local Python, but the checked-in regressions and real-executable/GUI paths
remain unrun pending a complete checkout. See the dated integration evidence.


## Replay-command boundary (18 November 2026)

The Python Studio bridge has two deliberately separate backend entry points:
`perform` accepts only canonical resource/station `move`, terrain `paint`,
`undo` and `redo` commands, while `save` alone is allowed to issue a
`save <new-sibling.json>` command. This prevents untracked terminal commands
from being stored in the replay history or bypassing draft receipt/readback
verification. Raw status, list, map, help and quit commands cannot be used as
history edits. Two new regressions exercise the boundary with fake and real
backends; only the isolated four-method guard check has actually run locally.
The complete Studio integration gate remains open.

## Complete Windows integration follow-up (19 November 2026)

The stacked branch now builds cleanly on Windows and the complete registered
headless/content suite passes **84/84**, including the real executable-backed
bridge gate. That gate passes **6/6** cases against `EmbervaleWorldMove.exe` and
the real starter fixture; the mocked bridge/viewport suite separately passes
**35/35**. The run corrected Windows create-only draft publication, case-only
temporary-path receipt comparison and portable Python mock-tool launching.
Player-save tests passed in the full set, and the bridge continued to operate
only on temporary fixture copies. A visible Tk interaction and SDL client
playtest remain outside this verification. See the [Windows integration
record](../06-development/STUDIO_WINDOWS_INTEGRATION_VERIFICATION_2026-11-19.md).

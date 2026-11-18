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

The viewport displays the complete terrain grid with scrollbars and placement markers. Click a placement or select it from the sidebar to inspect its stable ID, definition and tile. Click an empty destination tile to propose a move. The shared C++ `MovePlacement`, runtime validator and `History` determine whether it succeeds; invalid destinations leave the source and current editor state unchanged. Use Ctrl+Z / Ctrl+Y or the buttons for undo / redo. **Save new draft** opens a filename chooser and publishes only a fresh sibling JSON file through the existing guarded C++ writer. Filenames containing spaces are supported by the terminal session. The original snapshot and player saves are never changed by the GUI.

## Authority, revision and boundaries

This is a deliberately thin standard-library Python/Tk shell. It reads the source JSON to draw *base terrain and metadata only*. It neither edits JSON nor determines authoritative validity. Every edit, undo, redo and save is passed as commands to `EmbervaleWorldMove --session`; the entire bounded command history is replayed in a short-lived process and the GUI re-reads the C++ placement list before repainting. A rejected command does not get appended to the history. To avoid a fragile streaming-text subprocess protocol, the first shell uses a bounded history of 256 commands with a 20-second invocation timeout.

A new optional `--expected-hash <sha256>` flag on `EmbervaleWorldMove --session` compares the **exact source bytes** with the initial snapshot revision before *any* replayed command can execute. On a conflict the session returns exit code 4; the user must reopen the source rather than silently editing a different revision. The C++ draft writer independently rechecks the same exact source hash under its cooperative lock, and continues to reject existing files. This does not make the filesystem lock mandatory for outside writers or guarantee power-loss durability.

The UI does not currently paint terrain, edit actor spawns, auto-save, preserve undo across process restarts, handle multiple regions or launch an isolated game playtest. It uses a simple logical grid, not game sprites. This initial desktop client is intended to exercise STUDIO-030's selection/inspection/move path and reuse of shared command authority; the broader acceptance criteria remain open.

## Verification and remaining gate

On 18 November 2026, source for this branch was developed and checked in a local isolated workspace:

- `python -m py_compile tools/embervale_world_editor.py` passed.
- `python -m unittest discover -s tests -p 'test_world_editor_bridge.py' -v` passed **6/6**. These are mocked-backend contract tests, not actual C++ or gameplay tests: they cover move/undo/redo replay, rejected operation rollback, source revision conflict, space-containing output paths, save protection and bounded history.
- A Tk window was launched under `xvfb-run` against the mock tool without startup errors, then terminated by a 4-second smoke-test timeout. This is not a human interaction or rendering-quality test.
- The **full repository build, real `EmbervaleWorldMove` integration, client playtest and Windows Tk experience were not executed** because repository cloning is not available in this execution environment (GitHub DNS resolution).

Before merging: compile the complete stacked branch, run `ctest --test-dir build-headless --output-on-failure -R StarterWorld`, run the six Python bridge checks from repository root, and manually open the real fixture with the built mover. Move a tree, undo/redo, reject a blocked tile, save a sibling path containing spaces, validate and playtest the exact saved content with an isolated player save. Keep draft status until all gates are satisfied.

See [the history design](BOUNDED_UNDO_REDO.md), [guarded drafts](GUARDED_DRAFT_REVISIONS.md) and [editor workflow acceptance](EDITOR_WORKFLOWS.md).

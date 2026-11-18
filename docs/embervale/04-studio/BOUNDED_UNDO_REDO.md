# Bounded placement history (STUDIO-023)

**Status:** In-progress, in-memory editing primitive, stacked after the guarded draft experiment. Not a complete graphical editor or persistent journal.

`StarterWorldHistory::History` accepts one validated starter-world snapshot. Its `ApplyMoves(current, commands)` applies between 1 and 32 existing **resource or workstation** moves to an owned candidate using the shared `StarterWorldEdit::MovePlacement` and runtime validator. All intermediate states must be gameplay-valid. If any operation fails, the entire batch fails without changing the caller content, cursor or redo branch. A successful changed batch forms one undoable history entry.

`Undo(current)` replays inverse moves in reverse order. `Redo(current)` replays the original moves in forward order. Both validate their input and their complete output, and check that the input matches the **expected complete canonical semantic revision**. If a different tool has changed terrain, placements, or other relevant content since the last history operation, the command throws `CONTENT_CONFLICT`. It does not overwrite an unrelated change by reinstating an old world snapshot. Histories are limited to 64 applied entries; an all-no-op batch does not create an entry or erase pending redo, and a new meaningful edit after undo replaces the redo branch.

## Intended usage from a future editor

```cpp
StarterWorldHistory::History history(loadedWorld);
auto edited = history.ApplyMoves(loadedWorld, {
    {"resource.normal_tree.001", Position{30, 30}},
    {"station.furnace.001", Position{32, 30}}
});
auto restored = history.Undo(edited);
auto replayed = history.Redo(restored);
```

These examples require the source world to be valid and the target tiles to be free. Returned snapshots are candidates only. **No file is written**, no game world or player save is mutated, and no actor-spawn identities or definitions are changed.

The caller supplies the most recent snapshot to each action. History is deliberately not rebased automatically after an outside edit; the caller must inspect the changed content and begin a new session or apply a deliberate new edit. Canonical comparison detects semantic changes, not whitespace-only edits: disk publication must independently use the exact-byte expected SHA-256 and guarded new-file API.

## Terminal editing session (STUDIO-024 first surface)

The stacked `EmbervaleWorldMove --session <snapshot.json>` command opens a bounded terminal workspace.
`list` identifies all current placements; `map` or `map x y` displays a 21-by-21
terrain/occupancy view centered on the player spawn or chosen tile. `move <id> <x> <y>`,
`undo`, `redo`, and `status` operate only on an in-memory candidate using the shared
`History` implementation, not a separate command interpreter for validation.
`save <fresh-sibling.json>` revalidates and calls the existing expected-hash
create-only writer. Its source SHA-256 is captured when the session opens, so
editing the source externally before saving reports a conflict rather than
silently using the new bytes. Multiple saves require distinct sibling filenames.

The terminal view is an **interim interactive editor**, not a GUI. It does not
permit terrain painting, actor-spawn edits, in-place overwrites or publication.
The source file and player saves remain untouched, and the session's undo
history disappears on exit. See the dedicated session tests and evidence record.

## Remaining integration work

This class does not persist history across process restarts, support arbitrary object creation/removal, make undo available in the CLI, or implement UI shortcuts. A general command journal, batch file transaction and editor controls belong to later slices. When building the visual editor, connect its selection and hotkeys to this shared service rather than creating a second interpretation of world movement. Always confirm the generated candidate with the real runtime validator and use separately guarded draft publication.

See [editor workflows](EDITOR_WORKFLOWS.md), [guarded draft revisions](GUARDED_DRAFT_REVISIONS.md) and the [verification boundary](../06-development/STUDIO_HISTORY_VERIFICATION_2026-11-18.md).

## First visual shell (STUDIO-030)

An experimental desktop map viewport in [`tools/embervale_world_editor.py`](../../../tools/embervale_world_editor.py) wraps the actual `EmbervaleWorldMove --session` command. It shows the complete terrain grid, authored placements and a stable-ID inspector; clicks propose existing resource/station moves. Undo, redo and saving use the same in-memory C++ history and guarded draft publication on every replay. The editor does not write JSON itself.

`--session` additionally accepts `--expected-hash <sha256>` so the GUI can pin every replay to the exact source revision it opened and reject external source changes before executing commands. The session `save` command accepts the remainder of its line as a destination, supporting ordinary file paths containing spaces. See [visual editor scope and verification](VISUAL_WORLD_EDITOR.md). This is an experimental shell, not full editor acceptance or a completed graphical playtest.

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

## Remaining integration work

This class does not persist history across process restarts, support arbitrary object creation/removal, make undo available in the CLI, or implement UI shortcuts. A general command journal, batch file transaction and editor controls belong to later slices. When building the visual editor, connect its selection and hotkeys to this shared service rather than creating a second interpretation of world movement. Always confirm the generated candidate with the real runtime validator and use separately guarded draft publication.

See [editor workflows](EDITOR_WORKFLOWS.md), [guarded draft revisions](GUARDED_DRAFT_REVISIONS.md) and the [verification boundary](../06-development/STUDIO_HISTORY_VERIFICATION_2026-11-18.md).

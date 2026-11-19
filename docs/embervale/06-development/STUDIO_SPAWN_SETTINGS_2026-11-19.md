# Studio spawn settings implementation

**Date:** 19 November 2026  
**Base:** master `29749e032f2ccb4d5fbb663fe21a54e1887c3203` (home movement, PR #25).  
**Scope:** Remaining current-runtime behaviour fields for STUDIO-041.  
**Owner direction:** Complete the work and defer testing for now.

## Delivered behaviour

Selecting an actor exposes **Spawn settings…**. Existing monsters have editable
wander radius/interval, spawn cap, respawn enablement and per-spawn delay. The
inspector shows the accepted settings. Named NPCs show their supported read-only
behaviour settings and retain their editable map home.

The same C++ validated edit is available through `EmbervaleWorldMove --session`:

```text
spawn monster.passive_development.001 3 6 1 1 12
undo
redo
snapshot
save spawn-settings-draft.json
```

The arguments are ID, radius, wander ticks, cap, respawn boolean (0/1), and delay
ticks (0 inherits the enemy type). Invalid edits preserve the current world and
history; unchanged settings preserve a pending redo branch. One successful edit
is one history entry. Save/reopen uses the complete canonical snapshot and keeps
the selected configuration. Existing guarded sibling saves remain create-only.

Schema 3 adds bounded settings and an explicit `respawn_delay_ticks` to every
actor. Versions 1 and 2 retain their historical restrictions. Object operations
retain schema 3 after an upgrade, and allocator reservations survive undo. Actor
identity, creation order, delayed registration and gameplay bindings are retained.
The spawn cap is a runtime creation limit; it does not automatically populate a
spawn. See [ADR-019](../07-decisions/ADR_019_BOUNDED_SPAWN_SETTINGS.md) for limits
and the default-delay contract.

## Execution evidence

| Work | Outcome |
|---|---|
| C++ core, SDL game and editing-tool build | Passed; initial 23 steps and final 4-step validator rebuild, exit 0 |
| Python editor syntax check | Passed via `ast.parse` |
| C++ unit/integration and Python test suites | Not run, at the owner's request |
| Visible editor/game acceptance | Not run, at the owner's request |

Build command, using the existing Windows UCRT64 configuration:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build build-client --target GameEngine EmbervaleWorldMove --parallel 6
```

No earlier test totals are attributed to this implementation. No runtime or
visual test pass is claimed. STUDIO-041's current supported-field implementation
is delivered; its acceptance status remains `in_progress` while those checks are
deferred. Broader R2/R3 and Studio Alpha release gates remain open.

## Deferred acceptance

Exercise schema-1/2 historical rejection and schema-3 round trips, field bounds,
named-NPC and leash restrictions, no-op/undo/redo/branching, object edits after a
schema upgrade, exact bridge metadata/checkpoints, guarded save/reopen, selected
spawn authority, effective respawn timing and unchanged delayed actor creation.
Inspect the dialog and game behaviour visibly. These are follow-up checks, not
claims that they were executed during this work.

The next gameplay work package is GAME-001: an explicit unboosted fresh-player
profile, followed by the flat River Crossing blockout.

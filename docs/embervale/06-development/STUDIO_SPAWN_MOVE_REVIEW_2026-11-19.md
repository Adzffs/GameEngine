# NPC/enemy home movement — implementation record, 19 November 2026

**Status:** Implementation complete; testing and visible acceptance deferred at the owner's explicit request.  
**Base:** master `914a581ece8e3b2c4255230a4a9fa8a19e2f3520` (PR #24).  
**Planning scope:** First bounded slice of STUDIO-041: move existing actor
spawn/home positions. Behaviour-configuration editing remains separate.

## Implementation

The shared C++ move operation accepts existing NPC and monster placement IDs.
It moves the placement and its typed `NpcSpawnDefinition::spawnPosition`
together, then revalidates the entire private candidate. Definitions, stable
IDs, spawn IDs, initialization stage/order, wander settings, capacity and
respawn flags are retained. Invalid source/destination content throws before
returning a candidate.

Studio's ordinary Select / move tool exposes these moves. The inspector labels
an actor's location as its spawn/home, and actor removal stays disabled.
Creation/removal remains limited to supported resources/stations. The bridge
continues to check exact requested transitions, complete canonical snapshots,
frozen actor membership and observed undo/redo checkpoints. It permits a
requested home move while rejecting actor drift during an unrelated action.

Existing schemas 1 and 2 already represent actor coordinates. This slice adds
no world-schema field or player-save version. A home move alone does not
promote schema 1 or consume an authored-object allocation ID. GUI and C++ tool
must come from the same checkout because the placement-list heading now
describes movable actor homes.

## Added regression coverage

- `StarterWorldEditTests`: all ten actor records, typed/home parity,
  unchanged bindings and lifecycle/settings, no-op moves, v1 round-trip,
  invalid bounds/terrain/occupancy/spawn and disconnected-home rejection.
- `StarterWorldHistoryTests`: multi-actor undo/redo, rejected batch rollback
  and coexistence with schema-2 object allocation reservations.
- `StarterWorldRuntimeTests`: the existing wandering/respawn, delayed
  creation and spawn-authority scenario now uses the shared move command and
  a canonical reparse. A new scenario exercises relocated-guide dialogue,
  quest acceptance and exactly-once completion/reward, then a real shop buy.
  That scenario supplies the quest's ordinary gathered-item event directly;
  it does not claim to gather ten logs.
- Python contract checks: requested actor move/save/reopen, exact actor undo
  checkpoints and unsolicited actor drift even with a consistent full snapshot.
- Real executable-backed bridge scenarios: NPC/enemy moves and history,
  save/reopen parity, invalid homes, protected membership and mixed schema-2
  object/actor editing without resetting allocation reservations.

## Verification state

The branch was initially prepared through GitHub after the desktop execution
connection disconnected. On reconnection, the owner explicitly requested
completing the work without testing for now. The C++ core, SDL game and editing
tool were subsequently built successfully with the existing Windows/UCRT64
Debug configuration. Tests and visible gameplay acceptance remain deferred;
no runtime test pass is claimed for this change.

Executed build command (no test targets or suites):

```sh
cmake --build build-client --target GameEngine EmbervaleWorldMove --parallel 6
```

| Required check | Result for this change |
|---|---|
| C++ core / game / editing-tool compilation | Passed; test executables were not built or run |
| C++ edit/history/runtime regressions | Not run |
| Python mocked bridge/protocol checks | Not run |
| Real C++/Python bridge scenarios | Not run |
| Full game/client/graphics regression set | Not run |
| Visible Studio and SDL game acceptance | Not run |

The earlier 97 CTest / 39 Python pass counts belong to PR #24. They are not
test evidence for this change. The broader STUDIO-041 task remains in progress
because behaviour-configuration editing is separate from completed home movement.

## Deferred verification

Build this branch's core/tools and SDL client, then run:

```sh
cmake --build build-client --parallel 6
ctest --test-dir build-client --output-on-failure
python -m unittest discover -s tests -p test_world_editor_bridge.py
```

Use the configured dummy SDL driver for automated graphics checks and the
normal desktop driver for visible acceptance. Open a disposable source copy,
move the guide and a monster home, undo/redo and reject an invalid home, save
and reopen the exact draft, and use Play this draft. Confirm guide dialogue,
quest actions and trading still work; kill the relocated monster and observe
its return to the new home. Preserve ordinary player saves throughout.

The owner requested integration without waiting for these checks on
19 November 2026. They remain follow-up verification work, rather than a merge
blocker. Broader STUDIO-041 configuration work still needs its own explicit
compatibility contract.

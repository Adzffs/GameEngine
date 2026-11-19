# Resource/station placement verification — 19 November 2026

**Base:** master `77dc749194e061e1e9dbf68c71d57d6515fefb3b`.  
**Work:** STUDIO-040, bounded resource/station place/remove, including shared
history, schema-2 allocation, complete bridge-state checks and visual controls.

## Executed checks

Windows, GCC 16.1.0, CMake 4.4.0, Ninja, CPython 3.14.6 and the installed
UCRT64 SDL3 package. An isolated source checkout was used. The core/client and
all registered test executables were built in a new Debug Ninja directory;
changed sources were rebuilt incrementally during development.

```sh
cmake --build build-client --parallel 6
ctest --test-dir build-client --output-on-failure --parallel 4
python -m unittest discover -s tests -p test_world_editor_bridge.py
```

The CTest run used `SDL_VIDEODRIVER=dummy` and passed **97/97**: 85 headless,
7 graphics and 5 client tests. The actual C++/Python bridge test passed
**10/10 scenarios** within that total; the separate mocked bridge, protocol
and viewport checks passed **39/39**. Those counts are different test sets,
not 146 independent game integration scenarios.

The added C++ placement executable checks all ten supported definitions;
source immutability; v1/v2 round-trips and historical identity restrictions;
allocation exhaustion and rejection; water, occupancy, spawn, bounds and
route-blocking rejection; mixed move/paint/create/remove undo/redo; branch
replacement without ID reuse; exact revision conflicts; removed-ground
restoration; runtime station creation; and successful gathering at a new tree.

The real bridge scenarios check create/remove/move/paint save/reopen parity,
definition/identity display, retained allocation reservations, redo branching,
invalid commands and unchanged source bytes. Existing persistence, combat,
quest, crafting, shop and client/graphics tests remain included in the full run.

## Visible desktop acceptance

Using the actual Tk editor and C++ tool on a temporary fixture copy:

1. Placed a normal tree at (22,11), then removed it and restored it using Ctrl+Z.
2. Placed a furnace at (24,11). The inspector/list showed the stable authored
   IDs and correct definition bindings.
3. Fixed a discovered toolbar clipping problem by separating the filename/state
   caption from the edit controls. Repeated placement with the corrected layout
   at the default 1120×760 window size; Save remained visible while dirty.
4. Saved through the native dialog with its suggested fresh sibling filename.
   The verified-save acknowledgement appeared and the dirty indicator cleared.
5. Reopened the draft. Both authored objects remained and history reset to zero.
6. Used **Play this draft**, which launched the actual SDL client with temporary
   progress. Both the added tree and furnace appeared in the game world.
7. Walked to the new tree, equipped the starting bronze axe and gathered from
   it. The gathering bar appeared; a **Log** appeared in inventory and its item
   details confirmed the reward. Closed the game and test editor normally.

The saved world contains `resource.authored.000001` at (22,11),
`station.authored.000002` at (24,11), schema 2 and `next_placement_id: 3`.
Its SHA-256 is
`147ef7f434d3ea40429259786cd8e50d592822b54f38cf60a36016ede4b20fc3`.
The copied source retained the canonical fixture SHA-256
`33477349aceb2899fcdc8a4715a42833dc3e35f7cc44d37045dd9466b8fd7957`.
These were disposable test files; the ordinary development player save and the
already-open unrelated Studio session were left untouched.

## Remaining scope

This completes the bounded current resource/station placement operation.
It does not finish R2/Studio Alpha, enable actor placement, implement crash
recovery/in-place publication/manifests, establish another platform's desktop
behaviour, or prove a balanced River Crossing playthrough. See [ADR-018](../07-decisions/ADR_018_BOUNDED_OBJECT_PLACEMENT.md).

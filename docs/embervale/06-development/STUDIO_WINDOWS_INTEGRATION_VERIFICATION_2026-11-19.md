# Studio Windows integration verification — 19 November 2026

## Scope and revision

This record verifies the complete stacked Studio branch through
`5750aef27547af31c054d865da010d09a0991972`, plus the Windows integration
corrections committed with this record. It does not claim a graphical
interaction or rendering-quality playtest.

The checks ran in a separate worktree on Windows NT 10.0.26200 with CMake
4.4.3, Ninja 1.13.2, GCC 16.1.0 and CPython 3.13.14. The build disabled the
SDL client and graphics tests and enabled all headless tests and content tools.

## Defects reproduced and corrected

The first complete run built successfully but passed only 80 of 83 registered
tests. `StarterWorldMoveCliTests`, `StarterWorldPaintCliTests` and
`StarterWorldInteractiveSessionTests` all failed because the guarded draft
writer relied on a hard-link publication step that returned access denied on
Windows.

The writer now uses Windows `CREATE_NEW` plus bounded writes and flush/close
checks. This retains the create-only contract: it cannot replace an existing
draft, and an incomplete file created by this process is removed on failure.
Non-Windows publication continues to use the same-filesystem hard-link path.

The newly enabled real bridge gate then found a case-only canonical-path
difference in the sandboxed Windows temporary directory. Receipt checking now
compares absolute paths with the platform's case normalization while still
requiring exactly one acknowledgement and one SHA-256 receipt. Python-backed
mock tools are launched through the current interpreter so their contract
suite is portable to Windows; compiled editor tools remain launched directly.

## Commands and outcomes

```text
cmake -S . -B build-headless -G Ninja -DCMAKE_BUILD_TYPE=Debug
  -DBUILD_TESTING=ON -DGAMEENGINE_BUILD_CLIENT=OFF
  -DGAMEENGINE_BUILD_GRAPHICS_TESTS=OFF
  -DPython3_EXECUTABLE=<CPython 3.13.14>
cmake --build build-headless --parallel
ctest --test-dir build-headless --output-on-failure
python -m unittest discover -s tests -p test_world_editor_bridge.py -v
python -m py_compile tools/embervale_world_editor.py
  tests/StarterWorldVisualBridgeIntegration.py
  tests/test_world_editor_bridge.py
```

- Full C++/content/real-bridge CTest set: **84/84 passed**.
- `StarterWorldVisualBridgeIntegration`: **6/6 passed** against the built
  `EmbervaleWorldMove.exe` and the real 100×100 starter fixture.
- Mocked bridge and viewport contract suite: **35/35 passed**.
- Python syntax compilation: passed for the editor and both bridge suites.
- Player-save state, codec, file-store and lifecycle tests all passed as part
  of the 84-test set. The editor tests use private temporary fixture copies and
  no live player save.

The real bridge gate covers stable-ID movement, terrain painting, undo/redo and
redo-branch replacement, rejected invalid edits, exact-byte source conflicts,
fresh space-containing sibling draft publication, receipt/hash verification,
semantic readback and reopening the saved draft. The original snapshot remains
byte-identical in every successful case.

## Remaining boundary

The stacked branch is locally consolidated and all earlier Studio branch tips
are ancestors of this verified head. A visible Tk interaction and SDL client
playtest of the produced draft remain separate manual acceptance work.

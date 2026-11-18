# Studio real-executable bridge regression gate — 18 November 2026

**Scope:** STUDIO-030 integration verification, not a new player-facing editor feature.
**Parent reviewed:** `studio/authoritative-terrain-view` at
`d6f79d4f66f300c2b45aeb370d23cbabb7243fde` (draft PR #17).
This new work remains stacked and unmerged.

## Capability and coverage

- Optional local CMake test calls the real built `EmbervaleWorldMove` from
  `tests/StarterWorldVisualBridgeIntegration.py` through Python `StudioBridge`.
- A private copy of the canonical 100x100 snapshot is used for every test.
- The five cases cover parity, stable-ID movement, terrain edits, shared
  undo/redo branching, rejected edits, sibling draft saving, reopening and
  runtime hash validation, overwrite rejection, and source-revision conflicts.
- The canonical content file, running game and player saves are untouched.
- If Python is absent at configure time, the test is not registered.

## Verification observed on this run

- Read the parent branch, open draft PRs, master/Studio roadmap and backlog,
  existing evidence, C++ edit/session/history and the actual content fixture.
- The exact new test file (`763da5ef9cd066b03441018267e2b3cccf71e490`
  Git blob SHA) passed Python 3.13.5 `py_compile` and `ast.parse` locally.
- An isolated CMake 3.31.6 + Ninja registration scaffold containing the
  exact new Python test-registration block configured and built a dummy
  executable; `ctest -N -R StarterWorldVisualBridgeIntegration` listed
  **one test** with the intended executable/fixture arguments.
  This is a CMake syntax/registration smoke, **not** the production
  GameEngine configuration or a test execution.
- The local host has no complete GameEngine checkout; `git ls-remote`
  failed with `Could not resolve host: github.com`.
- **Not run:** new executable integration tests, full real-core build,
  existing committed bridge tests, Tk click-through, Windows or game playtest.
  No actual executable regression pass is claimed.

Keep the editor stack in draft until a full checkout permits those checks.

## Next gate

```sh
cmake -S . -B build-headless -G Ninja -DBUILD_TESTING=ON -DGAMEENGINE_BUILD_CLIENT=OFF
cmake --build build-headless --parallel 4
ctest --test-dir build-headless --output-on-failure -R StarterWorld
python3 -m unittest discover -s tests -p test_world_editor_bridge.py -v
```

Record exact tested commit, run the graphical editor against a built real
executable, and fix any integration regressions before merging stacked PRs.

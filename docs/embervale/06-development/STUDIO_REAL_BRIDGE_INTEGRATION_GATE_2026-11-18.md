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

## Follow-up: fail-closed bridge transition checks (18 November 2026)

At the PR #18 integration gate, review found the Python bridge checked placement
IDs/kinds and actor bounds but did not require a successful C++ response to
actually perform the requested move/paint or advance the expected undo cursor.
This could silently accept divergent tool output while displaying the wrong
world state.

- The bridge now checks applied/redo history count transitions for move,
  paint, undo, and redo; a new edit after undo must clear redo.
- Moving a resource/station must change exactly that placement and preserve
  ground rows. Painting must change only the requested ground tile and
  preserve placements. Undo/redo must visibly change placement or terrain.
- The response is checked **before** the UI mutates its map or replay list;
  failed checks leave the source, current GUI view and undo commands alone.
- Four new fake-tool regression cases cover ignored moves, incidental ground
  changes, incidental placement moves, and inconsistent backend history counts.
- Local Python 3 isolated extraction of the exact pure transition checker:
  `py_compile` passed and **4 unittest methods passed**, covering valid moves,
  paints, undo/redo, branching, stale cursor and malformed replies. This was
  deliberately an extracted helper test, **not** the committed Python module,
  full bridge suite or actual C++ executable. The exact source baseline
  inspected was `3e1ddf8b6c993182feacf689cac31a32c9e648a2`; no full-repository build or live editing
  test was possible because the host still cannot resolve `github.com`.
  Keep the PR draft and avoid integration until full tests pass.

## Follow-up: observed-revision checkpoint parity (18 November 2026)

Review of the Python StudioBridge at the integration gate identified a remaining
consistency hole: the existing transition checks required undo/redo cursor counts
and **some** visible change, but accepted an undo or redo that returned an
arbitrary in-bounds world instead of the earlier state. The C++ history protects
its own canonical revisions, yet the graphical front end also needs to detect a
malformed/mismatched response rather than silently displaying it.

- The bridge now caches only world states **actually observed** from the
  authoritative C++ tool: placements and terrain rows at each applied history
  cursor. Undo and redo must reproduce the exact earlier observed state.
- After a new move or paint from an undone cursor, the bridge drops cached redo
  checkpoints before recording the new tool response. It does not compute
  terrain inverses, modify authored JSON, or add another gameplay validator.
- Rejected/mismatched backend responses leave the displayed state, replay
  commands and checkpoint history unchanged.
- Three committed mocked-backend regressions cover incorrect undo, incorrect
  redo and redo-branch replacement after painting.

**Executed locally:** Python 3.13.5 `py_compile` and **6/6 unittest cases**
against the exact newly added *standalone helper* reproduced in an isolated
test file. The cases covered accepted move/undo/redo, incorrect undo and redo,
redo-branch replacement, impossible cursor and read-only no-op. This is **not**
execution of the complete committed bridge or its mocked-backend suite.

**Not executed:** the full Python bridge regression module, the new
real-executable integration test, complete GameEngine C++ build, GUI click-through,
Windows, or a gameplay playtest. Local `curl` to both github.com and
api.github.com failed with DNS resolution errors; direct HTTPS IP connections
were also unavailable. The repository was inspected and edited through its
connected GitHub repository interface. Keep PR #18 draft and stacked.

## Follow-up: verify actual draft bytes after publication (18 November 2026)

A review found that `StudioBridge.save` accepted any `Saved new draft:`
substring, without confirming the requested path or comparing the C++
`draft-sha256` receipt to the on-disk artifact. The C++ writer is already
guarded, but the UI could still report success for an unexpected/missing or
mismatched file when the tool response was wrong or the file was changed
immediately after publication.

- The bridge now requires one exact requested-path acknowledgement and one
  valid SHA-256 receipt; it rejects missing/nonregular/symlink/oversized
  outputs and reads back the saved bytes before reporting success.
- The fake executable test writer now emits a valid authored JSON draft with
  placements/terrain and the corresponding content digest (rather than
  placeholder text).
- Three committed fake-backend tests cover mismatched digests, wrong paths
  and missing files. Failures leave the displayed world and command history
  unchanged; questionable files are intentionally **not** removed.
- Locally executed **8/8** Python 3.13.5 standalone unit checks against the
  exact new receipt helper body: valid draft, wrong path/hash, missing file,
  duplicate/malformed receipt, symlink and oversized file. This is an
  **isolated helper execution**, not the complete committed bridge tests.

**Remaining verification:** Full committed Python bridge tests, complete
GameEngine build, real executable-backed integration, GUI click-through,
Windows validation and saved-world gameplay were not run because the local
compiler environment cannot resolve GitHub to clone the repository. The PR
must remain draft and stacked until complete tests can run.

### Source identity and review boundary

- Reviewed input head: `dae2b56e69c49dd42ac54de5a41d283b2288de93`.
- Receipt-verification implementation and checked-in regression source:
  [`1a1a4531249ed12e016193b2119a9eaf96a851a8`](https://github.com/Adzffs/GameEngine/commit/1a1a4531249ed12e016193b2119a9eaf96a851a8)
  (`tools/embervale_world_editor.py` and `tests/test_world_editor_bridge.py`).
- Locally executed on 18 November 2026: Python 3.13.5
  `python -m py_compile /tmp/test_draft_receipt.py`, then
  `python /tmp/test_draft_receipt.py` — 8 tests passed. The isolated
  fixture embeds the new helper body; the committed full module and fake
  executable regressions were **not** run. The GitHub commit and six-file
  diff were read back after pushing, and the PR head was checked.
- These tests exercise only draft-receipt fail-closed behaviour; they do
  not establish full C++ importer parity, successful game playtest, or
  successful integrated Studio publication.


## Follow-up: exact authored draft parity after hash verification (18 November 2026)

**Reviewed input:** PR #18 head `0f2b348bb301f76870346b6f37b4fa2f8e9c059f`.
**Implementation/tests:** [`a9eda09c1ee24aec721c59eced99b48654db4079`](https://github.com/Adzffs/GameEngine/commit/a9eda09c1ee24aec721c59eced99b48654db4079),
editor blob `3764b994ae900126c8984f52bef70999a4dbe14d`,
test blob `515b512d191599cfea70a6172b1e448745a73875`.

The existing digest check accepted correctly hashed files even if a faulty
backend wrote a *different* world than the one validated and displayed. The
bridge now uses the identical bytes read for SHA-256 verification to compare
against the original snapshot with **only** current authoritative terrain
rows and stable-ID placement coordinates substituted. Unchanged player spawn,
kind/definition bindings, spawn/lifecycle metadata, dimensions and other
schema fields must be preserved; canonical placement ordering may differ.
The bridge reports a save failure if parity is violated, without deleting
the uncertain output or changing its editor history.

- **Actually executed locally:** `python -m py_compile /tmp/embervale_saved_parity_test.py`
  and `python /tmp/embervale_saved_parity_test.py`: **8/8 passed**
  (correct save, reordered IDs, wrong ground, wrong position, altered spawn,
  missing metadata, malformed JSON, kind mismatch). This standalone
  file contained the **same pure parity helper body** as the committed
  change, with a minimal isolated fixture. It is not the checked-in suite.
- **Committed but not executed:** four new mocked-backend regression cases
  in `tests/test_world_editor_bridge.py`, including move/paint and
  correctly hashed wrong-world outputs.
- **Not executed:** full committed Python test module, complete C++ build,
  actual executable bridge integration, graphical click-through, Windows
  or game playtest. On this run, `git ls-remote` again failed with
  `Could not resolve host: github.com` in the local execution container.
  GitHub repository access was available through the connected repository
  interface; the exact commit and both changed files were read back
  after the push. Keep PR #18 draft and stacked until end-to-end testing.

**Next:** obtain a full checkout in a build-capable environment; execute the
new and existing Python mock contracts plus the real `StarterWorld` C++/CLI
tests. Fix any actual failures and consolidate the validated PR stack.


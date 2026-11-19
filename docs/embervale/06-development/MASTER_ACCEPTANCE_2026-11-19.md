# Master acceptance — 19 November 2026

Verified master: `2e3da94cf6bfaefc0b341824069b8af0472ff0e2` (PRs #18–#22 merged).
The dedicated Studio worktree was switched to a documentation branch starting
at that exact master. Runtime, editor and test sources were unchanged during
acceptance. Existing unrelated game-feel work and player saves were preserved.

## Environment and execution

Windows NT 10.0.26200, GCC 16.1.0, CMake 4.4.3, Ninja 1.13.2 and CPython
3.13.14; existing Debug Ninja headless and SDL client build directories.
Both `cmake --build build-headless --parallel` and
`cmake --build build-client --parallel` succeeded (incremental builds, no work
required; this is not a claim of a fresh clean compilation).

- `ctest --test-dir build-headless --output-on-failure`: **84/84 passed**.
- `SDL_VIDEODRIVER=dummy ctest --test-dir build-client --output-on-failure`:
  **96/96 passed**, comprising 84 headless, 7 graphics and 5 client tests.
- `python -m unittest discover -s tests -p test_world_editor_bridge.py`:
  **37/37 passed**.
- The real C++/Python bridge gate ran in both configurations: **7/7 scenarios**
  including mixed move/paint undo/redo, branch replacement, invalid edit
  rejection, source conflicts, guarded save/reopen and isolated launch checks.

The SDL dummy driver was confined to automated checks. The subsequent visible
editor/game process used the normal Windows desktop driver.

## Visible acceptance on master

Opened a temporary copy of the canonical starter fixture in the actual Tk
editor. Selected the normal tree at (5,5), moved it to (22,11), undid the move
(saved state restored), and redid it (unsaved state restored). Saved through
the native dialog to the fresh sibling `master acceptance 2026-11-19.json`.
The verified-save acknowledgement appeared and the dirty indicator cleared.
Reopen last saved loaded that draft, retained the tree at (22,11), and reset
history to zero. Play this draft launched the real SDL client, disabled the
launch button, and displayed the edited world with the relocated tree after
the welcome panel was dismissed. The game was closed normally.

The validated draft SHA-256 is
`b4ad3f4706e649e0d0651a1c8c06c4ccc44d339e97070f46cfbf9419c7dc7713`.
Both the fixture and temporary source retained SHA-256
`33477349aceb2899fcdc8a4715a42833dc3e35f7cc44d37045dd9466b8fd7957`.
The visible draft contains the move, not a terrain change. Mixed terrain
save/reopen and invalid-edit behavior were exercised by the real bridge tests.
Earlier visible collision/spawn/reachability and close-cancellation checks are
recorded in the editor guide; they were not all manually repeated in this pass.

## Roadmap interpretation

The bounded edit/save/reopen/play acceptance passes on current master. This
does not close every R2/Alpha gate: interruption recovery, in-place atomic
publication, immutable versioned manifests, structured capabilities and an
accessibility audit remain outstanding. STUDIO-032 and STUDIO-033 are now
in_progress. Resource/station creation/removal is the next authoring slice,
dependent on stable-ID allocation and save compatibility decisions.

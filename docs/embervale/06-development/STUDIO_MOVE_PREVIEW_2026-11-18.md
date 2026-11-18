# STUDIO-020 first move-preview slice — 18 November 2026

**Base:** master at `c59c57b5b1a696a9a1246731e79c692f68b5327f`.  
**Branch:** `studio/stable-id-move-preview` (not a completed Studio release).  
**Scope:** Typed, no-write move preview of existing resource/station by durable ID.

The new `StarterWorldEdit::MovePlacement` validates its source, copies the snapshot, looks up one durable ID, changes only its coordinate and revalidates the whole candidate through the runtime importer. The original terrain rows and all placement identities, lifecycle/order and typed definition bindings are retained. An identical coordinate is an explicit no-op. Unknown IDs and actor-spawn moves fail instead of silently succeeding. No runtime world, save file or content file is mutated.

`EmbervaleWorldMove` exercises this operation from a selected snapshot file. It accepts strict integer coordinates and reports the old/new location after validation. It writes no file. The code also adds `StarterWorldEditTests` and `StarterWorldMoveCliTests`, checking valid moves, inverse/no-op behaviour, definition and terrain preservation, bad identity, overlap, water, spawn collision, out-of-bounds input and bad CLI arguments.

## Verification boundary

The repository was inspected through a connected GitHub repository reader and edits were committed through its write API. The current task runtime could not resolve `github.com` from the local build container, so a full repository checkout and game build/test execution **have not been performed** for this new commit. The previously reported **89/89** passing game tests belong to the earlier runtime-import commit, not to this change. The C++ tests are registered but must be executed on this branch before its PR can be treated as verified and merged. The current static code review is not a substitute for that check.

Run a local headless build on this exact branch:

```bash
cmake -S . -B build-headless -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DGAMEENGINE_BUILD_CLIENT=OFF -DGAMEENGINE_BUILD_GRAPHICS_TESTS=OFF
cmake --build build-headless --parallel 4
ctest --test-dir build-headless --output-on-failure -R 'StarterWorld(Edit|Move|Runtime|Snapshot)'
./build-headless/EmbervaleWorldMove tests/fixtures/content/development-starter.snapshot.json resource.normal_tree.001 30 30
```

Next: verify and correct this slice, then implement expected-hash guarded draft writes with conflict rechecks and atomic publication boundaries, followed by undo/redo and a minimal visual editor.

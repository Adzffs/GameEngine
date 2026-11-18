# Starter-world runtime import verification — 18 November 2026

## Implementation and scope

The bounded runtime importer is implemented at [a498b240651f64bb673e5fb0c1db8f093e76b77c](https://github.com/Adzffs/GameEngine/commit/a498b240651f64bb673e5fb0c1db8f093e76b77c). Its Git tree is `de267dd4d7410689aba1525a2f4e5ab48b1f7403`, matching the locally reviewed implementation tree exactly. The accompanying documentation changes do not change executable source.

The supported input is the existing 100×100 starter snapshot with its twenty fixed identities and lifecycle bindings. Coordinates and ground can change after structural and gameplay validation. No-argument startup continues to use compiled content. Explicit `--world` startup loads the chosen content or fails; it never silently substitutes another world. Imported playtests use temporary player progress unless an explicit `--save` path is provided.

This record covers runtime loading and its compatibility checks. Shared editing commands, authored-content writes, undo/redo, publication and the visual editor remain future work.

## Environment and results

| Check | Configuration | Observed result |
|---|---|---|
| Headless build and suite | Linux, GCC 14.2.0, CMake 4.4.5, Ninja, C++20, Debug; client and graphics tests disabled | Build succeeded; **77/77 passed** |
| SDL client build and complete suite | Same compiler/build settings; SDL 3.4.20; client and graphics tests enabled | Build succeeded; **89/89 passed** |
| SDL test driver | `SDL_VIDEODRIVER=dummy`, `SDL_RENDER_DRIVER=software`, `SDL_AUDIODRIVER=dummy` | Includes 77 headless, 5 client and 7 graphics tests; these counts partition the same 89 tests |
| Standalone tools disabled | `GAMEENGINE_BUILD_CONTENT_TOOLS=OFF`; runtime and launch-option targets built | Both focused tests passed (**2/2**); runtime input remains available |
| Edited-content game smoke | Normal-tree identity moved to `(40,20)` in a temporary snapshot; actual `GameEngine --world ... --no-save`; dummy video/software renderer | File validated; game ran for two seconds, logged startup, accepted a quit signal, logged shutdown and exited **0**; no save or other files were created |
| Diff and documentation | Whitespace check plus repository documentation checker | Passed; **52 Markdown files, 824 links and 87 matching backlog tasks**, with an acyclic dependency graph and all 45 legacy Studio IDs retained |

The complete SDL suite was first attempted without a display. Two renderer-initialization tests reported no available video device. Running with the explicit dummy/software driver above passed all 89 tests. This is an environment-specific rendering test, not a manual visual playtest. Windows execution was not performed in this session.

The [machine-readable results](verification/2026-11-18/runtime-import-results.json) preserve the environment, counts and smoke result without machine-specific paths.

## What the checks prove

- **Unedited compatibility:** imported and compiled worlds agree across all 10,000 effective tiles and passability checks, resource/station identities and counts, pre-player and post-player entity order, delayed population creation and a deterministic 30-tick comparison.
- **Usable placement changes:** a relocated tree restores the old tile's authored ground, occupies the destination, retains its identity and accepts gathering through the authoritative action lifecycle after the required axe is equipped.
- **Spawn authority:** selected per-world spawn definitions control moved NPCs and monsters, delayed creation, wander state and respawn locations. Forged bindings/settings and population-capacity violations remain rejected.
- **Safe input rejection:** byte, string, nesting and aggregate-value limits, duplicate JSON keys, malformed numbers/types, unsupported schema/IDs/settings, incomplete terrain and unsafe gameplay placements are rejected before constructing the runtime world.
- **Saved locations:** blocked or disconnected saved players fall back to the selected safe spawn. Unsafe recovery piles relocate with their items and lifetime intact; a regression recovers 77 saved coins into an existing balance of 123. Loading does not rewrite the source save, and the player save format is unchanged.
- **Startup isolation:** invalid or missing selected content fails before graphics or the save lifecycle starts. Save paths and their `.tmp`/`.bak` sidecars cannot alias the selected world through normalized paths, symlinks or existing hardlinks. Rejection tests confirm all input/save bytes remain intact.
- **Build boundaries:** the parser, runtime validation and adapter belong to the SDL-free core. Omitting standalone tools does not remove runtime input.

Relevant tests are [runtime input](../../../tests/StarterWorldRuntimeInputTests.cpp), [runtime compatibility](../../../tests/StarterWorldRuntimeTests.cpp), [parser](../../../tests/StarterWorldSnapshotInputTests.cpp), [launch options](../../../tests/GameLaunchOptionsTests.cpp), [actual client startup](../../../tests/GameEngineStartupCliTests.cmake) and [validator executable](../../../tests/StarterWorldValidateCliTests.cmake).

## Reproduce

From the repository root, build the headless configuration:

```bash
cmake -S . -B build-headless -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DGAMEENGINE_BUILD_CLIENT=OFF -DGAMEENGINE_BUILD_GRAPHICS_TESTS=OFF
cmake --build build-headless --parallel
ctest --test-dir build-headless --output-on-failure -L headless
./build-headless/EmbervaleWorldValidate tests/fixtures/content/development-starter.snapshot.json
```

For the client, configure the normal SDL3 package location for the machine, then:

```bash
cmake -S . -B build-client -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DGAMEENGINE_BUILD_CLIENT=ON -DGAMEENGINE_BUILD_GRAPHICS_TESTS=ON
cmake --build build-client --parallel
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDL_AUDIODRIVER=dummy ctest --test-dir build-client --output-on-failure
```

Use a normal desktop video driver for an interactive playtest:

```bash
./build-client/GameEngine --world tests/fixtures/content/development-starter.snapshot.json --no-save
```

See the [runtime import guide](../04-studio/STARTER_WORLD_IMPORT.md) for supported edits, explicit save paths, rejection behavior and remaining work.

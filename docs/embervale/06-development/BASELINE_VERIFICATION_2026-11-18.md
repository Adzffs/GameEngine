# Baseline and starter-world export verification

**Date:** 18 November 2026 (GMT/UTC)\
**Unchanged baseline:** [`c697386382845f17ce02027e6bade24591a9eb36`](https://github.com/Adzffs/GameEngine/commit/c697386382845f17ce02027e6bade24591a9eb36)\
**Changed implementation:** The read-only exporter change containing this record; exact tested code/fixture hashes and per-test results are in [results.json](verification/2026-11-18/results.json).\
**Result:** Local baseline and exporter configurations passed. GitHub CI remains blocked before execution by an account billing lock.

## 1. Actual local results

| Configuration | Headless | Client | Graphics | Total passed | Failed/skipped |
|---|---:|---:|---:|---:|---:|
| Unchanged baseline, headless build | 70 | — | — | 70 | 0 / 0 |
| Unchanged baseline, client build with dummy driver | 70 | 4 | 7 | 81 | 0 / 0 |
| Exporter change, headless build | 72 | — | — | 72 | 0 / 0 |
| Exporter change, client build with dummy driver | 72 | 4 | 7 | 83 | 0 / 0 |

All configure, build and CTest commands exited `0`. Baseline client testing took 1.69 seconds; changed headless testing took 2.03 seconds; changed client testing took 2.13 seconds as reported by CTest. These are execution observations, not performance budgets or an optimisation claim.

The two new CTest entries are `StarterWorldSnapshotTests` and `StarterWorldExportCliTests`. The first is a C++ executable; the second is a CMake integration script. The 83 total therefore means **83 CTest entries**, not 83 C++ test executables. The snapshot executable passed 386 assertions in its recorded run.

The original runtime source and existing test files were not modified. A separate clean detached worktree at the baseline SHA established the 81-test result before the exporter was tested in its implementation checkout. Existing persistence, movement, combat, quest, interaction, client and graphics suites are included; passing them does not establish complete gameplay coverage.

## 2. Environment

| Component | Version or configuration |
|---|---|
| Operating system | Debian GNU/Linux 13, x86_64 |
| Compiler | GCC/G++ 14.2.0 |
| CMake | 4.4.5 |
| Ninja | 1.13.2.git.kitware.jobserver-pipe-1 |
| Engine configuration | Debug, `BUILD_TESTING=ON` |
| SDL | 3.4.20, shared Release library |
| SDL source | [`f00dc919ca35b9756316e86c4777392de442f9b2`](https://github.com/libsdl-org/SDL/commit/f00dc919ca35b9756316e86c4777392de442f9b2), tag `release-3.4.20` |
| Test video driver | `SDL_VIDEODRIVER=dummy` |

SDL was built into a local prefix with dummy/offscreen video and disk/dummy audio. X11, Wayland and external audio backends were disabled because the test environment did not provide their development packages. The software renderer remained available. This setup proves automated client/graphics behaviour in that configuration; it is not a native desktop, GPU, audio-device or visible interactive playtest. SDL's [pinned build guidance](https://github.com/libsdl-org/SDL/blob/f00dc919ca35b9756316e86c4777392de442f9b2/docs/README-cmake.md) describes the supported CMake installation approach.

No Windows/UCRT64 execution or macOS build is claimed. The checked-in Windows workflow remains the project’s Windows verification path once GitHub can schedule its jobs.

## 3. Reproduction commands

Run in a clean checkout of the intended commit. Record `git rev-parse HEAD` and `git status --short` first. Use separate build directories for separate toolchains/configurations.

### Headless

```bash
cmake -S . -B build-headless -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DGAMEENGINE_BUILD_CLIENT=OFF -DGAMEENGINE_BUILD_GRAPHICS_TESTS=OFF
cmake --build build-headless --parallel 4
ctest --test-dir build-headless --output-on-failure -L headless
```

The exporter is enabled by default in this change. The unchanged baseline has 70 headless tests; this change has 72 when `GAMEENGINE_BUILD_CONTENT_TOOLS=ON`.

### Local SDL dependency used for the client verification

The paths below are reproducible workspace choices. Run these commands from the directory containing an engine checkout, and keep dependency source/build/install output outside the engine repository:

```bash
git clone --depth 1 --branch release-3.4.20 https://github.com/libsdl-org/SDL.git deps/SDL
git -C deps/SDL rev-parse HEAD
cmake -S deps/SDL -B deps/sdl-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PWD/deps/sdl-install" -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_UNIX_CONSOLE_BUILD=ON -DSDL_DUMMYVIDEO=ON -DSDL_DUMMYAUDIO=ON -DSDL_X11=OFF -DSDL_WAYLAND=OFF -DSDL_ALSA=OFF -DSDL_JACK=OFF -DSDL_PIPEWIRE=OFF -DSDL_PULSEAUDIO=OFF -DSDL_SNDIO=OFF
cmake --build deps/sdl-build --parallel 4
cmake --install deps/sdl-build
```

Confirm the SDL commit matches the pinned SHA above. For actual desktop play, install/build the platform's normal display/audio dependencies rather than using this deliberately limited test configuration.

### Client and all tests

From the engine checkout, replace `/absolute/path/to/deps/sdl-install` with that local prefix:

```bash
cmake -S . -B build-client -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DGAMEENGINE_BUILD_CLIENT=ON -DGAMEENGINE_BUILD_GRAPHICS_TESTS=ON -DCMAKE_PREFIX_PATH=/absolute/path/to/deps/sdl-install
cmake --build build-client --parallel 4
SDL_VIDEODRIVER=dummy ctest --test-dir build-client --output-on-failure --no-tests=error -L '^(headless|client|graphics)$'
```

The recorded runs also captured `ctest --show-only=json-v1` and JUnit output, reconciling the discovered names and labels with executed results. The committed [machine-readable record](verification/2026-11-18/results.json) contains the environment, counts, source hashes and changed-client per-test outcomes without local machine paths.

## 4. What the new tests establish

[Snapshot tests](../../../tests/StarterWorldSnapshotTests.cpp) reconstruct the effective map from exported ground plus placement occupancy and compare every one of the 10,000 tile types and passability results with a real `World`. They repeat the map checks after player registration, preserve the pond and underlying ground, verify the shared resource/station creation order, and verify all ten spawn definitions and their lifecycle stages.

They also check actual pathfinder endpoints, repeated player registration without duplicate starter actors, deterministic output under reordered storage and a changed numeric locale, and rejection of malformed in-memory snapshot structure. A bounded interaction test queues movement and station interaction, opens the actual furnace, and confirms that subsequent export still describes initial authored content.

[CLI tests](../../../tests/StarterWorldExportCliTests.cmake) parse the executable's JSON independently through CMake, check complete dimensions/counts, compare the reviewed fixture, repeat the export, exercise argument errors and verify that no files are created in the isolated working directory.

The reviewed fixture contains **18,149 bytes**, including all 100 terrain rows and 20 placements. Its SHA-256 is:

```text
33477349aceb2899fcdc8a4715a42833dc3e35f7cc44d37045dd9466b8fd7957
```

These tests establish the export baseline. They do not prove a parser, external runtime loader, arbitrary edited-map validation, guarded save transaction or GUI. Those features do not yet exist.

## 5. GitHub CI diagnosis

The public [run 44018694678](https://github.com/Adzffs/GameEngine/actions/runs/44018694678), for engine commit `e9d32932e8f9b27e96283313cca320058792a81f`, shows this error for both jobs:

> The job was not started because your account is locked due to a billing issue.

The annotation was observed on **18 November 2026 at 03:23:32 UTC** through the public, signed-out run page. Both the [Linux job](https://github.com/Adzffs/GameEngine/actions/runs/44018694678/job/133822729389) and [Windows job](https://github.com/Adzffs/GameEngine/actions/runs/44018694678/job/133822729009) had no reported steps, no assigned runner and approximately two-second durations. Log requests returned HTTP 404. The additional Ubuntu runner migration notice was separate from the account-lock errors.

This resolves the original uncertainty about that run: it did not compile or test the source. No workflow source change is justified as a fix for this account error. Local passes do not turn that run green.

The repository owner must resolve the account billing lock, then rerun the intended commit’s Linux and Windows jobs. This implementation remains reviewable while that external action is outstanding. Account settings were not changed, and no passing GitHub result is claimed.

## 6. Remaining work and review boundary

The export-only architecture is recorded in [ADR-016](../07-decisions/ADR_016_READ_ONLY_WORLD_EXPORT.md). [Backlog progress](../02-roadmap/BACKLOG.md#current-progress) distinguishes implemented-for-review work from incomplete larger tasks.

Before the first editable world can be selected by the game, complete bounded parsing, reference/provider integration, spatial and approach validation, saved-location compatibility and unchanged-world loading equivalence. Then add the first shared move operation, persistence guards, undo and preview. The fresh-player profile and suspected economy chain remain separate game tasks.

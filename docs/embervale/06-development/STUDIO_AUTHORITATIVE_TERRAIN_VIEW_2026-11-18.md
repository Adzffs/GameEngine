# Authoritative world-editor terrain view — 18 November 2026

**Scope:** Bounded follow-up to STUDIO-030, stacked on `studio/visual-terrain-brush` / PR #16. **Status:** draft, not integrated into master.

## Capability

Added a read-only `terrain` command to the existing C++ editing session. It prints the current snapshot's complete base-ground rows (G/D/S/W) between explicit delimiters. Python/Tk requests those rows on startup and after every successful move, paint, undo or redo. It checks row count, row width and glyphs, and requires initial C++ rows to equal the source JSON's displayed rows. The UI never constructs terrain by replaying Python-side inverses. A failed command or malformed output cannot update placements, terrain or history.

Added fake-backed Python regression cases for missing terrain, malformed dimensions and startup disagreement, plus an interactive CMake scenario requesting terrain and checking delimiters. These committed tests require execution before merging.

## Verification and next gate

Static source inspection and exact-match source edits were performed on GitHub's fetched branch content. The local container could not resolve github.com or raw.githubusercontent.com (network restriction), so it could not obtain a complete checkout. **No complete project build, committed Python suite, C++ interactive test, Windows test or real game/editor playtest ran in this session.** Do not merge based on this record. Next, run the full project build and targeted StarterWorld tests on a complete checkout, then run `python -m unittest discover -s tests -p test_world_editor_bridge.py -v` and inspect the actual Tk editor. Resolve defects before integrating stacked branches in dependency order.

## Additional isolated protocol checks

A self-contained GCC C++20 smoke executable reproduced the new `PrintTerrain` serializer and passed under `-Wall -Wextra -Werror`, confirming the expected delimiters and rows. A separate Python `unittest` harness exercised the new delimiter/dimension/glyph parsing logic: **6/6 passed** (valid rows, missing response, truncated height, invalid glyph, invalid width and latest response selection). These checks ran locally after the push, but **they were not the checked-in GameEngine test files and did not link against the actual engine**. The full integration/build status stated above remains unchanged.

## Placement consistency hardening (later 18 November 2026)

The initial graphical bridge compared only the **set of placement IDs** with the source snapshot. That was insufficient: a backend that returned the same IDs with a different placement type or coordinates would have been accepted on startup, despite the terrain cross-check. The bridge now builds a source mapping of stable ID to placement kind and position and requires exact equality with the authoritative C++ listing at open time. Duplicate IDs in the source mapping are also rejected.

After each command, before accepting the new viewport or appending replay history, the bridge additionally rejects missing/extra IDs, changed placement kinds, out-of-bounds positions, and moved NPC/monster spawn points. Current graphical operations are bounded to moving resources/workstations and painting ground; actor spawn editing remains unsupported. A rejected response leaves the editor's displayed positions, terrain and local command list untouched. No live game, save format, source file, or publication policy was modified.

Four checked-in mocked-backend regression cases exercise initial coordinate drift, initial kind drift, post-paint read-only spawn drift, and post-paint out-of-bounds coordinates. During this update a separate **isolated** Python 3 script ran **9 parity-contract cases** and **4 mock output mutation snippets** successfully. That script reproduced the new validation logic and fake snippets; it was **not the full checked-in bridge test suite**. All new and previously committed Python tests, the real C++ command session, the complete C++ build and a real Tk interaction still require execution from a complete checkout.

**Next verification:**
```sh
cmake -S . -B build-headless -G Ninja -DBUILD_TESTING=ON -DGAMEENGINE_BUILD_CLIENT=OFF
cmake --build build-headless --parallel 4
ctest --test-dir build-headless --output-on-failure -R StarterWorld
python3 -m unittest discover -s tests -p test_world_editor_bridge.py -v
python3 tools/embervale_world_editor.py tests/fixtures/content/development-starter.snapshot.json --tool build-headless/EmbervaleWorldMove
```

The execution container still cannot resolve `github.com` for a complete checkout. This stacked pull request must remain draft until the actual C++/Python combination passes.

# Sprite resource validation and measured memory costs

**Date:** 20 November 2026 · **Task:** STUDIO-063 complete for the current eight-batch art contract.\
**Published code:** [4d7d01cb16ce5025687d2f1e0f2bb87513475f58](https://github.com/Adzffs/GameEngine/commit/4d7d01cb16ce5025687d2f1e0f2bb87513475f58).\
**Starting master:** [b7b06368709bd8833ddd1293628ce900dec6e5a0](https://github.com/Adzffs/GameEngine/commit/b7b06368709bd8833ddd1293628ce900dec6e5a0).\
**Tested code/art-report tree:** `5321148492900fe0878af080c6743bbd1d8f2697`.\
**References:** [resource contract and usage](../05-game/ASSET_RESOURCE_LIMITS.md) · [per-texture costs/checksums](art-resources/2026-11-20/memory-report.json) · [verification results](art-resources/2026-11-20/results.json).

## Delivered behavior

The shared cache now checks catalogue geometry, identities, binding completeness and memory budgets before opening images. It verifies each bounded PNG's structure, CRCs and compressed pixel stream before SDL decoding, then passes the same owned bytes to SDL. Bad catalogue metadata uses basic shapes; invalid/missing optional PNGs fall back individually. Reinitialization clears old frame bindings, texture allocations and transparency caches.

The new read-only checker validates all eight recorded manifests, source/output checksums and exact generated geometry, then reports costs. Pilot exports run the same metadata/file checks before replacing the generated binding. When Python is available, CMake runs the checker before its single copy owner; a rejected check prevents copying over previous packaged outputs. Existing no-relink repair is retained. All 16 code/guide/report blobs and their resulting tree were compared with local staged bytes before publishing.

The tracked PNGs, `.ase` masters, manifests, previews and `ForestPilotLayout.h` remain unchanged. The only added asset-side file is the numeric resource policy. This task does not integrate the unavailable Windows melee/equipment/polish work or change pose/timing artwork. REL-004 and owner-deferred ART-005 retain their prior status.

## Inspection findings and corrections

| Finding | Evidence/response |
|---|---|
| The old runtime bounded file size/dimensions but checked a path before reopening it in SDL | Load bounded owned bytes and decode that exact buffer; eliminate the header/reopen gap |
| A small valid header does not establish a bounded complete pixel stream | Stream zlib inflation through 64 KiB, cap output at declared scanline bytes, verify filters/end and reject extra/truncated streams before SDL |
| No aggregate texture/CPU transparency/staging policy existed | One versioned numeric policy shared by CMake-generated C++ defaults and Python checks |
| Export rectangle checks did not cover complete runtime binding contracts | Reject malformed references, types, IDs, paths, geometry, duplicate slots and incomplete present action/variation tables |
| A cache reset could retain capacity/frame references | Release image state and reset all frame bindings/accounting during Clear/reinitialization; exercise valid → invalid → valid paths |
| Valid source copying needed an explicit validation gate | Keep one copy owner; add its read-only dependency before writes; prove source refusal preserves previous outputs |
| Compressed PNG size obscures actual pixel storage | Record PNG bytes, four-byte RGBA texture estimate, one-byte alpha cache and single-image staging separately |

The findings describe inspected boundaries and reproduced new rejection/fallback cases. No old-version out-of-memory crash, hostile-image security guarantee or physical GPU-memory measurement is claimed. SDL's own [PNG API documentation](https://wiki.libsdl.org/SDL3/SDL_LoadPNG_IO) scopes its convenience loader to trusted images. The current narrow repository export format remains the input boundary.

## Current memory accounting

| Component | Bytes | Approximate MiB |
|---|---:|---:|
| All 30 encoded PNGs | 281,049 | 0.27 |
| Logical RGBA8 texture pixels | 11,681,792 | 11.14 |
| Retained per-pixel alpha | 2,920,448 | 2.79 |
| Combined logical retained pixels | 14,602,240 | 13.93 |
| Largest single-image staging estimate | 2,980,364 | 2.84 |
| Final pixel cache plus largest staging estimate | 17,582,604 | 16.77 |

Staging is encoded image bytes plus the larger of decoded RGBA pixels or the fixed 64-KiB preflight scratch buffer; validation and SDL decoding happen sequentially. Final-cache-plus-staging is a conservative logical accounting estimate. Actual process RAM/VRAM, allocator capacity/overhead, zlib/SDL internals, driver padding, render targets and simulation memory are excluded. Each 384×1,920 axe/pickaxe sheet alone has 2,949,120 RGBA bytes; its compressed size is much smaller.

The [version-1 policy](../../../assets/embervale/world-art-limits.json) permits at most 64 images, 16,384 frames, a 4,096-pixel edge, 4,194,304 pixels per image, 4-MiB PNGs, 32-MiB encoded catalogue, 64-MiB RGBA texture estimate, 16-MiB alpha and 20-MiB image staging. Both edge and pixel caps apply. Manifests are bounded to 2 MiB. Current image/metadata costs remain comfortably within these explicit limits; these are guardrails for the current pipeline, not a production performance target for every future art style.

## Executed verification

Linux GCC 14.2.0, CMake 4.4.5, Ninja 1.13.2, audio-enabled SDL 3.4.12, **zlib 1.3.1**, Python 3.12.14 and system Python 3.13.5.

| Check | Result | Meaning |
|---|---:|---|
| Full SDL-configured CTest | 120/120 | 98 headless, 9 client, 13 graphics; dummy video/software/audio |
| Separate client/graphics-disabled CTest | 98/98 | Core remains SDL-free and now links zlib |
| Python editor/authoring units | 79/79 per interpreter | 15 new resource cases plus all 64 existing cases |
| Resource C++ core | 46 assertions | Counts, dimensions, geometry, IDs/paths, binding slots/completeness, budgets, PNG CRC/stream/filter limits and regular-file behavior |
| Real SDL resource/cache test | 12 assertions | Full 30-image decode, known pixel/alpha costs, invalid metadata before I/O, corrupt/missing fallback, limits and reset |
| Read-only asset check | Pass | Eight recorded batch versions, 30 PNG/source hashes, 1,175 frames and exact generated header |
| Private packaging integration | Pass | Bad source rejected before copy; old outputs preserved; corrupt/deleted outputs repaired; all 30 textures/15 sounds match in spaced Debug and Ninja Multi-Config Release paths |
| Documentation checker | Pass | 104 Markdown files, 1,352 links, 96 synchronized tasks, acyclic dependencies and 45 retained legacy Studio IDs; external URLs are not fetched by this checker |

The packaging proof builds only the art target; the Multi-Config Release result is a copy/validation test, not a full optimized Release executable test. It copies source/fixtures into a private temporary tree and verifies the real repository assets remain intact. Python tests additionally assert that the read-only full check preserves all art files and the generated binding, and that a malformed export candidate leaves its previous generated header unchanged.

This gate is not a multi-file transaction or a lock against concurrent asset exports. Keep source-art/header generation separate from compilation/copying. The runtime independently validates the exact owned PNG bytes it decodes.

Compressed-stream fixtures include a 1×1 header hiding 100,000 decoded bytes, short data, invalid row filters, duplicate compressed streams, and an image crossing 64-KiB output and one-byte IDAT input boundaries. These cases reject or pass under the explicit contract; they do not require allocating an enormous pixel image to test the limit.

Existing publication/recovery/starter/persistence/audio/render tests remain included. The checked-in older River Crossing manifest still passes current validation; its recorded original validator revision is retained rather than rewriting an immutable version.

Reproduce on the configured build/toolchain:

```bash
cmake --build build-client -j4
cmake --build build-headless -j4
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDL_AUDIODRIVER=dummy ctest --test-dir build-client -j4 --output-on-failure
ctest --test-dir build-headless -j4 --output-on-failure
python -m unittest discover -s tests -p 'test_*.py'
python tools/art/check_world_art.py --check
```

The resource validator uses zlib's streaming API and CMake `ZLIB::ZLIB`. [Setup](GETTING_STARTED.md) now includes that dependency, including the official [MSYS2 UCRT64 package](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-zlib). The [upstream zlib manual](https://zlib.net/manual.html) was consulted; its current documentation/package version is independent of the tested installed 1.3.1 library. A dynamically linked runtime must be available in the selected toolchain path when applicable.

## Acceptance and remaining work

STUDIO-063 meets its current acceptance: reject invalid image dimensions, rectangles/bindings/frames and excessive logical resource usage, and record costs. Its STUDIO-060 prerequisite supplies the current named/checksummed batches; wider provenance/licensing/importer scope remains in progress. STUDIO-061/062 keep broader rendering/Studio workspace acceptance. REL-002 still owns clean installation/distribution and runtime dependency packaging; QA-003 retains broader usability/accessibility.

The source format is currently RGBA8 non-interlaced PNG with only IHDR/contiguous IDAT/IEND chunks. Other formats/metadata and general third-party importing require explicit extension. Current action tables have four walking and six chopping/Mining poses, with eight facings and three tool tiers. Future pose-count changes must update both validators and runtime arrays together, preserving the unavailable animation integration's separate status.

No current Windows/native GPU execution, actual VRAM/process profiling, complete installer, full uncoached starter acceptance or new animation visual acceptance is claimed. Player save format 13, publication formats, gameplay authority, audio routing and source-art provenance remain unchanged.

Implementation: [C++ resource preflight](../../../src/Content/Assets/WorldArtResources.cpp), [runtime cache](../../../src/Graphics/ForestSprites.cpp), [read-only check](../../../tools/art/check_world_art.py), [Python validation/export binding](../../../tools/art/world_art_resources.py), [core cases](../../../tests/WorldArtResourcesTests.cpp), [real SDL cases](../../../tests/WorldArtResourceGraphicsTests.cpp), [package cases](../../../tests/WorldArtPackagingIntegration.py), [Python cases](../../../tests/test_world_art_resources.py).

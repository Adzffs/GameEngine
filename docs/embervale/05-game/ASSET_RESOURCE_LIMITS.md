# World-art validation and memory limits

**Implemented:** 20 November 2026 · **Task:** STUDIO-063, current eight-batch world-art scope.

The exporter, read-only asset check and game cache now enforce the current sprite contract and explicit resource budgets. Invalid catalogue metadata falls back to basic shapes before loading images. An invalid/missing optional image falls back individually. Existing artwork, animation timing, editable masters and generated frame geometry are preserved.

## Check the current art

From the repository root:

```bash
python tools/art/check_world_art.py --check
python tools/art/check_world_art.py --check --summary
```

The default output is a JSON memory/resource report. `--summary` prints one build message. `--root` selects a complete copied authoring tree. These commands are read-only; they do not export, repair images, regenerate bindings or rewrite manifests. A rejected check exits `1` with a diagnostic; success exits `0`.

When a working Python interpreter is available, CMake runs this check before its single **GameEngineWorldArt** copy owner. A failed source check leaves previous packaged files intact. The copy owner still repairs corrupted/deleted build outputs without relinking C++. Without Python, the offline build gate is absent; the C++ runtime guards remain active. Explicitly select a working interpreter with `-DPython3_EXECUTABLE=...` when needed.

The pilot builders validate the complete catalogue before replacing `ForestPilotLayout.h`. Invalid candidates preserve the previous generated binding. Exporting art through LibreSprite is a separate mutation and remains intentional. The read-only checker verifies the generated header matches the current validated manifests byte-for-byte.

## Recorded costs

The unchanged published set contains **8 batches, 30 textures and 1,175 frames**. The [full per-texture report](../06-development/art-resources/2026-11-20/memory-report.json) records image sizes and source/output checksums.

| Resource | Current logical cost | Default limit |
|---|---:|---:|
| Encoded PNG files | 281,049 bytes | 32 MiB per catalogue; 4 MiB per file |
| RGBA8 texture estimate | 11,681,792 bytes, about 11.14 MiB | 64 MiB per catalogue |
| Retained transparency cache | 2,920,448 bytes, about 2.79 MiB | 16 MiB per catalogue |
| Texture plus transparency pixels | 14,602,240 bytes, about 13.93 MiB | 80 MiB combined |
| Largest single-image staging estimate | 2,980,364 bytes, about 2.84 MiB | 20 MiB |
| Final pixel cache plus largest staging estimate | 17,582,604 bytes, about 16.77 MiB | 100 MiB combined logical budget |

Texture accounting uses four bytes per pixel. CPU transparency uses one byte per pixel. Initialization handles one image at a time: staging includes its owned PNG bytes plus the larger of its decoded RGBA pixels or the 64-KiB streaming-validation buffer. The last row is a conservative final-cache-plus-staging estimate, not a measured peak of the process.

These values exclude allocator capacity/overhead, zlib/SDL decoder internals, driver allocations/alignment, renderer targets, metadata containers and all other game systems. They do not measure actual VRAM or process RAM. Compressed file size alone is not a texture-memory estimate. The axe and pickaxe sheets each require 2,949,120 logical RGBA bytes despite their small compressed files.

## Resource policy

[world-art-limits.json](../../../assets/embervale/world-art-limits.json) is the single numeric policy. CMake validates its version/positive integer fields and compiles them into the SDL-free validator. Python reads the same file.

| Limit | Default |
|---|---:|
| Images | 64 |
| Frames | 16,384 |
| Image width or height | 4,096 pixels |
| Pixels per image | 4,194,304 |
| Encoded PNG | 4 MiB |
| Encoded catalogue PNGs | 32 MiB |
| Manifest | 2 MiB per batch |
| Logical texture / transparency / staging | 64 / 16 / 20 MiB |

Both the edge and pixel caps apply: a 4,096×4,096 image fails the pixel budget. A renderer reporting a lower maximum texture size also rejects unsupported images before decoding. The current runtime cache is sized from the generated image catalogue; increasing a numeric policy does not add images or animation slots by itself.

## Validation boundaries

Metadata checks cover positive integer dimensions, count/aggregate budgets, relative texture/source paths, duplicate IDs/texture paths, missing image references, finite integer atlas rectangles, pivots inside frames, ground points inside the 32-pixel tile and selector ranges. Duplicate runtime binding slots or incomplete present action/variation tables reject rather than silently overwriting or losing frames.

The current action contract is eight facings, four walking poses, six chopping/Mining poses and three tool tiers. Variation table sizes match the current runtime arrays. A future authored pose-count change must update both validator bindings and runtime arrays with its tests; it must not bypass a failed guard. The unavailable eight-pose local animation prototype is not integrated by this task.

The current PNG export contract is deliberately narrow: **RGBA8, non-interlaced, IHDR → contiguous IDAT → IEND**, with no additional chunks or trailing bytes. Bounds, CRCs, dimensions, complete compressed-stream length and PNG row filters are verified. Inflation streams through a fixed 64-KiB buffer and stops if it exceeds the declared dimensions. Broken/truncated streams, an extra compressed stream and decompression beyond the declared image reject before SDL pixel decoding.

The runtime reads a bounded regular, non-symlink file into owned bytes and decodes those same bytes with SDL_IOFromConstMem/SDL_LoadPNG_IO. It does not reopen a path after checking its header. The offline check additionally compares PNG and editable-master hashes against the authoring manifests. It supports the currently recorded batch versions: building/variation v2 and the other six v1. It verifies checksums of `.ase` masters, not their internal editor format.

Changing an image into a different valid PNG of the same geometry is not runtime authentication; the offline manifest checks catch changed source exports. General third-party asset import, metadata chunks, other PNG formats and a hardened untrusted-image decoder require their own reviewed scope. SDL describes its PNG convenience loader as intended for trusted images; these guards support the controlled repository pipeline.

## Build dependency and verification

The C++ preflight uses **zlib**, discovered by CMake as `ZLIB::ZLIB`; it remains SDL-free. Install its development headers/library for the chosen toolchain. For MSYS2 UCRT64:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-zlib
```

Keep a dynamically linked zlib runtime available through the same toolchain/runtime path when applicable. The current Linux run uses zlib **1.3.1**; documentation/package lookup does not establish execution on another version or on Windows. See [Getting started](../06-development/GETTING_STARTED.md).

The executed Linux suites pass 120/120 SDL-configured entries, 98/98 separate headless entries and 79/79 Python units per interpreter. Tests cover finite/bounds/identity/selector/budget failures, compressed-stream expansion, one-byte IDAT boundaries, real SDL cache costs and fallback, unchanged authoring files, rejected package sources, preserved outputs, repair, spaced paths and Ninja Multi-Config Release. The broader accessibility/native GPU/Windows and starter-playthrough acceptance remains open.

Primary references consulted: [SDL_LoadPNG_IO and trusted-image scope](https://wiki.libsdl.org/SDL3/SDL_LoadPNG_IO), [zlib streaming API manual](https://zlib.net/manual.html), [official MSYS2 UCRT64 package](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-zlib). The installed SDK headers and tested library versions remain the implementation baseline.

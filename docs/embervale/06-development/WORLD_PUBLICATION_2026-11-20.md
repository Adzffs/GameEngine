# Fixed world publication and selected-version launch

**Date:** 20 November 2026 · **Task:** STUDIO-026 complete for the bounded starter contract.\
**Published code:** [d44c3cf061aed70a92b1f38adee3ca423ac72e9d](https://github.com/Adzffs/GameEngine/commit/d44c3cf061aed70a92b1f38adee3ca423ac72e9d).\
**Starting master:** [ff295ae467b4bd1fbce69b8fe47ef9b6aaab467d](https://github.com/Adzffs/GameEngine/commit/ff295ae467b4bd1fbce69b8fe47ef9b6aaab467d).\
**Tested code/package tree:** `008a5ae709903ad1f6cfe54d81d38dee9c6e2a87`.\
**Evidence:** [machine-readable results](world-publication/2026-11-20/results.json) · [usage and contract](../04-studio/WORLD_PUBLICATION.md).

## Result

Studio now has **Versions…**: validate the saved draft, publish it under a fresh name, inspect a selected version and start it in an isolated temporary game process. Earlier versions remain available. Selecting an older manifest changes the map used by the next launch. The CLI exposes the same C++ validation/publication/load contract. The checked-in [River Crossing v1](../../../content/published/README.md) is a fixed copy of the existing authored candidate.

The code commit's 26 published file blobs and resulting tree were compared against local staged bytes. Follow-up status/roadmap documentation pins that commit. No original Windows animation worktree was recovered or integrated; REL-004 remains open and ART-005 polish remains owner-deferred.

## Findings and implementation decisions

| Finding | Implemented response |
|---|---|
| A validated draft filename can later contain different bytes | Receipt binds raw-source SHA-256, canonical-world SHA-256, schema, profile and generated validation-input revision; publishing revalidates/rechecks |
| A two-file package can be interrupted after only its first file | Exclusive name reservation; complete canonical payload first; complete manifest committed last; partial directories remain unselectable |
| Retrying an uncertain publication can overwrite or assume ownership of leftovers | Fresh names only; inspect the manifest after uncertainty; incomplete/occupied names are refused and retained |
| An old report or edited manifest must not bypass current gameplay safety | Runtime checks hash/layout/canonical bytes and revalidates current supported placement, spawn, doorway and access rules before Engine/player loading |
| A rollback selector can accidentally imply player rollback or mutate a running world | Explicit manifest per fresh process; no global mutable current pointer, hot reload or player-state rollback |
| A selected persistent save can overwrite published content | Check primary/tmp/bak against the whole library and selected-file aliases; Studio always uses --no-save in private cwd |
| The initial full graphics run had no usable default display driver | Configure dummy video/software rendering/dummy audio for the automated suite; all 116 then pass |
| Native Studio publishing works, but this SDL build lacks X11/Wayland drivers | Record native controls separately from dummy-SDL game starts; the native game failure is reported by Studio, not treated as rendering acceptance |

The shared bounded JSON reader was extracted from the existing snapshot parser for strict metadata parsing; its former limits remain in force and existing parser tests pass. Publication reuses the existing staged exclusive writer and cooperative source lock. Ordinary draft/recovery output rules are retained. CMake fingerprints core sources, source headers, its build definition/template and platform/compiler ID/version; this is a recorded validation-input fingerprint, not a binary signature or complete compiler-flag/build provenance.

Old published validator revisions can load only under the same supported profile/schema after current revalidation. New publication receipts require the current validator revision. This allows compatible old maps to remain useful without allowing stale receipts to publish changed drafts.

## Executed verification

Linux GCC 14.2.0, CMake 4.4.5, Ninja 1.13.2, audio-enabled SDL 3.4.12, Python 3.12.14 and system Python 3.13.5/Tk 8.6.

| Check | Result | Coverage/limit |
|---|---:|---|
| Full SDL-configured CTest | 116/116 | 96 headless, 8 client, 12 graphics; dummy video/software/audio |
| Separate client/graphics-disabled CTest | 96/96 | SDL-free core and real content tools |
| Python editor units | 64/64 on each interpreter | 11 new publication cases; 53 existing editor/recovery cases retained |
| Publication C++ core | 31 assertions | Receipt/manifest, different v1/v2 Worlds, fresh spawn, rollback, malformed/tampered/current-rule rejection |
| Launch options | 34 assertions | Existing source/save behavior plus version selection/conflicts and library isolation |
| Real publisher/editor integration | 10 cases in one CTest entry | Saved edit/reopen, stale/raw bytes, changed validator, routes, collisions, forged hash, interruption and competing publisher |
| Killed publication children | 4 per integration run | Reserved, world-written, committed boundaries plus competing-name owner death |
| Real game process integration | One CTest entry | v1/v2/v1 fresh starts, changed spawn, exact selected hash output, poisoned unused save fixtures, tamper rejection before load and library save refusal |
| Existing bridge/recovery integration | Retained in full suites | Twelve-case bridge and ten-case recovery suites each remain one entry |
| Production core archive | No crash-pause symbols | Publication and existing draft pause hooks excluded from runtime/core |
| Documentation checker | Pass | 102 Markdown files, 1,307 links, 96 synchronized tasks, acyclic dependencies and 45 retained legacy Studio IDs; external URLs not fetched |

The unconfigured SDL run failed seven graphics entries with `No available video device`; those failures are recorded as environment setup, and the complete configured rerun passes. No production behavior was changed to force those tests through. The runtime test script waits for the real fresh-player initialization marker and confirms the process remains alive before stopping its temporary session; it compares source, versions and deliberately invalid normal-save fixtures after every launch.

Reproduce from the appropriate configured build directories:

```bash
cmake --build build-client -j4
cmake --build build-headless -j4
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDL_AUDIODRIVER=dummy ctest --test-dir build-client -j4 --output-on-failure
ctest --test-dir build-headless -j4 --output-on-failure
python -m unittest discover -s tests -p 'test_*.py'
ctest --test-dir build-client -R '^(WorldPublication.*|PublishedWorldClientLaunchTests|GameLaunchOptionsTests)$' --output-on-failure
```

The publication crash probe compiles its own hook-enabled publication translation unit; the normal core/tool does not contain it. Fixtures use private temporary directories, explicit temporary player modes and synthetic drafts. No normal player save is used.

## Native desktop observation

A private River Crossing copy was opened in real Linux/Tk Studio. **Versions… → Validate draft → Publish version** produced `village-draft-v1`; repeating that name displayed the reserved/published-name refusal. After validation, publishing `village-draft-v2` preserved both list entries. Selecting `village-draft-v1` verified its fingerprint and enabled play. Both native names deliberately contain the same saved map; automated coverage separately uses distinct map/spawn versions.

The older-version Play action started the real game and Studio then reported exit code 1. A direct diagnosis verifies selected-manifest/hash loading before SDL reports `No available video device`; this SDL build has X11 and Wayland disabled. The native result establishes controls, publication, collision feedback and selection, plus observable launch failure. It does not establish native gameplay/rendering. Three actual dummy-SDL v1/v2/v1 starts pass separately. The launch status text was adjusted to say **Starting** when spawning, avoiding premature success wording.

Original/private draft SHA-256 and both native published world hashes remain `72c9d565ad1db11fc4e27eb8e512ef0dd23b5a902ef18da6a486a3c2ed28297f`. The checked-in River Crossing v1 has the same hash. Its validator revision is `4f7a4cc5ab2b7e315c957396d16c11c23f9bce5031d667dbfd2c0681f2e316dd`; client and headless publisher receipts agree.

## Scope and follow-up

STUDIO-026 meets its current acceptance: version-bound exact validation, immutable-by-application manifest/payload creation and explicit selected-version loading in a fresh session. Its bounded prerequisite slices already provide current starter placement/access/write safety. STUDIO-021 remains in progress for wider in-place replacement/durability; STUDIO-050/051 retain general regions/exits and richer diagnostics. STUDIO-032 and QA-003 retain broader keyboard/focus/scale and platform acceptance. R2/Alpha 0.1 is still partial.

Player save format stays 13; receipt/manifest formats are independently 1. The current 100×100/ground schemas 1–4 remain the content boundary. Application create-only behavior and checksums do not make writable files tamper-proof or authenticate their source. No power-loss/media/network-filesystem guarantee, world/RNG snapshot, player-progress rollback, general asset manifest, binary freeze, online deployment or current Windows execution is claimed. Native/Windows game acceptance and wider packaging/usability remain on the roadmap.

Implementation: [publication core](../../../src/Content/Publication/WorldPublication.cpp), [CLI](../../../src/Tools/WorldPublishMain.cpp), [Studio client](../../../tools/embervale_publication.py), [Studio dialog](../../../tools/embervale_publication_ui.py), [runtime launch](../../../src/Core/GameLaunchOptions.cpp), [core tests](../../../tests/WorldPublicationTests.cpp), [real-tool interruption tests](../../../tests/WorldPublicationIntegration.py), [real game process tests](../../../tests/PublishedWorldClientLaunchTests.py).

# Windows usability verification

**Date:** 20 November 2026.\
**Verified code:** [4454067](https://github.com/Adzffs/GameEngine/commit/44540678bfa682870f2c9efb8907a1d36891910a).\
**PR:** [#46](https://github.com/Adzffs/GameEngine/pull/46).\
**Work package:** QA-003 remains in progress for broader accessibility and human acceptance.

The recovered candidate now builds and passes its complete Windows suites. This
supersedes the pending-build status in the [recovery handoff](USABILITY_REVIEW_2026-11-20.md).
Save format 13, server authority, 600-ms ticks, content, artwork and generated
geometry are unchanged. UI text scaling remains a separate follow-up.

## Reproduced findings and fixes

| Finding | Result |
|---|---|
| Mouse Close discarded queued crafting interaction/grid work | Preserve the queued intent; authoritative sessions also guard further clicks until consumption |
| Bank Escape replaced an unconsumed transfer with BankClose | Preserve the exact deposit/withdraw request; close is available after it reaches the engine |
| CRLF checkout rejected an unchanged generated art binding | Canonicalize only CRLF spelling for text comparison; changed geometry, lone CR and BOM still reject; binary hashes/budgets remain exact |
| Windows recovery journal source comparison used case-sensitive strings | Compare Path identities with host filesystem semantics while retaining the existing source key and integrity checks |
| A test read an actively byte-locked owner file on Windows | Preserve the content check for journals and check lock identity/metadata without reading its protected range |
| Autosave fixture rewrote backup bytes in Windows text mode | Write the fixture in binary mode; retain exact no-erasure/no-rewrite assertions |
| Isolated packaging configure omitted the parent's dependency/toolchain selection | Inherit compiler and prefix path; retain invalid-source refusal, preserved output and no-relink repair |
| Publication test expected a noncanonical executable spelling | Assert the actual resolved executable path; retain exact temporary/no-save arguments and isolation assertions |
| Recovered backlog acceptance text disagreed with its JSON contract | Restore matching acceptance; retain task progress/findings in notes and work order |

Two mouse-close regressions failed before correction. The new production bank
integration also failed before correction when Escape replaced its queued deposit.
These were fixed before the final full test run.

## Executed verification

Windows UCRT64 GCC 16.1.0, CMake 4.4.3, Ninja 1.13.2, SDL 3.4.12, zlib 1.3.2,
and Python 3.14.6. Client rendering uses dummy video/software/audio for repeatable
automated input/render checks; this is not physical GPU/audio or manual UI acceptance.

| Check | Result |
|---|---:|
| Complete Debug client configuration | 122/122 CTest entries: 98 headless, 10 client, 14 graphics |
| Separate client/graphics-disabled configuration | 98/98 CTest entries |
| Python authoring/editor suite | 80 run: 78 passed, two unavailable-symlink skips |
| Usability input and queued-close cases | 22 assertions |
| Production bank round trip | 12 assertions |
| Documentation consistency before new report | 106 Markdown files, 1366 links, 96 matching tasks; acyclic; 45 legacy IDs retained |

The bank test opens a real World session with a fresh character, sends SDL mouse
and keyboard events through Graphics, consumes requests with Engine, and waits
for committed World ticks. It verifies deposit/withdraw ownership, literal h and
Backspace filtering, no withdrawal from hidden rows, queued-transfer preservation
and authoritative closure. It resolves the interrupted functional withdrawal
recheck without claiming a new manually observed Windows playthrough.

Packaging tests use owned private copies, reject broken source before copying,
preserve previous packaged output, repair deleted/corrupt output without C++
compilation, and exercise spaced paths plus Ninja Multi-Config Release. Both
.o and Windows .obj files are checked to keep this proof art-only.

The earlier Linux native Return/Exit/retry and warning-layout observations remain
in the original report. Ordinary Windows focus/Alt-Tab and manual native save
choices remain broader QA-003 acceptance; so do full keyboard navigation,
scalable text, high-DPI, colour-independent feedback and an uncoached full journey.
The existing starter/combat/recovery replay remains included in the passing suites.

## Original animation integration recovered

The separate local `work/GameEngine-next` checkout was found again on Windows:
branch `feat/integrated-combat-audio`, head `ce3160a62a4c14ffb2d2907ba6fb5f9748b7524a`,
with unresolved index entries and uncommitted integration edits. Its original
animation checkout remains on `polish/player-animation` at `196a279`.

A create-only local recovery checkpoint at `work/rel004-checkpoint-20261120`
contains 79 changed/untracked working paths, 94 distinct index-stage blobs,
index/status records, staged/working patches and a 9,342,301-byte history bundle.
Index blob hashes were checked while copying. The original worktree was not
modified. This checkpoint is a recovery artifact, not a verified integrated
build or published animation acceptance. Read the [integration handoff](INTEGRATION_HANDOFF_2026-11-20.md)
and inspect the existing work before applying any old delta again. REL-004
remains in progress and ART-005 remains owner-deferred.

## Reproduce

Use the UCRT64 compiler/runtime path, installed SDL/zlib and an actual Python
interpreter as described in [getting started](GETTING_STARTED.md). With configured
builds, run:

```powershell
cmake --build build-client --parallel 4
$env:SDL_VIDEODRIVER='dummy'; $env:SDL_RENDER_DRIVER='software'; $env:SDL_AUDIODRIVER='dummy'
ctest --test-dir build-client --parallel 3 --output-on-failure
cmake --build build-headless --parallel 2
ctest --test-dir build-headless --parallel 3 --output-on-failure
python -m unittest discover -s tests -p 'test_*.py'
python tools/check_documentation.py
```

Use Program Files CTest where the MSYS2 copy is blocked by local execution policy.
All file-failure experiments must stay on disposable profiles. Normal player saves
and the unresolved animation integration must remain separate from these tests.

# River Crossing usability review and recovery handoff

**Date:** 20 November 2026.  
**Work package:** QA-003, in progress.  
**Status:** Recovered draft candidate; final build/verification and merge remain open.  
**Candidate code:** [f9c45e0](https://github.com/Adzffs/GameEngine/commit/f9c45e0a1e8ad744d9859c15ca32a11a60908a95).  
**Base:** [b0ab668](https://github.com/Adzffs/GameEngine/commit/b0ab668e0b4d09899c135e08c8211456d5f999e7).  
**Branch:** [feat/river-crossing-usability-recovery](https://github.com/Adzffs/GameEngine/tree/feat/river-crossing-usability-recovery).

The focused pass reproduced input defects, implemented fixes, ran automated
checks and exercised the native Linux game. Before final publication the selected
execution environment failed and its filesystem became unreachable. The recorded
patches have been reconstructed on GitHub. **These candidate changes are not yet
merged into master.** The existing verified-master pin remains unchanged.

## Findings and candidate changes

| Reproduced problem | Implemented candidate behavior |
|---|---|
| Escape ignored help and dialogue | Help closes first; active dialogue queues a server close; terminal text dismisses locally |
| R changed running behind help | Help consumes background gameplay keys; Enter/Space/Escape/H dismiss it |
| Typing h in bank search opened help | Bank owns text keys; h and Backspace work; a search hint is shown |
| Escape abandoned crafting requests | Queued and in-flight crafting/recipe work blocks Escape closure until reconciliation |
| Focus loss retained a drag/context menu | Uncommitted gesture and menu cancel; later release cannot submit the old drag |
| An unrelated button release finished a drag | Only the initiating mouse button releases the gesture |
| Rapid dialogue actions could advance twice before a tick | One command stays in flight until its exact result or originating session/node changes |
| Catch-up could overwrite acknowledgement | Dialogue consumes results after every committed tick, including rejection |
| Dialogue had no keyboard selection | Tab/Shift-Tab or arrows select; Enter/Space activates; > marks focus; waiting text explains the delay |
| Right-click reused an older hover target | The click resolves against a read-only live World view, preserving ground/object/sprite priority |
| Save error overflowed the desktop and lacked its game parent | Native dialog belongs to the live window and uses explicit short lines; safe Return/Escape defaults remain |
| Save warning covered the first bank withdrawal slot | Warning moves above open bank/station/shop/help panels; a real-render pixel test protects the bank panel |

Dialogue body text wraps at words within its existing two-line area. This does
not implement general text pagination or scalable fonts. Escape also closes a
side panel; Ctrl+Q enters the existing guarded shutdown path. The candidate
[controls and recovery guide](../05-game/CONTROLS_AND_RECOVERY.md) documents scope.

## Checks observed before interruption

These results belong to the pre-interruption work, **not a newly executed test
run of the reconstructed GitHub commit**. No final reconstructed-tree build or
documentation-check success is claimed.

| Check | Observed result and boundary |
|---|---|
| Initial regression on unchanged game code | All nine original usability assertions failed |
| Earlier client candidate | 121/121 CTest entries passed: 98 headless, 9 client, 14 graphics |
| Separate SDL-free build | 98/98 CTest entries passed |
| Python authoring/editor units | 79/79 passed on each of Python 3.12.14 and 3.13.5 |
| Later focused usability cases | 20 assertions passed after the context/message revisions |
| Later production dialogue integration | 41 assertions passed, including fresh right-click, rapid Continue, rejection and catch-up |
| Final save-warning rendering | 6 assertions passed, including unchanged bank-panel pixels |
| Final full rebuild and suite after the last layout change | Started rebuild; final completion/results could not be recovered; rerun required |

The earlier full suite preceded the final context-menu, message-wrapping and
warning-position changes; the focused cases above ran afterwards. Native builds
used Linux GCC 14.2.0, CMake 4.4.5, Ninja 1.13.2, SDL 3.4.12 and zlib 1.3.1.
Python versions were 3.12.14 and system 3.13.5. The existing full suite covers
starter replay, crafting, shops, combat/recovery, persistence/autosave, editor
recovery, publication and asset resources. It does not establish human pacing.

## Native Linux observations

X11/software rendering at 1280x720 and dummy audio were used with private,
explicit saves. Official SDL and signed Debian packages supplied a private X11
SDK; the earlier console-only SDL build had no native video driver.

The checked-in publication `content/published/river-crossing-v1/manifest.json`
was revalidated and loaded. Its world SHA-256 was
`72c9d565ad1db11fc4e27eb8e512ef0dd23b5a902ef18da6a486a3c2ed28297f`.

| Exercise | Observed result |
|---|---|
| Fresh welcome and Mara approach | Escape returned to the world; Talk-to approached Mara |
| Quest keyboard selection | Arrows visibly selected Gathering Basics; Enter showed waiting feedback; quest became ACTIVE |
| Private save-parent failure | Replacing only an owned fixture directory with a blocking file produced Return/Exit choices |
| Safe defaults | Escape and Return resumed the game; clicking Return also resumed |
| Restore storage and retry | Autosave recovered; normal shutdown saved; next process restored the ACTIVE quest |
| Native error readability | Dialog changed from 1371x102 to 650x177; both buttons and recovery/loss text fit |
| Immediate right-click | Mara and bank actions resolved without waiting on a hover frame |
| Bank search | h appeared literally without help; Backspace cleared it |
| Bank transfer | Earlier deposit removed the axe from the bag and tutorial guidance requested withdrawal |
| Final warning layout | Warning visibly sat above the open bank, leaving item rows clear |
| Final withdrawal recheck | Interrupted after deposit input by environment loss; completion was not observed |
| Explicit unsaved exit | Process stopped with existing exit code 1; previous checkpoint bytes stayed unchanged |

The checkpoint retained through the explicit-exit exercise had SHA-256
`c2e49941d7fe450e5eeaa595adace828dd02b5662f0e996eb4f74258cd2d0898`.
Only disposable owned fixtures were blocked; ordinary player saves and published
world files were untouched. The final third session's closure was not observed.
If its old filesystem is recovered, inspect its private `qa-native` fixtures
before restoring the blocked parent; do not remove unrelated player files.

The virtual desktop lacked normal window-manager focus policy. Terminal/dialog
keys were routed to their observed windows where global focus was unavailable.
Ordinary desktop Alt-Tab/Windows focus, high-DPI, GPU and audible-device acceptance
remain open. These were assisted observations, not an uncoached full starter run.

## Source recovery and integrity

The old workspace was `/workspace/scratch/a8d2d93fb48f/GameEngine`, local branch
`feat/river-crossing-usability`. Environment
`ccarenv_b64_Y2NhcmVudl82YTljYWU2ODhlNTg4MTkxODA4ZjFkOTY0ZGNkZTExMA`
explicitly failed startup/readiness recovery. Shell access then reported no
usable execution environment. Raw final build/log files are not claimed recovered.

Recovery fetched the exact base files, verified their Git blob hashes, applied
the recorded replacements with unique-match checks, and independently serialized
Git trees to check that all unrelated files remain intact. Both recorded
intermediate source fingerprints match:

| Source | Recorded prefix | Reconstructed full Git blob SHA |
|---|---|---|
| Engine.cpp before the later live-World event argument | 167ca14 | 167ca14afe99d415fe0455ea0360e3cc45984e15 |
| Graphics.cpp before the later context/message/layout changes | 0d65006 | 0d6500618f4b93cab38a8a5bfe654a3374101c9b |

Final Graphics.cpp has the recorded 6859 lines; the usability test has 128.
The eight final candidate blob hashes are in the
[recovery manifest](usability/2026-11-20/recovery.json). Full byte comparison
against the inaccessible original final files and a fresh build remain open.
Preserve this published candidate before applying any further patches.

## Resume verification

1. Fetch the recovery branch and inspect its eight code/test changes against the base. The branch should also include the current documentation follow-up.
2. Configure the SDL client/graphics and separate SDL-free builds using [getting started](GETTING_STARTED.md), including zlib and the selected SDL SDK.
3. Build and run the full suites plus focused tests below; expected inventory is 121 client entries and 98 standalone headless entries.
4. Recheck keyboard/dialogue/rapid actions, bank h/Backspace/deposit/withdraw, focus-loss drag cancellation and the visible save-failure choices on disposable profiles. Recheck same-frame mouse-close/recipe replacement with queued crafting work; the candidate specifically adds Escape guards, and this broader edge needs acceptance.
5. Run the documentation checker; record exact source/configuration and results. Fix any regression before marking the draft ready or merging.

```sh
cmake --build build-client -j4
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDL_AUDIODRIVER=dummy ctest --test-dir build-client -j4 --output-on-failure
cmake --build build-headless -j4
ctest --test-dir build-headless -j4 --output-on-failure
python -m unittest discover -s tests -p 'test_*.py'
python tools/check_documentation.py
./build-client/GameEngine --world-version content/published/river-crossing-v1/manifest.json --profile fresh --save /path/to/private-qa-profile/progress.save
```

For native fault checks, wait for successful startup before blocking only the
owned disposable save parent. Retain its original directory, restore it before
successful retries, and compare checkpoint bytes around explicit unsaved exit.

## Remaining roadmap scope

QA-003 stays in progress: reconstructed-candidate verification/merge, interrupted
withdrawal recheck, wider keyboard access for inventory/equipment/crafting/shop/
bank selection, scalable UI text/longer layouts, colour-independent feedback,
ordinary focus/high-DPI and Windows acceptance. GAME-005/GAME-004 still need
uncoached navigation, gathering, crafting, food, combat and death/recovery
observation and real elapsed time. REL-004 still needs the separate original
animation source; ART-005 remains owner-deferred. Save format 13, authority,
600-ms ticks, art resources and published content are unchanged.

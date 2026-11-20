# Editor recovery and interrupted draft writes — 20 November 2026

**Task:** STUDIO-025, complete for bounded recovery of accepted editor work.\
**Published code:** [378092f](https://github.com/Adzffs/GameEngine/commit/378092feab53698e4078400d1a9cfc936c39f4c3).\
**Baseline:** 4dae1ccf23d20c49c6dc6eebca43e09eda404f5f.\
**Evidence:** [machine-readable results](editor-recovery/2026-11-20/results.json), [user/reuse guide](../04-studio/EDITOR_RECOVERY.md).

## Problem and resulting behaviour

Unfinished edits previously lived only in Python command/history memory. A forced termination lost them, and a killed C++ draft writer could leave an ownerless directory lock. The Windows path also created its final destination before finishing the write.

Studio now journals the exact original base plus accepted typed commands in a private, bounded per-window store. Recovery replays that base through the authoritative C++ editor, compares the resulting world fingerprint, previews actual terrain/object differences and exports a new validated JSON draft. It never merges into a changed original. Missing/corrupt originals and legacy directory locks do not prevent independent export.

The journal is a recovery draft. Its cached exact-byte input is the base revision. Existing authored/published files remain separate, and the recovered result is another draft. No STUDIO-026 publication manifest, validation receipt framework or automatic game-version selection is implemented.

## Findings and fixes

| Finding | Implemented correction |
|---|---|
| No durable record of accepted unsaved edits | Checkpoint original bytes, raw-byte digest, commands and authoritative state digest after accepted dirty edits |
| Recovery could otherwise replay against changed geography | Use a private cached base and show a changed-original warning; always select a fresh output name |
| Multiple editor windows could interfere | Separate session UUIDs, OS owner locks and explicit resolution |
| Partial/corrupted journals could look like a recovery candidate | Bounded checksummed envelopes, strict identity/command limits, ignored temporary writes and C++ replay verification |
| Directory locks outlive a killed writer | New stable advisory/exclusive OS lock files; process death releases ownership |
| Legacy locks have no trustworthy liveness evidence | Refuse rather than delete unknown legacy data; independent recovery export remains available |
| Windows destination was exposed during writing | Stage and close/flush the full file, then publish with no-overwrite MoveFileW |
| Startup recovery could stay behind another application on the preview desktop | Map/raise the parent and chooser before waiting for the restore choice |
| Failed normal journal commits could accumulate temporary files | Clean only the exclusively owned temporary stage on handled errors; retain prior generations |

## Policy and boundaries

The [recovery guide](../04-studio/EDITOR_RECOVERY.md) defines discovery, retention, explicit discard, source conflicts, storage limits and lock handling. Each normal session keeps three completed generations; a stop after commit but before pruning may leave four. Every journal carries a version, session/source identity, sequence, base digest, accepted commands, expected world fingerprint and UTC time. No arbitrary save/exit commands can enter replay history.

The original source is not needed for replay once its valid bytes are cached. C++ parses the private base, runs the original commands and validates the final snapshot. A matching replay fingerprint is required before export. The output receipt/hash, world parity and a fresh StudioBridge reopen are checked before resolving the old session. Recovered undo history starts at 0/0; authored allocation reservations survive.

Normal draft saving still requires a fresh sibling. Explicit recover-copy permits a selected new .json in another existing directory, using the same validation, exact-base recheck, create-only destination and staging. It cannot overwrite an existing file. The UI also refuses the recorded original path even if that original has disappeared.

The .embervale.writer.lock sidecar intentionally remains after release so cooperating processes share one lock identity. Unknown legacy .embervale.lock directories and interrupted world staging directories are retained. No age/PID rule takes ownership away from another editor.

## Executed automated checks

| Exercise | Observed result |
|---|---|
| Repeated unchanged redraws | No duplicate journal write |
| Two live windows on one map | Separate generations; live owners excluded from recovery/discard |
| Retention | Three normal generations preserved; previous generation survives an incomplete commit |
| Damaged checksum, invalid version/identity, oversized command history or injected save/newline command | No trusted recovery candidate; source bytes preserved |
| Failed replace and storage-budget rejection | Previous complete bytes retained; repaired storage can checkpoint latest state |
| Explicit resolution/collection | Only the selected closed session's known files removed; another session retained |
| Accepted edits with undo, allocation reservations, paint and spawn changes | Full authoritative replay matches; next authored ID remains reserved correctly after reopen |
| Changed original | Conflict detected; separate recovered copy matches checkpoint; changed source bytes unchanged |
| Missing/corrupt original | Independent cached-base recovery succeeds |
| Legacy lock with unknown contents | Ordinary write refuses; separate recovery succeeds and retains the lock contents |
| Occupied output | Destination bytes and recovery generations retained |
| Semantically invalid newest journal with valid envelope | C++ replay/fingerprint rejects it; selecting the earlier generation recovers correctly |
| Killed Python during second journal commit | First complete generation recovered; partial temporary file ignored |
| Killed Python after second checkpoint, with a later live edit | Second checkpoint restored; later uncheckpointed edit absent |
| Killed C++ after lock acquisition, during partial write, after staging, and after publication | No partial final file before commit; published file is complete; source/prior draft intact |
| Active C++ writer | Competing write refuses; production writer reuses the OS lock after the owner is killed |

The standalone C++ crash probe compiles the same draft-writer source with test-only pauses. Production GameEngineCore and editing/runtime executables contain no pause hook or environment fault switch. The production archive was checked for absence of the test symbol. Each integration run kills seven child processes at deterministic boundaries; it does not rely on racing a very fast write.

| Configuration | Passed | Scope |
|---|---:|---|
| Linux Debug SDL/client/graphics/content tools | 113/113 CTest entries | 94 headless, 7 client, 12 graphics |
| Separate Linux Debug without client/graphics | 94/94 CTest entries | SDL-free core/content tools |
| Python 3.12.14 editor units | 53/53 tests | Eleven new recovery integrity/ownership cases |
| System Python 3.13.5 editor units | 53/53 tests | Same suite on the second interpreter |
| StudioRecoveryIntegration | 10 cases | Included in one CTest entry above |
| StarterWorldDraftFileTests | 24 assertions | Included in CTest |
| Existing real editor bridge | 12 cases | Still included in its single CTest entry |

GCC 14.2.0, CMake 4.4.5, Ninja 1.13.2 and audio-enabled SDL 3.4.12 were used. SDL automated tests used dummy video/audio and software rendering. The previous autosave source's 112/93/42 counts are historical; this change adds one CTest entry and eleven Python unit cases.

## Native restore verification

A real desktop run used system Python 3.13.5/Tk 8.6 and the built production editing tool. Only disposable copies of the public starter fixture were used.

1. Recorded accepted stone paint and a new bank at (42,20), then closed that recovery owner.
2. Independently changed the original map's adjacent tile, leaving a different valid source revision.
3. Launched Studio. The chooser displayed both saved generations, the changed-original warning and the real change summary: one terrain tile and one added object.
4. Selected Restore, accepted a fresh filename in the real file picker, and observed village-draft-recovered.json opened in Studio with Saved, no unsaved edits and undo/redo 0/0.
5. Reopened both files through the real backend. The full recovered snapshot exactly matched a freshly reconstructed authoritative replay; the external original remained byte-identical. The restored bank kept station.authored.000001, and the old recovery session was resolved.

This verifies one native Linux recovery path and the visibility correction. It is not a complete uncoached editor/accessibility review, a Windows result or a starter-game playthrough. The filesystem hashes and native observations are recorded in results.json; no private map or player profile is used.

## Reproduce

~~~bash
cmake --build build-headless
ctest --test-dir build-headless --output-on-failure
cmake --build build-client
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDL_AUDIODRIVER=dummy \
  ctest --test-dir build-client --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py'
~~~

Python/content tools are needed to register StudioRecoveryIntegration. See [Getting started](GETTING_STARTED.md). The integration script is intentionally named separately from test_*.py because it requires the actual built C++ tool and crash probe.

## Reuse and remaining work

- [Recovery journal](../../../tools/embervale_recovery.py): private storage, bounds, checksums, ownership, replay and explicit new-copy restore.
- [Chooser](../../../tools/embervale_recovery_ui.py) and [editor](../../../tools/embervale_world_editor.py): visible discovery, change preview, status, save/discard and switch/close integration.
- [Writer](../../../src/Content/Edit/StarterWorldDraftFile.cpp) and [session](../../../src/Tools/WorldEditSession.cpp): OS locks, staged no-overwrite publication and validated recovery export.
- [Python integrity tests](../../../tests/test_editor_recovery.py), [real integration](../../../tests/StudioRecoveryIntegration.py) and [crash probe](../../../tests/StarterWorldDraftCrashProbe.cpp): preserve these behaviours during further editor work.

STUDIO-025 is complete for accepted, runtime-valid bounded editor work. STUDIO-021 remains in progress for broader in-place guarded replacement/durability; STUDIO-026 publication selection remains planned; STUDIO-032 and QA-003 retain broader platform/accessibility acceptance. Invalid unfinished gameplay drafts, portable relocation of recovery groups, network filesystems, physical-media/power-loss durability and current Windows execution are not established here.

The Windows implementation follows the official [CreateFileW sharing/creation contract](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew) and [MoveFileW fresh-destination rule](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefilew). These source/API checks do not replace a Windows build and runtime test.

Original Windows animation integration REL-004 remains unavailable, and ART-005 polish stays owner-deferred. Player save version 13 and the periodic player-autosave policy are unchanged.

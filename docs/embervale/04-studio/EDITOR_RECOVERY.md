# Recover unfinished map edits

**Implemented:** STUDIO-025 in [378092f](https://github.com/Adzffs/GameEngine/commit/378092feab53698e4078400d1a9cfc936c39f4c3), 20 November 2026. See the [verification report](../06-development/EDITOR_RECOVERY_2026-11-20.md).

Studio now keeps private recovery copies after accepted edits. These copies are separate from your map files and player progress. Normal Save new draft still creates a fresh sibling file.

## Restore a map

1. Open the same map in Studio. Closed recovery sessions are offered automatically.
2. Choose a copy. Studio validates it through the C++ editing tool and shows changed terrain/object counts and whether the original has changed.
3. Press **Restore selected copy…** and choose a fresh .json filename.
4. Studio opens the recovered draft. The original stays as it is, and the recovered draft starts with a fresh undo/redo history.

**Recovery…** opens the chooser again. **Keep copies for later** and Escape leave the copies available. **Discard session…** requires an explicit confirmation and discards only that closed session's recovery generations. Another open editor's session is never offered for recovery or discard.

If the original is missing, corrupt or blocked by an old save lock, the cached base can still be replayed and exported as a separate draft. If the original changed, Studio shows that conflict and never silently rebases edits onto it. An occupied destination is rejected. Choose a new filename rather than replacing your original.

The three recent completed generations are normally retained for each session. Older generations can be selected if a newer copy cannot be validated. Damaged/oversized/unsupported records and partial temporary writes are not trusted; if nothing is offered, use **Recovery…** to check diagnostics.

## Where copies live

| Platform | Default private recovery directory |
|---|---|
| Windows | LOCALAPPDATA/EmbervaleStudio/recovery |
| Linux/other Unix | XDG_STATE_HOME/EmbervaleStudio/recovery; otherwise ~/.local/state/EmbervaleStudio/recovery |

To select another real, writable directory:

~~~bash
python3 tools/embervale_world_editor.py content/worlds/river-crossing.blockout.json \
  --tool build-headless/EmbervaleWorldMove --recovery-dir /path/to/private-recovery
~~~

Use the same recovery directory when reopening that map. Each window gets a separate session; the source's resolved absolute path identifies its recovery group. Moving the original to a different path does not automatically retarget those copies. Reopen the recorded original path, even if the file has disappeared, to discover them.

Recovery stores complete original bytes, their SHA-256, accepted commands, a fingerprint of the authoritative resulting world, a format version and a UTC timestamp. It includes authored ID reservations and spawn settings. It does not persist player state, game RNG, undo stacks, arbitrary invalid edits or runtime actors.

## Failure and retention policy

| Condition | Result |
|---|---|
| Accepted dirty edit | Checkpoint its base and accepted command history; unchanged redraws do not rewrite it |
| Save or explicit discard | Resolve the current recovery session; remove only its owned generations |
| Close/switch cancelled | Keep editing and keep the recovery session |
| Process stops during journal writing | Ignore the uncommitted temporary file; retain previous complete generations |
| Three valid generations already exist | Commit the new one before pruning old copies; a crash before pruning can leave a fourth |
| Live owner | Skip the session using an OS lock, without guessing from age or PID |
| Storage failure | Keep the last complete copies and live edits; show a recovery-status error and allow Save new draft |
| Storage budget reached | Refuse another checkpoint and show the error; do not delete unrelated or unresolved sessions |
| Closed resolved session | Reclaim its known terminal metadata when a later session starts |
| Original revision differs | Preview the conflict; recovery can create a separate validated copy |

A base is limited to 2 MiB, a journal to 3 MiB, and a history to 256 accepted commands. Checkpoint storage has a 32-MiB budget and a 128-session limit, with a small bounded count of files inspected per session. Temporary replay work uses a separate disposable directory. Unresolved sessions are kept until a user resolves/discards them; unexpected data is preserved for inspection.

## Save locks and partial draft files

Current C++ writes use a process-lifetime OS lock in a persistent .embervale.writer.lock file. A process exit releases the lock automatically. The small file remains so lock identity stays stable; do not unlink it while another editor may be using it.

Legacy .embervale.lock directories have no reliable owner record. Studio refuses that original-source write without deleting the directory or guessing whether its writer died. Recovery uses a private replay base, so it can export a new copy while retaining the legacy lock and unknown data.

World drafts are staged completely before a new filename is published. Linux uses an exclusive hard link; Windows now stages, flushes and closes the file before a no-overwrite MoveFileW. Interrupted staging directories are ignored and retained for inspection. Existing world files, existing drafts and player saves are never replaced by recovery.

The normal save command remains sibling-only. The explicit recovery-export command, **recover-copy**, accepts a fresh .json in another existing directory after C++ runtime validation. This bounded export is not an immutable publication manifest or an in-place update.

## Scope

[Executed checks](../06-development/EDITOR_RECOVERY_2026-11-20.md) cover Linux process kills at lock/write/stage/publication boundaries, real command replay, corrupted generations, changed/missing originals, active owners and native restoration. Current Windows execution, network filesystem semantics and power-loss/physical-media durability remain unverified.

STUDIO-021's broader in-place replacement work, STUDIO-026 validated publication and QA-003's broader accessibility/platform review remain open. This feature is independent of [periodic player saves](../06-development/PLAYER_AUTOSAVE_2026-11-20.md).

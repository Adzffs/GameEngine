# Guarded draft revisions

**Current policy, 20 November 2026:** STUDIO-021 remains in progress for its broader in-place replacement/durability scope. Create-only drafts, process-lifetime locks and interrupted-write recovery are implemented. See [recovery guide](EDITOR_RECOVERY.md) and [executed verification](../06-development/EDITOR_RECOVERY_2026-11-20.md). Historical first-write results remain in the [18 November record](../06-development/GUARDED_DRAFT_VERIFICATION_2026-11-18.md).

## Current commands and boundaries

~~~bash
EmbervaleWorldMove --hash content/drafts/starter-v1.json
EmbervaleWorldMove content/drafts/starter-v1.json resource.normal_tree.001 30 30
EmbervaleWorldMove content/drafts/starter-v1.json resource.normal_tree.001 30 30 \
  --expected-hash HASH_FROM_FIRST_COMMAND --write-draft content/drafts/starter-v2.json
~~~

Work on your own draft rather than a committed fixture. Current runtime-valid schemas 1-4, typed move/paint/place/remove/spawn settings and bounded undo/redo share the C++ validator. The default operation is a read-only dry run. --hash fingerprints exact original bytes, including whitespace.

| Operation | Output rule |
|---|---|
| Ordinary save / --write-draft | Fresh sibling file, source hash rechecked |
| Explicit recover-copy session command | Fresh .json in a selected existing directory, private base rechecked |
| Existing destination | Refuse without replacing it |
| Changed source | CONTENT_CONFLICT, no candidate publication |
| Source read | Regular non-symlink file, at most 2 MiB |
| Candidate | Canonical runtime-valid bytes, at most 2 MiB |

Recovery replays cached original bytes and accepted commands in a private directory. Only its explicit new-copy export relaxes the sibling-directory restriction. This recovery export does not implement in-place overwrite or automatic game selection. The separate [fixed publication command](WORLD_PUBLICATION.md) now reuses this exclusive staged writer for world/manifest pairs and explicit fresh-session selection.

## Cooperative ownership

The C++ writer holds an OS lock in source.embervale.writer.lock while rereading the source and publishing. Linux uses flock; Windows opens the lock without sharing. The OS releases ownership when the process closes/exits, including abrupt death. The small sidecar remains intentionally; unlinking it during another writer's lifetime could split lock identity.

A live owner causes a refusal. A leftover regular lock file can be reused once its owner dies. Neither age nor a reused PID proves a lock is stale. Locks coordinate supported writers; they do not stop independent text editors from changing the source.

Legacy source.embervale.lock directories have no reliable owner evidence. They are refused and retained without deletion. The [private recovery path](EDITOR_RECOVERY.md) can export a separate new draft despite such a lock.

## Complete staging and failure handling

A unique directory under the destination parent holds staged bytes. Linux publishes by exclusive same-filesystem hard link. Windows now exclusively creates/flushes/closes the staging file, then uses MoveFileW to a name that must not exist. The final filename is not written incrementally.

Handled failures clean only owned stages. A killed process may leave an unknown staging directory; future writes ignore it and do not promote or delete its contents. A stop before publication leaves the final name absent. A stop after publication leaves a complete final file, as exercised on Linux. Source and prior drafts remain intact in the tested boundaries.

No current Windows execution or power-loss/physical-media durability guarantee is claimed. Local Linux process-kill tests are distinct from filesystem/power-failure tests. The previous Windows direct-to-final creation path has been replaced in source; its new runtime behaviour still needs Windows acceptance.

## Exit status

- 0: successful dry run, hash or draft publication.
- 2: invalid arguments/hash shape.
- 3: invalid input or typed operation.
- 4: revision conflict or occupied output.
- 5: source/lock/write failure.

Session errors are reported on stderr; the Python bridge refuses the result before accepting state or a save receipt. See [CLI contract](CLI_AND_AI_CONTRACT.md) and [visual editor](VISUAL_WORLD_EDITOR.md).

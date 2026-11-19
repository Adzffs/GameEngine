# Guarded draft revisions — bounded first write (STUDIO-021)

**Status:** Development-branch experiment, not the completed Studio authoring workflow.
**Scope:** Save a *new*, validated sibling snapshot after a supported resource or station move.
**Parent:** Stable-ID move-preview PR #9, itself awaiting a full repository build.
**Data:** Starter-world schema 1 only; all 20 compiled placements remain required.

## Usage

Work on a **separate draft copy** of the starter snapshot, not the committed test fixture or a player save.

```sh
EmbervaleWorldMove --hash content/drafts/starter-v1.json
EmbervaleWorldMove content/drafts/starter-v1.json resource.normal_tree.001 30 30
EmbervaleWorldMove content/drafts/starter-v1.json resource.normal_tree.001 30 30 \
  --expected-hash HASH_FROM_FIRST_COMMAND --write-draft content/drafts/starter-v2.json
EmbervaleWorldValidate content/drafts/starter-v2.json
```

`--hash` validates and fingerprints the **exact original file bytes**, including whitespace. Do not use the canonicalized content hash from a different copy. `--write-draft` parses and gameplay-validates the exact input, applies the normal stable-ID move command to an owned candidate, then serializes and revalidates that candidate. It requires a fresh filename in the same directory as the input. It cannot overwrite input, existing draft files, or a player save through an in-place operation. An unchanged-position move can create a new copy, so consumers should not assume every successful write moves an object.

Output includes the original and new SHA-256 digests. The new file is a regular validated snapshot that can be passed to `GameEngine --world` for an isolated playtest. No live world is mutated by the editing command.

## Concurrency and file boundaries

The writer obtains a `<source>.embervale.lock` directory using an exclusive filesystem directory creation operation, and rereads the input under that **cooperative** lock. A mismatching expected SHA-256 is a conflict and writes no candidate. It stages complete bytes in a uniquely named directory under the output parent, and uses an exclusive same-filesystem hard link to publish to a new filename; the link operation fails if a file already occupies the name. Temporary and lock directories are removed on the normal error/success paths.

This first slice deliberately **does not** perform in-place atomic replacement. It requires filesystem hard-link support and refuses to silently fall back to destructive copying. A crash may leave a lock or staging directory requiring manual inspection before cleanup. The lock is not mandatory for independent text editors; a noncooperating writer can still edit the source. Output visibility is all-or-nothing under normal supported local filesystem semantics, but data is not explicitly fsynced, so no power-loss durability claim is made. The source file is read only.

Source input is limited to 2 MiB and must be a regular non-symlink file. Output parent must already exist. Paths outside the source file's canonical parent are rejected by the current restricted workflow. Other possible editing actions (actor spawn relocation, arbitrary object authoring and changing terrain) remain unavailable.

## Exit statuses

- `0` — successful hash, dry-run or new draft publication.
- `2` — invalid command arguments, flags or hash shape.
- `3` — input fails the game schema/geography checks, or move is invalid.
- `4` — source revision conflict or output already exists.
- `5` — source/read/lock/write failure.

## Before integration

Reconcile with the parent move PR and build the exact stacked tree; run `StarterWorldDraftFileTests`, `StarterWorldMoveCliTests`, all existing starter-world tests and the game smoke test with a newly saved draft. Verify Windows filesystem behaviour and failure cleanup. No new player-save wire format has been introduced. After that, the remaining STUDIO-021 work is a general guarded revision API with durability and OS-specific compatibility; then STUDIO-023 undo/redo, followed by preview/editor UI.

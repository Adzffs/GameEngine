# CLI and AI editing contract

**Status:** Proposed editing interface. The implemented commands are [EmbervaleWorldExport](STARTER_WORLD_EXPORT.md), [EmbervaleWorldValidate and explicit game input](STARTER_WORLD_IMPORT.md). The shared editing command families below are not yet available.\
**Updated:** 18 November 2026\
**Goal:** Let a human and an AI agent inspect or change content through the same predictable operations.

## 1. Contract principles

Commands act on an explicit local region/content path. They do not search for a likely production map, modify live server state, or pick the most recently edited draft. Read operations are separate from write operations. Writes require a known base hash and an explicit target mode. Errors are structured, and failed transactions do not leave a partially edited document.

The canonical interface is the typed command service. The CLI is its first automation frontend. A later protocol wrapper can expose the same operations without becoming a second implementation of validation or editing. The GUI also calls this service.

A machine must be able to answer four questions after every operation: what input revision was used, what changed, which checks ran, and whether anything was written. Human-readable prose alone is not sufficient evidence for an automated follow-up.

## 2. Proposed command families

| Command | Purpose | Writes authored content? |
|---|---|---|
| `capabilities` | Report tool version, schema support and enabled operations | No |
| `map inspect` | Read dimensions, placements, definitions and current hash | No |
| `map export-baseline` | Export the compiled world to a new explicit output | New export only |
| `map paint` | Propose bounded terrain changes | Only with explicit draft-write mode |
| `map place` | Add a placement with a new durable identity | Only with explicit draft-write mode |
| `map move` | Move an existing authored placement | Only with explicit draft-write mode |
| `map remove` | Remove a placement after dependency checks | Only with explicit draft-write mode |
| `map apply` | Apply a typed patch containing multiple operations atomically | Only with explicit draft-write mode |
| `map diff` | Compare authored revisions semantically | No |
| `map validate` | Run a named validation profile on an exact candidate | Report output only |
| `map preview` | Render the selected candidate and overlays | Preview output only |
| `content publish` | Create a validated immutable package | New package only |
| `playtest` | Launch an isolated test using an explicit package and save | Test state only |

Exact executable name, subcommands and flag spelling remain an implementation decision. Initial work need only implement the subset required by the first complete tree-move workflow. Unsupported operations should return a capability error, not succeed with no effect.

## 3. Proposed tree-move workflow

These examples describe intended syntax. `BASE_HASH` means the exact hash returned by inspection; it is not a guessed constant. Paths are illustrative and must be replaced with actual project paths once the content pipeline exists.

```bash
embervale-studio capabilities --format json
embervale-studio map inspect --file content/drafts/development_starter.json --format json
embervale-studio map move --file content/drafts/development_starter.json --id resource.tree.001 --x 6 --y 5 --expected-hash BASE_HASH --dry-run --format json
embervale-studio map move --file content/drafts/development_starter.json --id resource.tree.001 --x 6 --y 5 --expected-hash BASE_HASH --write-draft --format json
embervale-studio map validate --file content/drafts/development_starter.json --profile gameplay --report artifacts/validation.json
embervale-studio map preview --file content/drafts/development_starter.json --overlay collision,spawns --out artifacts/preview.png
```

The coordinates are an example, not an assertion that this location is valid in the final map. The dry run must evaluate it against the actual candidate. The write invocation uses the same base hash; if another edit occurred after the dry run, it fails and asks the caller to inspect the new state.

The first implementation may retain stricter gameplay-valid edits for individual placement moves while still allowing incomplete terrain work in a draft. Whatever draft policy is selected must be explicit in capabilities and validation output. No policy can allow unsafe structure, out-of-bounds access or unknown identity references to pass as a valid typed model.

## 4. Typed patch format

A proposed patch envelope includes the target region, expected hash, optional expected revision and an ordered list of operations. Every operation has an explicit type and allowed fields. Unknown operation types are errors. The patch is data, not shell text or executable code.

```json
{
  "patch_version": 1,
  "region_id": "embervale.fixture.tree_move",
  "expected_hash": "COPY_EXACT_HASH_FROM_INSPECT",
  "operations": [
    {
      "operation": "move_placement",
      "placement_id": "resource.tree.001",
      "position": { "x": 1, "y": 1, "surface_id": "ground" }
    }
  ]
}
```

This is valid JSON illustrating the envelope, not an executable current-game patch. A successful batch is all-or-nothing. If an operation requires an ID allocated by an earlier operation, either reference an explicit new ID supplied in the same patch or use a specified symbolic-reference mechanism. Do not depend on undocumented order-dependent runtime IDs.

Commands must not resolve file references outside the selected project root. Resolve symlinks/path normalisation according to a documented policy before writing. Limit file size, operation count and affected tile count. Those limits should be reported by capabilities and have rejection tests.

## 5. Transaction and concurrency algorithm

1. Resolve the explicit target within the allowed project scope and read its metadata/bytes.
2. Parse and validate input safety; derive the canonical semantic hash.
3. Compare the expected hash and optional revision. Stop on mismatch.
4. Apply typed operations to an in-memory candidate, retaining the prior state and inverse data.
5. Run the requested structural/gameplay checks and compute the semantic diff.
6. For a dry run, return the candidate hash, diff and diagnostics without writing content.
7. For a write, acquire the documented per-document/package lock and reread/recheck the base hash under that lock.
8. Write the candidate to a temporary file in the appropriate filesystem location, flush according to the supported durability contract and atomically replace where supported.
9. Release the lock and return the persisted hash, revision, diff and write result.

Checking a hash before taking a lock is insufficient: two writers can both pass that check. Recheck at the commit boundary. Atomic rename prevents readers seeing partial bytes on supported filesystems; it does not by itself prevent lost updates or guarantee survival of every power failure.

For multi-file packages, use a tested package transaction or staged immutable files followed by one manifest activation. Do not claim transaction success after replacing only the first of several related files.

## 6. Output and diagnostics

A proposed JSON response contains `tool_version`, `operation`, `status`, `target`, `base_hash`, `candidate_hash`, `written`, `changes` and `diagnostics`. When a field does not apply, the documented response schema must specify whether it is absent or null; callers should not infer success from an empty object.

Diagnostics carry a stable code, severity, message, region/placement ID and optional field path or tile coordinates. Examples of proposed codes include `CONTENT_VERSION_UNSUPPORTED`, `PLACEMENT_ID_DUPLICATE`, `REFERENCE_UNKNOWN`, `SPAWN_BLOCKED`, `APPROACH_UNREACHABLE`, `CONTENT_CONFLICT` and `WRITE_FAILED`. Codes are an API and should not change merely to improve wording.

### Proposed exit-code classes

| Code | Meaning |
|---|---|
| `0` | Requested operation completed successfully |
| `2` | Invalid command arguments or unsupported capability |
| `3` | Invalid content or failed required validation |
| `4` | Stale content/conflict; no edit committed |
| `5` | Read/write/lock failure |
| `6` | Internal tool failure |

A dry run that detects a blocking validation error should use the validation code while still returning structured diagnostics. A valid draft saved with nonblocking design warnings can succeed while clearly reporting those warnings. Avoid printing progress banners into machine-readable stdout; use stderr or a documented progress stream.

## 7. AI workflow

An agent first reads capabilities, inspects the exact region and identifies the authored objects by stable ID. It proposes a bounded patch, runs a dry run, reviews validation and a semantic diff, then writes the authorised draft change. It generates a preview and a validation report bound to the new hash. If required by the user's task, it can prepare a publication candidate or a focused code/content change for review.

The agent must stop and rebase its intent when the expected hash is stale. It must not suppress validation, expand its filesystem scope or overwrite another writer's work to force success. The command contract should make this behaviour easy by returning the actual current hash and a concise conflict summary.

Repeated requests need explicit retry semantics. A content move to an already-selected coordinate can report a documented no-op; a create operation must not duplicate an object after an uncertain response. For writes where outcome is unknown, inspect the current file before retrying. Optional request IDs can help correlate logs, but they do not replace checking committed content.

## 8. Preview and test evidence

A preview report records content hash, renderer/tool version, dimensions, overlays and selected layer. Canonical content must be deterministic across supported platforms. Pixel equality across different graphics backends is a separate claim; pin the renderer/environment where pixel snapshots are used, or compare a specified semantic render description.

A playtest command must select a content package explicitly and use a test save path. Its result should report the tested package hash and scenario/seed. “Launched successfully” is not proof that gathering, combat or persistence worked; smoke scenarios need their own observed outcomes.

## 9. Publication and optional integrations

Publication validates that the report belongs to the exact candidate, required profile and supported tool/schema version. It writes an immutable content package and returns its manifest location. Activation, GitHub push, public release and live-server rollout remain separately scoped actions.

An optional protocol wrapper may provide capability discovery, inspect, dry-run, apply, validate and preview resources. It must enforce the same path scope, hashes, limits and result schemas. Do not expose unrestricted shell execution or arbitrary filesystem writes merely to make editing convenient. Keep the CLI documented and usable without the wrapper.

See [Content model](../03-engine/CONTENT_MODEL.md) for validation levels and [Editor workflows](EDITOR_WORKFLOWS.md) for human operation.

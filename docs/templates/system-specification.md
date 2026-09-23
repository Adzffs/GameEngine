# System Specification Template

**System ID:** `SYS-___`
**Status:** Proposal / Needs Audit / In Progress / Done
**Owner:**
**Related milestone:**

## Purpose and boundaries
What responsibility does this system own? What belongs elsewhere?

## Player-facing behavior
Describe the player-visible rules and flow.

## Invariants
-

## Data model and stable IDs
List state, ownership, definitions, serialization and validation constraints.

## APIs and command flow
Inputs, validation, state transitions, outputs/events, ordering and error results.

## Authority and determinism
Server/client responsibilities, random sources/order, duplicate/stale command handling.

## Persistence and migrations
Saved fields, versions, historical schemas, defaults and transactional failure behavior.

## Failure cases
Cancellation, invalid input, full inventory, disconnect, stale state, malformed content/save.

## Content authoring and validation
How data is added, referenced, validated and presented.

## Tests
Unit, integration, lifecycle, client, migration and performance tests.

## Open decisions and dependencies

## Evidence and implementation references
Commit, files, test commands/results, review and status.

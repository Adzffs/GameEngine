# ADR-016: Read-only starter-world snapshot before import

**Date:** 18 November 2026\
**Status:** Historical export decision, implemented first and extended by [ADR-017](ADR_017_BOUNDED_STARTER_WORLD_IMPORT.md) for shared core ownership and explicit runtime input.\
**Related work:** ENG-002, STUDIO-001–004, STUDIO-010–012 and STUDIO-015.\
**Sources:** [baseline inventory](../03-engine/STARTER_WORLD_BASELINE.md), [snapshot implementation](../../../src/Content/Export/StarterWorldSnapshot.cpp), [runtime parity tests](../../../tests/StarterWorldSnapshotTests.cpp).

## Context

The user needs control over the placement of trees, workstations, enemies and NPCs. The current engine constructs its development world in C++ and stores occupied tiles together with terrain. Its constructor inserts a pond, applies ordered terrain rectangles, registers spawn definitions and creates some actors immediately. Other actors appear only after a successful first-player registration. Resource and station IDs share one counter, while actors use another.

Extracting a final rendered grid would lose the ground beneath objects. Sorting placements for a clean file would also lose creation order unless lifecycle metadata is represented separately. An editor built on either incomplete representation could appear useful while changing gameplay during migration.

## Decision

Build a separate SDL-free `EmbervaleWorldSnapshot` library and an `EmbervaleWorldExport` command. Both depend on existing `GameEngineCore` definitions; the game does not depend on these tools. `GAMEENGINE_BUILD_CONTENT_TOOLS` controls their inclusion. There is no dependency cycle and no change to the existing world-construction path.

Capture the complete initial starter content into a typed snapshot. Recover underlying ground using the current `Map` initialization and ordered terrain definitions before occupancy is applied. Preserve the initial player spawn, all resources/stations, all ten spawn records and explicit initialization stage/order. Use explicit durable string mappings for authored identities and existing enum bindings for compatibility assertions.

Write a deterministic JSON snapshot with a distinct `format` marker and `schema_version: 1`. Use restricted ASCII terrain rows for reviewable 100×100 geography. Sort placements by durable ID, preserve initialization keys separately, and produce fixed field ordering with LF and no timestamp. The current serializer validates its supported in-memory snapshot before writing.

The CLI writes only to standard output and accepts no input or destination path. This avoids introducing the editor's file transaction, save-recovery and overwrite semantics in a read-only extraction step. Shell redirection is the caller's explicit file operation.

Do not add a JSON parser yet. An output-only tool needs a correct writer and independent JSON verification, not an unreviewed runtime loader. CMake's built-in JSON reader checks the emitted document in CLI tests. Parser choice, hostile-input limits, unknown-field policy, general edit validation and runtime provider integration remain STUDIO-011/014 and related work.

## Alternatives considered

| Alternative | Reason not selected for this slice |
|---|---|
| Export the current occupied tile grid alone | Cannot recover terrain when an object moves; misses delayed actor definitions |
| Instantiate the full game and save its live state | Mixes authored content with runtime state and risks entering save lifecycle behaviour |
| Build the loader and GUI together | Combines too many compatibility changes before a baseline is proved |
| Add a third-party parser solely for export | Adds a dependency without an input requirement; the future parser decision remains explicit |
| Treat sorted placement order as initialization order | Changes legacy actor/object creation semantics |

## Consequences

The snapshot is a concrete baseline that people and automation can inspect, diff and test. The CLI runs in the headless configuration and uses no player-save API. At the original exporter boundary, output was not supported runtime input. ADR-017 adds explicit loading and supported geography changes; the exporter itself remains read-only.

The registry intentionally supports the current unique legacy placements. A second placement of the same resource type requires a new durable identity and an explicit registry change. This is a baseline compatibility bridge, not the final general placement catalogue.

The snapshot's terrain extraction follows legacy rules separately from `World`; full-grid runtime comparison is therefore a required regression check. The runtime's coupled ground/occupancy representation remains unchanged. A future loader must validate bounds before `Map` access, preserve spawn authority checks, and resolve all saved-location risks before activating edited content.

Preview backend and GUI packaging remain open decisions. Their absence does not prevent this extraction proof; it does prevent claiming Studio Alpha 0.1 is complete.

## Validation and follow-up

Acceptance for this slice is deterministic valid JSON, a reviewed complete fixture, all-cell terrain/occupancy parity, lifecycle and object-binding parity, and passing existing relevant regressions. The [verification record](../06-development/BASELINE_VERIFICATION_2026-11-18.md) records local outcomes and platform limitations.

Next implement bounded input and baseline loading equivalence, then one shared move operation. Keep this snapshot and its historical baseline assertions available while that integration is developed. If the runtime input format differs, introduce an explicit versioned conversion rather than silently treating the export marker as permission to load arbitrary documents.

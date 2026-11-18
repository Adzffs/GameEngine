# Starter-world snapshot input validation (first import boundary)

Status: **implemented on `studio/world-import-foundation` for review; not a runtime importer**.

The read-only exporter in PR #4 emits a version-1 starter-world snapshot. Before loading this data into the authoritative game world, the input must be rejected safely when malformed, incompatible or unsupported.

## What this slice provides

`tools/validate_world_snapshot.py` uses only the Python standard library and validates the complete version-1 snapshot envelope, dimensions, rows, surface, player spawn, placement identity, spawn settings, initialization stages and ordering. It rejects duplicate JSON object fields and duplicate placement/spawn IDs. It has a file-size bound and does not write any content.

Run from the repository root:

```sh
python3 tools/validate_world_snapshot.py tests/fixtures/content/development-starter.snapshot.json
python3 tools/test_validate_world_snapshot.py
```

The second command contains positive-fixture and malformed-input cases. These tests are **not part of CTest yet** and have **not been verified in the repository's GitHub Actions environment**. No claim is made that a Python validator is a substitute for the C++ runtime's content validation.

## Boundaries and next work

This slice deliberately does **not**:

- load the file into `World` or replace compiled world construction;
- mutate resources, terrain, NPCs, saves or running sessions;
- implement editor commands, undo, previews or publication;
- establish that the file is safe or playable solely because its structure is valid.

Next, design and implement a C++ parser and typed authored-content model with the same schema rules, then prove complete runtime parity against the existing compiled world. Keep the existing world-construction path available until that parity gate passes. Ensure movement, collision, NPC stage/order, resource depletion, station interaction, quests and saved-player compatibility remain intact.

The importer must reject unknown future schema versions rather than silently reinterpret them. Changes to authored IDs require explicit migration rules. Never sort initialization order just because the JSON placement array is sorted for clean diffs.

Related: [world exporter](STARTER_WORLD_EXPORT.md), [content model](../03-engine/CONTENT_MODEL.md), [save compatibility](../03-engine/SAVE_COMPATIBILITY.md).

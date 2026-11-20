# Authored content model and validation contract

**Status:** Broader authoring schema and rules are proposed. The distinct [starter snapshot](../04-studio/STARTER_WORLD_EXPORT.md) now has a [bounded runtime loader](../04-studio/STARTER_WORLD_IMPORT.md) for its existing 100×100 identity registry. The small illustrative schema below remains unsupported.\
**Updated:** 20 November 2026\
**Purpose:** Give engine, editor and automation work one precise description of authored content.

## 1. What is being saved

A region document describes an initial playable location. It contains terrain, authored placements and references to reusable definitions. It does not contain the current health of a monster, a player's inventory, a tree's depletion timer or a running dialogue session.

The real development region is currently 100×100 tiles with default player spawn `(0,0)`. Its world construction includes a constructor-created water tile, ordered terrain overlays and object/spawn lifecycles. The exporter must capture that actual behaviour; a small example in this document is only a schema illustration. [Sources: DevelopmentWorldContent.h](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Development/DevelopmentWorldContent.h), [World.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/World.cpp), [Map.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Map.cpp).

### Vocabulary

| Term | Meaning | Example |
|---|---|---|
| Region | Named authored map space | `embervale.development_starter` |
| Definition | Reusable gameplay type | Normal tree, furnace, named NPC |
| Placement | One durable authored instance | `resource.tree.001` |
| Surface | Distinct navigable layer | `ground` initially |
| Footprint | Tiles occupied by a placement | One tile in the initial adapter |
| Approach tile | A position from which an interaction is legal | Reachable tile beside a bank |
| Runtime entity ID | Temporary identity assigned in a running world | An integer assigned at creation |
| Content revision | Human-readable revision number | Useful for display and history |
| Content hash | Digest of canonical semantic content | Used to detect changed input |

The authoring ID stays unchanged when its coordinates or display name change. Copying a placement creates a new ID. Undoing a deletion restores its original ID if no conflict exists. Duplicate IDs are errors, including duplicates that happen to occupy different tiles.

## 2. Coordinates and layers

The proposed coordinate convention uses integer tile positions, origin at the top-left, positive X to the right and positive Y downward. Valid coordinates satisfy `0 ≤ x < width` and `0 ≤ y < height`. Pixel size belongs to presentation; it must not be multiplied into saved tile positions. The existing renderer's 32-pixel tiles do not make world coordinates pixel coordinates. [Source: Graphics.h](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Graphics/Graphics.h).

Layers have separate responsibilities:

| Layer | First implementation | Later extension |
|---|---|---|
| Base terrain | Grass, dirt, stone, water and approved blocking terrain | More terrain definitions and transitions |
| Gameplay placements | Existing resources and stations, then NPC/monster spawns | Supported larger footprints and region portals |
| Decorations | Optional noninteractive visual records | Density tools and seeded scatter |
| Labels/markers | Authored editor or presentation metadata | Quest/debug overlays |
| Collision overrides | Prefer derived collision; omit until justified | Explicit, validated overrides with clear provenance |
| Surfaces | Exactly one accepted `ground` surface | Reviewed multi-surface movement model |

A decorative canopy can cover several pixels or tiles without becoming an equally large collider. A building's artwork, footprint, door and interaction points are separate concepts. Multi-tile runtime support is a later capability, not an automatic consequence of adding footprint metadata to a file.

## 3. Complete small schema illustration

The following is valid JSON and a complete **proposed-format** 3×3 fixture. It is not loadable by the current game, is not the real development map and is not a final JSON Schema specification. The legend and resource definition would be resolved by the proposed catalogue adapter.

```json
{
  "schema_version": 1,
  "region_id": "embervale.fixture.tree_move",
  "revision": 1,
  "size": { "width": 3, "height": 3 },
  "surfaces": [ { "id": "ground", "elevation": 0 } ],
  "terrain": {
    "encoding": "rows",
    "legend": { "G": "terrain.grass" },
    "rows": [ "GGG", "GGG", "GGG" ]
  },
  "player_spawn": { "x": 0, "y": 0, "surface_id": "ground" },
  "placements": [
    {
      "id": "resource.tree.001",
      "kind": "resource",
      "definition_id": "resource.normal_tree",
      "initialization": { "stage": "world_start", "order": 0 },
      "position": { "x": 2, "y": 1, "surface_id": "ground" }
    }
  ],
  "region_exits": []
}
```

For a row encoding, there must be exactly `height` rows and exactly `width` symbols per row, using a deliberately restricted symbol alphabet. Do not count arbitrary UTF-8 bytes as tiles. If another encoding is selected after testing readability and file size, document an explicit migration and keep the same interpreted grid.

The final schema must define required fields, allowed enums, numeric ranges, unsupported-feature behaviour and unknown-field policy. A v1 implementation should reject unknown gameplay fields with a useful diagnostic rather than silently dropping a future feature on save. Tool-only annotations can have a designated, bounded extension object if a concrete need emerges.

## 4. Identity registry and references

Bridge existing enum values to authored strings through an explicit mapping table. The exporter and importer must not renumber `NpcSpawnId`, `ItemType`, resource or station identities. The initial content model may still refer to compiled gameplay definitions; externalising every definition catalogue is a separate workstream. [Sources: NpcSpawnDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/NPC/NpcSpawnDatabase.cpp), [DevelopmentWorldContent.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Development/DevelopmentWorldContent.cpp).

Maintain a registry containing stable ID, kind, current definition, introduction revision, deprecated aliases and migration rule where needed. A retired identifier is not reused for a different object. Display-name changes need no identity migration. An ID rename, merge or deletion requires a reference report covering quests, dialogue, shops, spawns, assets, tests and any persisted field that uses it.

Validate references against the selected content package, not whichever global database happens to be available. This is particularly important because current typed NPC/monster creation checks supplied spawns against the canonical compiled spawn database. The implemented starter adapter owns a validated selected spawn catalogue per World; all other gameplay definitions remain compiled. Future general providers must retain that authority rather than accepting arbitrary fields. [Source: World.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/World.cpp).

## 5. Validation is a sequence of guarantees

| Level | Checks | Consequence |
|---|---|---|
| Input safety | File size, encoding, depth, dimensions, multiplication overflow, collection limits | Unsafe input is rejected before allocation or tile writes |
| Structure | Version, required fields, row shape, unique IDs, legal enum values | Malformed content cannot enter the typed model |
| References | Known definitions, surfaces, assets and target regions | Missing dependencies are explicit errors |
| Spatial validity | Bounds, supported footprints, overlap policy, spawn clearance | Impossible placements are identified |
| Gameplay validity | Authoritative movement, approach tiles, mandatory routes, spawn rules | Broken required gameplay blocks publication |
| Runtime parity | Imported baseline matches relevant existing observations | Migration can proceed |
| Playtest quality | Readability, pacing, density, navigation and enjoyment | Design changes are accepted or revised |

Set numeric resource limits from measured supported map sizes during the foundation milestone. Whatever limits are selected must be documented and tested; “large enough” is not a validation rule. The current `Map::GetTile` indexes the vector directly, so bounds validation must happen before loader writes. [Source: Map.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Map.cpp).

Use actual pathfinding/interaction semantics to test reachability, including diagonal restrictions. A simple connected-cell check can be a fast preliminary diagnostic, but it must not override a disagreement from authoritative rules. Validate routes from the designated spawn to required guide/service/quest approaches and approved exits. Not every optional wilderness tile needs to be reachable; required destinations must be explicitly marked so the validator does not invent the designer's intent.

Terrain beneath a moved object remains authored terrain. Placement validation must reject prohibited terrain and unsupported overlaps. Testing an attempted move onto water should normally demonstrate rejection, while deletion or movement away must preserve the old underlying tile. Do not “fix” an invalid move by silently painting ground.

## 6. Canonical files and hashes

Canonical serialisation uses a specified field order, stable placement ordering by durable ID, stable numeric formatting, UTF-8 and a consistent newline convention. Semantically unordered collections are sorted; semantically ordered content, such as an intentional dialogue choice sequence, keeps its order. Wall-clock timestamps and tool-specific caches are excluded from semantic content.

**Serialisation order is not runtime creation order.** The full exported model must preserve an explicit initialisation stage and order key, or an equivalent versioned baseline registry, separately from the array's canonical ID ordering. The proposed stages are `world_start` and `first_player_registration`, with a unique, stable order key within each stage. The runtime adapter registers the selected spawn catalogue before creating its actors and then follows the recorded initialisation order. Moving or renaming a placement must not change that key. New placements receive a documented key through the command service; duplicate/unsupported stage-order combinations are validation errors. This metadata selects known engine lifecycle hooks, not scripts. The illustrative fixture contains one world-start placement; the full development export must account for every existing creation stage and order.

A package manifest records hashes of its constituent files and the schema/tool version used for validation. Avoid placing a file's own digest inside the bytes used to calculate that same digest. Revision numbers help a person follow history; hashes detect changed input. The command layer checks both when supplied and rechecks under the write lock.

One-file atomic replacement does not make a multi-file package atomic. The first release should minimise transaction breadth. When a content edit must span several files, stage a complete package and activate one manifest only after every file is ready, or use a journaled package transaction with tested recovery.

## 7. Drafts and publications

**Current starter implementation:** [World publication](../04-studio/WORLD_PUBLICATION.md) binds exact saved/canonical bytes and validation implementation inputs, creates canonical world.json plus a last-committed manifest.json under a fresh name, and explicitly selects a version for a fresh runtime process. Receipt/manifest schemas are version 1; world schemas 1–4 remain supported. Older recorded validator revisions must pass current runtime checks. Current accepted drafts still require runtime validity; the broader unfinished-draft and multi-file/asset model below remains proposed.

Proposed content states are **Draft**, **Validated** and **Published**. A structurally valid draft may contain a disconnected road or unfinished building and must remain recoverable. Gameplay validation determines whether it can become a validated candidate. Publication creates an immutable revision selected explicitly by a development launch or release manifest. Any semantic edit invalidates the previous validation result.

Draft autosaves belong in an editor workspace, separate from player saves and the active game content package. Startup must not select whichever draft was edited most recently. A failed validation should preserve the draft and its diagnostics; it should not replace the last published package.

Publication here means making a validated content revision available for explicit game loading. It does not imply deployment to a public server or publishing a GitHub release. Those are separate workflows.

See [CLI contract](../04-studio/CLI_AND_AI_CONTRACT.md) for transactions and [Save compatibility](SAVE_COMPATIBILITY.md) for persistent-state changes.

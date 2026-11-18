# Save compatibility and content migration

**Status:** Current starter import preserves save version 13 and adds selected-map position handling. Broader versioned content/save migration mechanisms below remain proposed.\
**Updated:** 18 November 2026  
**Primary rule:** A geography edit must not silently discard a player's earned state.

## 1. Current persistence boundary

The current source declares `CURRENT_PLAYER_SAVE_VERSION = 13`. Saved data includes position, health, inventory slots, skill XP, equipment, quest records, bank entries, tutorial state, combat style, known recipes and an optional recovery pile. The recovery pile stores coordinates, remaining ticks and items. This list describes the current data structure, not a promise that every historical version has been independently tested during this documentation work. [Source: PlayerSaveData.h](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Persistence/PlayerSaveData.h).

The existing persistence guide says the development application opts in through `EngineConfiguration::developmentPlayerSavePath`; the usual application path is `saves/development-player.save`, relative to the process working directory. Runtime entity IDs are assigned afresh, startup does not rewrite loaded saves and normal ticks do not autosave. Controlled shutdown uses the existing file store. The guide explicitly does not guarantee survival of forced termination, OS failure or power loss. [Source: DevelopmentPlayerPersistence.md](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/docs/DevelopmentPlayerPersistence.md).

The file store already performs bounded reads and structured path/error handling. Reuse its relevant patterns, but do not assume a player-save transaction automatically solves concurrent Studio edits or multi-file content publication. [Source: PlayerSaveFileStore.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Persistence/PlayerSaveFileStore.cpp).

## 2. Three independent version domains

| Domain | What a version means | Change policy |
|---|---|---|
| Player save format | How persistent player state is encoded | Change only when saved semantics or encoding require it |
| Content schema | How region and definition files are encoded | Explicit parser support and migration rules |
| Content revision | Which authored geography/content package is selected | Immutable published package plus compatibility assessment |

A tree moving two tiles does not inherently require player save version 14. Conversely, adding a persisted region or surface identity may require a save-format change even when the visual map changes very little. Keep these concerns separate in both code and release notes.

The first flat-world migration should preserve the current save format wherever possible. Because current saved positions have no region or surface fields, the adapter must explicitly interpret them as positions in the selected legacy-compatible development region on `ground`. This is a compatibility rule, not an excuse to assume every future map uses the same coordinate space.

## 3. Position fallback and remaining migration scope

For the compiled world, the existing blocked/out-of-bounds player fallback remains the development spawn. Imported starter content uses its selected player spawn for new players, saved-position fallback and death respawn. Saved player positions must be traversable and connected to the selected safe starting area. The recovery pile must remain accessible from that area as well; incompatible recovery positions are relocated without changing their item quantities, owner or remaining expiry ticks. Player registration still triggers delayed starter entities only once.

This is a bounded compatibility rule for the same 100×100 starter region and `ground` surface. It does not add region identity to saves, perform an offline migration, or provide general versioned content migration manifests. The [runtime import guide](../04-studio/STARTER_WORLD_IMPORT.md) documents command selection and scope; [World](../../../src/World/World.cpp) and [runtime tests](../../../tests/StarterWorldRuntimeTests.cpp) contain the implementation and regression scenarios.

Imported-world launches use no player save by default. An explicit `--save` opts into the existing persistence lifecycle, while the save target and its temporary/backup paths are checked against world-file aliases. Ordinary no-argument startup keeps the normal development save. Runtime content validation does not write player files.

### Location migration matrix

| State | Hazard when geography changes | Required decision/test |
|---|---|---|
| Saved player position | Building, water or blocked tile now occupies it | Preserve valid position; otherwise use an explicit safe fallback |
| Player respawn point | Authored spawn becomes blocked or dangerous | Validate before publication and at runtime entry |
| Recovery pile | Items appear outside bounds or beyond a new barrier | Define reachable relocation that preserves items and ownership |
| Monster home/spawn | Changed terrain invalidates respawn or leash | Revalidate spawn, home region and reachable behaviour |
| Named NPC placement | Dialogue, shop or quest target cannot be approached | Validate identity references and mandatory approach routes |
| Region exit/arrival | Destination is removed or not loaded | Reject broken reference or apply an explicit migration |
| Quest objective location | Coordinates no longer describe the intended objective | Prefer durable authored references; migrate any coordinate-bound objective |
| Future surface position | Old save has no height/surface identity | Explicit default or conversion, with no ambiguous bridge placement |

## 4. Safe fallback policy

For the first release, retain the established development spawn rule while adding coverage for edited content. A published package must contain a valid player spawn and the required routes from it. Test that the point remains outside enemy spawn/aggression conditions intended to be unsafe for a new player.

For later regions, a proposed fallback hierarchy is: preserve the valid original position; use a versioned relocation rule for the affected area; use the region's nominated safe arrival; finally fail with a clear diagnostic if no safe legal placement exists. Do not select a random nearby tile, teleport into a locked area or delete a save to recover from invalid content.

Recovery-pile relocation needs its own rule. Preserve quantities and ownership, avoid duplicating the pile, keep expiry semantics explicit, and report what moved. A review should decide whether timers continue, pause or receive a bounded adjustment during an offline migration; do not change this silently. Store migration evidence without putting private player contents in public logs.

## 5. Identity migration registry

Maintain durable authored IDs separately from temporary runtime IDs. An identity migration entry should record the old ID, new ID or retirement action, affected content versions, reason and validation fixture. Changes to enum mappings require review because saved data and compiled gameplay definitions may use those values.

Acceptable cases include renaming a display label without changing its ID, moving a placement while keeping identity, and replacing a retired quest reference through an explicit mapping. Dangerous cases include reusing a retired ID for a different NPC, changing an enum's numeric meaning, or treating a missing item definition as permission to discard the player's item.

Before deleting a definition, generate a dependency report. If saved state can reference it, choose a supported migration, keep a compatibility definition or defer deletion. A general-purpose migration scripting language is not required for the first editor release; reviewed typed transformations are easier to constrain and test.

## 6. Baseline export and importer rollout

The first migration is behavioural preservation, not a redesign of River Crossing. Capture the existing 100×100 region, the original pond, terrain overlays, seven resources, three stations and the ten authored NPC/monster spawn records with their lifecycle conditions. Compare the imported world before and after player creation so deferred entities are not accidentally moved to startup. [Sources: DevelopmentWorldContent.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Development/DevelopmentWorldContent.cpp), [NpcSpawnDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/NPC/NpcSpawnDatabase.cpp), [World.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/World.cpp).

Rollout steps:

1. Save read-only reference snapshots and representative save fixtures from the pinned baseline.
2. Export content while the current construction path remains active.
3. Validate and inspect the export, including underlying terrain and occupancy.
4. Add an explicit loader selection and run both paths in comparison tests.
5. Exercise gathering, station use, NPC interaction, combat, death/recovery and save/reload.
6. Make one small authored edit and test the expected difference plus unchanged state.
7. Enable the new path in development only after parity and recovery gates pass.
8. Remove legacy construction only when rollback and compatibility evidence no longer rely on it.

A loader failure should be visible. During migration, an explicit developer-selected fallback can aid diagnosis; silently substituting the old map would make an invalid content release look successful.

## 7. Required regression fixtures

| Fixture | Expected result |
|---|---|
| Missing save | New player uses validated spawn; startup does not write a save |
| Existing valid save | Persistent values preserved and runtime identity assigned normally |
| Saved tile becomes blocked | Player moves to documented fallback; inventory/quests/XP remain unchanged |
| Saved tile outside resized region | Controlled fallback or explicit incompatibility result |
| Default spawn blocked | Publication fails; runtime load fails safely if encountered |
| Recovery pile obstructed | Chosen relocation policy preserves items exactly once |
| Unknown item/quest mapping | Explicit incompatibility diagnostic; no silent value loss |
| Truncated/corrupt save | Structured failure and original file preserved |
| Interrupted content write | Previously published content remains selectable |
| Undo/redo geography change | Authored identity preserved; no player-save rewrites |
| Imported baseline plus deterministic actions | Same relevant state as compiled baseline |
| Future surface migration | Old saves enter documented ground surface; incompatible cases are explicit |

Historical fixtures must state their originating format version. A current serializer generating a “legacy” fixture is insufficient evidence that real older encodings remain supported. Keep test saves synthetic and small, with no personal account details.

## 8. Rollback and operational ownership

Content rollback selects a previous immutable package; player-save rollback restores persistent state and is a different, potentially destructive action. Do not roll back player saves automatically because a tree was placed badly. First determine whether the old package can safely interpret the current save state.

Each release should record supported save versions, supported content schemas, the selected package hash, migrations applied, known incompatible combinations and recovery instructions. A migration that changes persistent data needs an exact-once or idempotent design and a recorded completion marker. Interrupted migration must resume or refuse safely without multiplying rewards or losing items.

The authoring workspace is never a backup location for the user's actual player save. Playtests use an explicit test save or a copied fixture. See [Editor workflows](../04-studio/EDITOR_WORKFLOWS.md) for that separation and [CLI contract](../04-studio/CLI_AND_AI_CONTRACT.md) for guarded writes.

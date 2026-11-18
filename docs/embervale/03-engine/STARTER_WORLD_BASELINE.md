# Starter-world baseline and integration inventory

**Status:** Source-verified inventory and future integration requirements; not an importer or migration sign-off.\
**Reviewed:** 18 November 2026\
**Source baseline:** `c697386382845f17ce02027e6bade24591a9eb36` (`master` before the first exporter implementation).\
**Purpose:** Support `STUDIO-001` / `STUDIO-002` with an exact account of the compiled development world. Test execution results belong in the implementation verification record.

The first export preserves the existing development area. It does not implement the proposed River Crossing redesign. The sections labelled **Observed** describe the inspected source; **Integration requirement** describes work a future loader or editor must satisfy. See the [content model](CONTENT_MODEL.md), [architecture](ARCHITECTURE.md) and [save compatibility plan](SAVE_COMPATIBILITY.md) for the wider design.

## 1. Ownership map

| Concern | Current owner | Consequence for Studio |
|---|---|---|
| Dimensions, default spawn, terrain rectangles, resource and station placements | [DevelopmentWorldContent.h](../../../src/World/Development/DevelopmentWorldContent.h), [DevelopmentWorldContent.cpp](../../../src/World/Development/DevelopmentWorldContent.cpp) | Read these definitions; avoid another manually maintained coordinate list in production code. |
| Initial grass and the pond | [Map.cpp](../../../src/World/Map.cpp), [Tile.cpp](../../../src/World/Tile/Tile.cpp) | Constructing a map already changes one tile to water. |
| Overlay order, occupancy writes, creation stages, player and monster respawn | [World.cpp](../../../src/World/World.cpp) | A catalogue dump alone does not capture world construction. |
| NPC identities, names, combat, dialogue and shop references | [NpcDefinitionDatabase.cpp](../../../src/NPC/NpcDefinitionDatabase.cpp) | Preserve compiled definition references during the first export. |
| Ten authored spawn records and a duplicated monster subset | [NpcSpawnDatabase.cpp](../../../src/NPC/NpcSpawnDatabase.cpp) | Preserve spawn settings and check that the subset agrees with the full catalogue. |
| Population registration and wander timing | [NpcSpawnManager.cpp](../../../src/NPC/NpcSpawnManager.cpp) | Registration happens before actor creation. |
| Temporary runtime IDs | [EntityManager.cpp](../../../src/Entity/Manager/EntityManager.cpp), [ObjectManager.cpp](../../../src/World/Object/Manager/ObjectManager.cpp) | These are separate counters, not durable authored identities. |
| Collision, paths and interaction approaches | [Tile.cpp](../../../src/World/Tile/Tile.cpp), [Pathfinder.cpp](../../../src/Pathfinding/Pathfinder.cpp), [InteractionSystem.cpp](../../../src/Interaction/InteractionSystem.cpp) | Validation must use the game's rules. |
| Visual terrain, object drawing, camera and area labels | [Graphics.cpp](../../../src/Graphics/Graphics.cpp), [Graphics.h](../../../src/Graphics/Graphics.h) | Geography export alone does not export presentation. |
| Startup catalogue checks | [ContentValidator.cpp](../../../src/Content/ContentValidator.cpp) | Existing checks are not a general authored-map validator. |
| Save path and startup/shutdown policy | [main.cpp](../../../src/main.cpp), [Engine.cpp](../../../src/Core/Engine.cpp), [WorldPlayerPersistenceLifecycle.cpp](../../../src/Persistence/WorldPlayerPersistenceLifecycle.cpp) | Export must not enter the application's persistence lifecycle. |

## 2. Observed terrain construction

The map is **100 × 100 tiles**. Coordinates are integer `(x,y)`, with `(0,0)` at the top left. `Map` creates grass tiles and, when both dimensions exceed 10, sets **(10,10) to water**. `World` then applies the following rectangles in this exact order. Bounds are inclusive, and every overlay skips a tile already marked water. [Sources: Map.cpp](../../../src/World/Map.cpp), [World.cpp](../../../src/World/World.cpp), [DevelopmentWorldContent.cpp](../../../src/World/Development/DevelopmentWorldContent.cpp).

| Order | Material | Minimum `(x,y)` | Maximum `(x,y)` |
|---:|---|---|---|
| 0 | DIRT | (0,2) | (24,3) |
| 1 | STONE | (1,1) | (4,4) |
| 2 | STONE | (19,1) | (22,4) |
| 3 | DIRT | (3,4) | (12,4) |
| 4 | DIRT | (3,4) | (4,7) |
| 5 | DIRT | (6,4) | (7,6) |
| 6 | DIRT | (9,4) | (10,6) |
| 7 | DIRT | (12,3) | (13,6) |
| 8 | STONE | (3,8) | (12,12) |
| 9 | DIRT | (3,6) | (4,8) |
| 10 | STONE | (13,7) | (19,12) |
| 11 | DIRT | (12,5) | (13,8) |
| 12 | DIRT | (19,3) | (20,8) |
| 13 | DIRT | (5,0) | (9,2) |
| 14 | DIRT | (16,0) | (19,2) |

The counts below are derived by replaying the inspected constructor and overlay rules. They are useful independent expectations, not a claim that an imported world has passed parity tests.

| Tile type | Ground before objects | Effective runtime map after objects |
|---|---:|---:|
| GRASS | 9,791 | 9,788 |
| DIRT | 100 | 99 |
| STONE | 108 | 102 |
| WATER | 1 | 1 |
| TREE | 0 | 7 |
| STATION | 0 | 3 |
| WALL | 0 | 0 |
| **Total** | **10,000** | **10,000** |

`Tile` makes grass, dirt and stone walkable; water, tree, wall and station are blocked. The effective baseline therefore has **11 blocked tiles**. NPCs and the player do not mark their positions as blocked map tiles. [Sources: Tile.cpp](../../../src/World/Tile/Tile.cpp), [World.cpp](../../../src/World/World.cpp).

**Integration requirement:** retain ground separately from object occupancy. Exporting only the effective map loses the ground below a moved or deleted object. A future adapter must derive the affected tile from ground plus remaining occupancy. The first export should include all 10,000 ground cells, not just the visibly developed corner.

## 3. Observed resources and stations

Resources are created in the listed order, then stations. `ObjectManager` uses one increasing counter for both groups. Every resource creation writes `TileType::TREE`, including ore; station creation writes `TileType::STATION`. Preserve this effective-map behavior during baseline migration. [Sources: DevelopmentWorldContent.cpp](../../../src/World/Development/DevelopmentWorldContent.cpp), [World.cpp](../../../src/World/World.cpp), [ObjectManager.cpp](../../../src/World/Object/Manager/ObjectManager.cpp).

| Runtime object ID at baseline | Definition | Position | Ground beneath object |
|---:|---|---|---|
| 1 | NORMAL_TREE | (5,5) | GRASS |
| 2 | OAK_TREE | (8,5) | GRASS |
| 3 | WILLOW_TREE | (11,5) | GRASS |
| 4 | COPPER_ROCK | (5,9) | STONE |
| 5 | TIN_ROCK | (8,9) | STONE |
| 6 | IRON_ROCK | (11,9) | STONE |
| 7 | COAL_ROCK | (14,12) | STONE |
| 8 | FURNACE | (14,9) | STONE |
| 9 | CRAFTING_TABLE | (17,9) | STONE |
| 10 | BANK | (20,3) | DIRT |

The placement structures currently contain type and coordinates, with no authored instance ID. Resource gathering requirements, rewards, uses and respawn timing belong to [ResourceDatabase.cpp](../../../src/World/Object/Resource/ResourceDatabase.cpp); depletion is live [ResourceNode](../../../src/World/Object/Resource/ResourceNode.h) state and does not belong in the initial authored snapshot.

**Integration requirement:** assign fixed authored IDs through an explicit baseline registry. The current resource and station types each occur once, so they can identify legacy entries in this bounded export, provided duplicate, missing and unknown mappings fail visibly. A future second tree needs a new placement ID. Moving, renaming or changing an existing placement must not regenerate its ID from its type, coordinates or runtime number.

## 4. Observed actor lifecycle and identity

All ten records in `GetStarterSpawns()` are registered before any actor is created. The constructor creates Mara, then the passive and aggressive monsters. Durable and elite monsters plus the five supporting characters are created after the first successful player registration. The persistence load/create paths invoke the same delayed hooks. [Sources: NpcSpawnDatabase.cpp](../../../src/NPC/NpcSpawnDatabase.cpp), [World.cpp](../../../src/World/World.cpp).

| `NpcSpawnId` | Display name | Position | Wander radius / interval in ticks | Maximum / respawns | Creation hook |
|---|---|---|---|---|---|
| DEVELOPMENT_GUIDE_SPAWN | Mara Vale | (3,3) | 0 / 0 | 1 / false | World construction |
| PASSIVE_DEVELOPMENT_SPAWN | Mossling | (6,1) | 2 / 5 | 1 / true | World construction |
| AGGRESSIVE_DEVELOPMENT_SPAWN | Ashfang | (18,2) | 0 / 0 | 1 / true | World construction |
| DURABLE_DEVELOPMENT_SPAWN | Stoneback | (16,11) | 1 / 7 | 1 / true | First player registration |
| ELITE_DEVELOPMENT_SPAWN | Cinder Warden | (22,3) | 0 / 0 | 1 / true | First player registration |
| TORREN_ASH_SPAWN | Torren Ash | (16,10) | 0 / 0 | 1 / false | First player registration |
| ELIA_REED_SPAWN | Elia Reed | (21,2) | 0 / 0 | 1 / false | First player registration |
| NESSA_THORNE_SPAWN | Nessa Thorne | (4,6) | 0 / 0 | 1 / false | First player registration |
| BRANN_DEEPWELL_SPAWN | Brann Deepwell | (9,10) | 0 / 0 | 1 / false | First player registration |
| SERA_FLINT_SPAWN | Sera Flint | (17,1) | 0 / 0 | 1 / false | First player registration |

Names and combat settings come from [NpcDefinitionDatabase.cpp](../../../src/NPC/NpcDefinitionDatabase.cpp), rather than the spawn records. Monster respawn delays are 8, 10, 14 and 24 ticks for Mossling, Ashfang, Stoneback and Cinder Warden respectively. The spawn flag enables that definition's respawn behavior; it is not itself a timer.

### Creation order is distinct from serialization order

For an ordinary fresh world with no intervening caller-created entities, the separate entity counter assigns Mara **1**, Mossling **2**, Ashfang **3**, player **4**, Stoneback **5**, Cinder Warden **6**, Torren **7**, Elia **8**, Nessa **9**, Brann **10**, Sera **11**. There are three entities before player creation and eleven afterward. The delayed hooks guard against duplicating their starter actors. A second ordinary player is entity 12. [Sources: EntityManager.cpp](../../../src/Entity/Manager/EntityManager.cpp), [World.cpp](../../../src/World/World.cpp).

These numbers describe construction compatibility, not permanent identities. Player loading allocates a fresh runtime ID. Resources and stations use a different counter and can have the same number as an actor.

**Integration requirement:** canonical files may sort placements by durable ID, but must preserve a separate stage and order. A complete baseline ordering is `world_start`: Mara, Mossling, Ashfang, seven resources, three stations; `first_player_registration`: Stoneback, Cinder Warden, Torren, Elia, Nessa, Brann, Sera. The latter stage runs **after** successful player registration. Zero-based keys 0–12 and 0–6 respectively express this without treating the player as authored content. Register the selected spawn catalogue before creating actors.

Typed `World::CreateNpc` and `CreateMonster` compare supplied spawn fields with `NpcSpawnDatabase::TryGet`, including coordinates, type, wander settings, maximum and respawn flag. A later loader needs a coherent selected catalogue; passing edited coordinates through the current functions or removing their checks is insufficient.

## 5. Presentation, quest and service dependencies

### Fixed presentation labels

`Graphics::DrawMap` draws these labels at fixed world-pixel positions. `Graphics::TileSize` is 32 pixels. Labels are not placements and are not moved by changing the corresponding resources or services. [Sources: Graphics.cpp](../../../src/Graphics/Graphics.cpp), [Graphics.h](../../../src/Graphics/Graphics.h).

| Label | World-pixel origin `(x,y)` |
|---|---|
| VILLAGE SQUARE | (34,154) |
| WOODLAND | (112,226) |
| COPPER MINE | (112,414) |
| WORKSHOP | (424,414) |
| TRAINING FIELD | (168,122) |

**Integration requirement:** a later preview or geography edit must account for labels, terrain colors, camera transforms and minimap interpretation. Do not present the read-only geography export as a complete visual snapshot or silently reinterpret pixel label coordinates as tiles.

### Identity and route dependencies

Quests reference compiled NPC types, dialogue IDs and nodes, item types, and prerequisite quest IDs. Character objectives are matched by NPC type in [QuestSystem.cpp](../../../src/Quest/QuestSystem.cpp), not by an authored placement ID. Current definitions are in [QuestDefinitionDatabase.cpp](../../../src/Quest/QuestDefinitionDatabase.cpp).

| Quest | Relevant character/service access |
|---|---|
| Gathering Basics | Mara; normal logs |
| Mining Basics | Mara; copper ore |
| Tracks in the Ash | Mara; talk to Sera |
| Shield or Shelter | Torren; craft a wooden shield; alternate completion with Nessa |
| Fangs at the Gate | Sera; defeat three aggressive development monsters |
| Heart of the Ash | Mara; defeat the elite development monster; alternate completion with Brann |

Mara has TALK and TRADE interactions and references `DEVELOPMENT_GUIDE_SUPPLIES`. The other five friendly characters have TALK and their own dialogue definitions. Elia's banker description does not make her the bank station: the bank is the separate object at `(20,3)`. [Sources: NpcDefinitionDatabase.cpp](../../../src/NPC/NpcDefinitionDatabase.cpp), [DialogueDefinitionDatabase.cpp](../../../src/Dialogue/DialogueDefinitionDatabase.cpp), [ShopDefinitionDatabase.cpp](../../../src/Shop/ShopDefinitionDatabase.cpp).

Existing prose refers to Sera and Ashfangs at the east road; Heart of the Ash describes an “Ashfang alpha” although the target definition is displayed as Cinder Warden. This is observed wording drift for a later content review, not a reason to change identity or quest behavior during export. Tutorial progression also depends on dialogue, logs, planks, both copper and tin, bronze production, equipment, combat, bank contents and Gathering Basics completion; its actual checks are in `World::ProcessTutorialProgress` and its labels in `Graphics`.

The quest table records definitions, not successful end-to-end completion. A separate source-level audit finding is that the inspected death hook calls `QuestSystem::RecordDefeated` only after a cast to `NPC`, while `Monster` derives from `Entity` and `Combatant`, not `NPC`. This mismatch needs a focused gameplay regression check and separately scoped correction; the exporter must not conceal it by claiming quest parity. [Sources: World.cpp](../../../src/World/World.cpp), [Monster.h](../../../src/Entity/Monster/Monster.h), [NPC.h](../../../src/NPC/NPC.h).

**Integration requirement:** retain reachable interaction approaches and review directional prose when geography changes. Duplicating an NPC type has quest consequences even if its new placement ID is unique. Do not assume a displayed role, quest description or tutorial label fully specifies the runtime rule.

## 6. Player creation, saves and location-bearing state

### Observed entry and recovery paths

| Path | Current behavior and owner |
|---|---|
| `World::CreatePlayer()` | Uses `EntityManager::CreatePlayer`; the player's initial `(0,0)` comes from the base `Entity` constructor. This path does not explicitly read the default-spawn constants or validate the map before registration. |
| Missing save | `World::LoadOrCreatePlayerFromFile` validates the development spawn, builds a player with development defaults, sets the spawn, registers it and invokes delayed starter hooks. |
| Existing save | `PlayerSaveState` reconstructs persistent state; `World::TryRegisterLoadedPlayer` checks the saved position. A blocked or out-of-bounds tile falls back to the development spawn with a warning. An invalid fallback prevents registration. |
| Loaded recovery pile | After player registration, `World` restores pile coordinates, items, owner and `currentTick + remainingTicks`. That code does not apply the player's map-validity/fallback check to the pile. |
| Player death respawn | `World::ProcessPlayerRespawns` uses the development-spawn constants and restores health after the recovery delay. It does not perform a map-validity check at that assignment. |
| Monster respawn | `World::ExecuteMonsterRespawn` restores the monster at its stored original spawn and resets runtime activity/wandering. It keeps the existing entity ID. |
| Save capture | `World::SavePlayerToFile` captures a living player's state and, when present, recovery-pile coordinates, items and remaining lifetime. |

Sources: [World.cpp](../../../src/World/World.cpp), [Entity.cpp](../../../src/Entity/Entity.cpp), [EntityManager.cpp](../../../src/Entity/Manager/EntityManager.cpp), [PlayerSaveState.cpp](../../../src/Persistence/PlayerSaveState.cpp). `PlayerSaveState` deliberately leaves map-position validation to world integration. These are separate migration review targets, not evidence that an edited map is already supported.

The player respawn delay is **4 ticks** and a newly created recovery pile lasts **600 ticks**, as declared in [World.h](../../../src/World/World.h). On death, carried inventory is transferred to a pile at the death position when the inventory replacement succeeds. `ProcessPlayerRecoveryPiles` returns its contents atomically when the living owner is within one tile on each axis and has inventory capacity; it also handles expiry. A geometry migration must consider this recovery rule and reachability, independently of the player's fallback position.

The application chooses `saves/development-player.save` relative to the process working directory in [main.cpp](../../../src/main.cpp). The engine adds development equipment only for newly created players and saves through the lifecycle on controlled shutdown. Startup does not normally reserialize a loaded player, but [PlayerSaveFileStore::Load](../../../src/Persistence/PlayerSaveFileStore.cpp) can recover or clean transaction `.tmp` / `.bak` files. A read-only content exporter must therefore avoid the player-save APIs entirely. Future playtests need an explicit isolated save path.

### Historical save formats

[PlayerSaveData.h](../../../src/Persistence/PlayerSaveData.h) declares current version **13**. [PlayerSaveTextCodec.cpp](../../../src/Persistence/PlayerSaveTextCodec.cpp) accepts wire versions **1–13**, checks their historical required fields/catalogues and normalizes successful decodes to version 13. Encoding and `PlayerSaveState` validation require the current canonical version. This is a source inventory of implemented branches, not a claim that historical fixtures were executed during this review.

| Wire version | Historical contract relevant to migration |
|---|---|
| 1 | Player position, health, inventory, skills and equipment; no quest section |
| 2–3 | Gathering Basics only; version 3 includes the counted historical quest catalogue |
| 4 | Adds Mining Basics: two canonical quest records |
| 5 | Adds bank entries |
| 6 | Adds tutorial state |
| 7 | Adds combat style |
| 8 | Adds recovery-pile state |
| 9 | Adds known recipes |
| 10 | Adds Tracks in the Ash: three quests |
| 11 | Adds Shield or Shelter: four quests |
| 12 | Adds Fangs at the Gate: five quests |
| 13 | Adds Heart of the Ash: six quests |

Older quest records are overlaid on current defaults without inventing earned progress. Pre-bank saves receive an empty bank; pre-tutorial saves are marked tutorial complete/skipped; pre-style saves receive ACCURATE; pre-recipe saves receive the starter recipe set. The saved player and recovery-pile positions contain only `(x,y)`, with no region or surface identity. A future loader must define their interpretation explicitly; merely moving a tree does not itself require a new player-save version. See [Save compatibility](SAVE_COMPATIBILITY.md).

## 7. Verification boundary and remaining integration requirements

Existing public observations are sufficient for baseline comparison: [World.h](../../../src/World/World.h) exposes the map, entity/resource/station collections, spawn manager, movement queues, interaction queues and resulting sessions. [NPC.h](../../../src/NPC/NPC.h) and [Monster.h](../../../src/Entity/Monster/Monster.h) expose definition/spawn identity; monsters also expose original spawn and wander settings. No private runtime access is required for the core construction checks.

The first exporter should be checked against a real `World` at construction and after player creation: all 10,000 effective tiles and collision results, every object, all ten registered spawn records, delayed actor creation and runtime ordering. Deterministic JSON and a representative movement/interaction scenario provide further evidence. Exported authored content must stay unchanged after live movement or depletion. These are test targets; execution outcomes must be reported separately.

The following constraints remain before an editable runtime path can be accepted:

1. **Input safety:** `Map::GetTile` indexes directly. Validate dimensions, multiplication limits, row lengths, coordinates and enum references before allocation or tile writes.
2. **Terrain-aware validation:** the current `ContentValidator::ValidateStarterWorldContent` and spawn validator construct a bare `Map`; those paths do not apply the terrain overlays. Their catalogue checks cannot stand in for validating a selected edited terrain grid.
3. **Exact approach reachability:** `Pathfinder` forbids diagonal corner cutting and may return a best-effort partial path. A nonempty path alone does not prove the requested approach is reachable; verify its endpoint. `InteractionSystem` evaluates readiness within one tile on each axis, including diagonals, after validating the actor and target; a validator must combine legal movement with these actual interaction rules.
4. **Preserved lifecycle:** separate canonical serialization from creation order; keep spawn-catalogue registration, player registration and delayed creation distinct.
5. **Selected catalogue:** typed NPC creation must validate against the selected accepted content, while retaining existing type, capacity and lifecycle checks.
6. **All location-bearing state:** review direct player creation, saved position fallback, death respawn, recovery piles, monster homes and later exits/surfaces together. Do not repair geography by discarding player state.
7. **Presentation and progression:** account for fixed labels, directional prose, compiled quest targets, shop access and tutorial routes.
8. **Migration evidence:** parsing, round trips, imported-world parity, edited-world behavior and save/reload comparisons remain separate gates. A successful read-only export proves none of those on its own.

The first slice can leave the existing runtime construction intact. JSON loading, guarded edits, undo/redo, a shared command service, graphical editing, previews, isolated playtest launch, multi-tile footprints and elevation remain subsequent work under the [content contract](CONTENT_MODEL.md) and [Studio design](../04-studio/STUDIO_DESIGN.md).

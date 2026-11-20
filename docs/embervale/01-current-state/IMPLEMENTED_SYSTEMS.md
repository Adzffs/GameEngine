# Original engine inventory and current-state reference

**Original inventory date:** 18 November 2026\
**Repository:** Adzffs/GameEngine\
**Reviewed baseline:** `e9d32932e8f9b27e96283313cca320058792a81f` on `master`\
**Purpose:** Explain what is already present, what it does, and what still needs evidence or development.

## Current state after the 20 November review

Read [CURRENT_STATUS.md](CURRENT_STATUS.md) for current implementation, source versions, checks and next work. The inventory below is pinned to the original baseline. It does not imply that later editor, sprites or gathering animation are unimplemented. The [project review](../06-development/PROJECT_REVIEW_2026-11-20.md) records current fixes and limits.

## How to read this inventory

Embervale already has a substantial local game prototype: an authoritative world simulation, gathering and crafting, melee combat, six named friendly characters, a six-quest starter arc, banking, shops, and versioned player saves. The next development stage should preserve those systems while making their content editable. It should not assume the game is a blank engine.

Throughout this document, **SOURCE VERIFIED** means the behaviour or definition was found in the pinned source. It does not mean a new build, playthrough, or test suite passed during this documentation task. **REMOTE RESULT VERIFIED** describes an observed repository result. **HISTORICAL REPORT** identifies a claim in an earlier PR or document. **PLANNED**, **PROPOSED**, and **UNVERIFIED** keep future work and uncertainty visible.

The original source inventory below is pinned to the engine baseline. Subsequent verification on 18 November reproduced all **81 existing tests passing** locally on Linux with SDL’s dummy driver at documentation merge `c697386`. The [verification record](../06-development/BASELINE_VERIFICATION_2026-11-18.md) contains the new exporter checks and remaining platform limits.

## 1. Baseline at a glance

| Area | Source-verified baseline | Interpretation |
|---|---|---|
| Language and build | C++20, CMake 3.20 minimum, optional SDL3 client | A headless core already exists. |
| World timing | 600 ms tick interval | Rendering and gameplay timing have different responsibilities. |
| Development map | 100 × 100 tiles, default player spawn `(0, 0)` | Most authored starter content occupies a much smaller part of that map. |
| Terrain content | 15 ordered terrain rectangles; a constructor-created water tile at `(10, 10)` | Export must reproduce actual construction rules. |
| Resource placements | 7 | Three tree tiers and four mining resources. |
| Station placements | 3 | Furnace, crafting table, bank. |
| NPC catalogue | 10 definitions | Six friendly characters and four monster types. |
| Quest catalogue | 6 | Two tutorials, two side quests, two main quests. |
| Item catalogue | 23 non-`NONE` entries | Includes one explicitly named developer test weapon. |
| Recipe catalogue | 14 entries | Smelting, forging, planks, and a wooden shield. |
| Skill types | 5 non-`NONE` entries | Attack, Defence, Woodcutting, Mining, Smithing. |
| Player save format | Version 13 | Existing migrations and compatibility behaviour must be preserved. |
| Baseline test targets | 70 headless + 7 graphics + 4 client = 81 | Fresh local baseline pass recorded separately. |
| Read-only world export | Complete 100×100 ground plus 20 placements and lifecycle metadata | Implemented as a separate SDL-free tool; two additional CTest entries. |

Sources: [build definition][IS-CMAKE], [clock][IS-CLOCK], [world dimensions][IS-WORLD-CONTENT-H], [terrain and placements][IS-WORLD-CONTENT], [NPC definitions][IS-NPC], [quest definitions][IS-QUEST], [items][IS-ITEM], [recipes][IS-RECIPE], [skills][IS-SKILLS], [save data][IS-SAVE]. Counts refer to the pinned catalogue lists, excluding sentinel values where stated.

## 2. Engine and simulation foundations

### World ownership and command processing

`GameEngineCore` holds simulation and content logic without requiring SDL when both client and graphics-test options are disabled. `World` owns the map, entities, object manager, action manager, movement and interaction systems, combat service, dialogue and shop systems, scheduling, and player persistence integration. The client executable adds `Engine`, input, and graphics. This separation is a useful starting point for headless Studio validation. [Build][IS-CMAKE] · [World interface][IS-WORLD-H]

The server-command model includes movement, attacking, item use and drop, ground-item pickup, equipment removal, station interaction, crafting, dialogue, shop transactions, quest actions, banking, tutorial skip, and combat-style selection. Commands carry runtime actor identities and, where required, session IDs or expected revisions. The architecture is designed around authoritative decisions rather than accepting client-side results. [Command definitions][IS-COMMAND]

This is an **authority foundation**, not proof of a complete online server. No network transport, authentication, remote-client replication, deployment service, or MMO operational platform was found in the reviewed tree. One concrete boundary still needing an audit is the client calling `Player::ToggleRunning()` directly. Future networking must decide how that request enters the authoritative command path. [Engine][IS-ENGINE] · [Complete tree][IS-TREE]

### Tick ordering, actions, and determinism

The world update explicitly processes queued commands, advances the tick, processes scheduled events and player respawns, resolves movement and interactions, completes timed actions, handles AI and melee engagements, grants rewards, ticks status effects, and advances tutorial state. The order is part of observable gameplay behaviour. An importer or editor preview must not silently change it. [World implementation][IS-WORLD]

Reusable action states, cancellation reasons, lifecycle events, deterministic scheduling, and independent injectable random sources already support controlled simulations. Combat, reward rolls, and gathering use separate random-source injection points. These seams are valuable for exporter parity tests and content smoke scenarios. They do not, on their own, establish deterministic whole-world replay or network rollback. [World implementation][IS-WORLD] · [Test registrations][IS-CMAKE]

## 3. Movement, interaction, and combat

| Capability | Implemented scope | Remaining boundary |
|---|---|---|
| Click-to-move | Tile paths, nearest reachable fallback, path execution checks, movement-system orchestration | Large-region performance and future surface-aware paths need their own evidence. |
| Running | Stamina consumption and recovery, running toggle, movement along visible path segments, L-shaped running work in the merged history | Fresh-player travel measurements and a complete command-boundary review remain necessary. |
| World interaction | Approach and interaction intent for resources, stations, NPCs, monsters, and ground items | Moving content must preserve usable approach tiles and interaction distance. |
| Melee | Combat ratings, attack resolution, timed/repeating engagement, combat styles, hit feedback, retaliation | Ranged combat, magic, party combat, and advanced boss mechanics are future decisions. |
| Monster AI | Aggression, chasing/engagement intent, wandering and spawn metadata | A full behaviour-tree editor or general scripting system is not present. |
| Death and rewards | Entity death events, kill credit, reward tables, monster respawn, player recovery | Editor map changes must account for recovery piles, respawn destinations, and retained world references. |
| Health and effects | Health pool, food healing, timed status-effect framework, equipment-derived stats | A framework is not a complete authored library of buffs, debuffs, or consumables. |

Sources: [movement system][IS-MOVEMENT], [pathfinder][IS-PATH], [player][IS-PLAYER], [world][IS-WORLD], [monster AI][IS-AI], [status-effect manager][IS-STATUS], [merged change history][IS-PR2].

### Existing enemy roles

| Enemy | Source role | Spawn anchor | Design implication |
|---|---|---|---|
| Mossling | Passive introductory monster | `(6, 1)` | Suitable for teaching deliberate combat engagement. |
| Ashfang | Aggressive early threat | `(18, 2)` | Its aggression and leash should influence safe-route placement. |
| Stoneback | Durable progression encounter | `(16, 11)` | Introduces preparation against a sturdier enemy. |
| Cinder Warden | Elite encounter | `(22, 3)` | Serves the current starter-arc climax; not a finished multi-phase boss system. |

These are current development placements, not the proposed River Crossing layout. Runtime creation is also staged: the constructor creates Mara, Mossling, and Ashfang; advanced monsters and the remaining friendly cast are ensured after player creation or registration. Preserve that distinction during baseline export and parity verification. [NPC definitions][IS-NPC] · [spawn catalogue][IS-SPAWNS] · [World][IS-WORLD]

## 4. Gathering, crafting, inventory, and economy

### Gathering and progression

Resources have definitions for required tools and skills, reward production, action timing, success chances, depletion, and respawn behaviour. The current placement set contains normal, oak, and willow trees, plus copper, tin, iron, and coal rocks. Gathering is integrated with the action framework, including cancellation when relevant circumstances change. [Resource catalogue][IS-RESOURCE] · [World][IS-WORLD]

Attack, Defence, Woodcutting, Mining, and Smithing are present as skill types. Equipment and combat style affect derived combat ratings. Recipe production pacing reduces eligible recipe duration as skill mastery rises, with a defined minimum. These systems should become editable through shared definitions gradually; changing every enum-backed catalogue at once would widen the compatibility risk. [Skills][IS-SKILLS] · [Player][IS-PLAYER] · [Production pacing][IS-PACING]

**Fresh-player testing needs a dedicated profile.** Development defaults currently grant a bronze axe and bronze pickaxe; `ENABLE_TEST_PLAYER` is enabled and adds 2,000 Woodcutting XP, 5,000 Mining XP, and 5,000 Smithing XP. This is useful development setup but would distort measurements of a true beginner experience. The roadmap must establish both reproducible developer and fresh-player profiles before approving starter progression times. [Player constructor][IS-PLAYER] · [Development configuration][IS-DEVCONFIG]

### Crafting and inventory

Inventory uses fixed slots, stack rules, quantity validation, and transaction helpers. The crafting implementation includes a nine-slot grid, shaped and shapeless recipe matching, crafting sessions, revision checks, inventory/cursor settlement, and UI integration. Furnace recipes and crafting-table recipes are distinct. There is no separate anvil placement in the current starter station catalogue; the forging recipes currently require the furnace. [Command definitions][IS-COMMAND] · [Recipe catalogue][IS-RECIPE] · [World][IS-WORLD]

The fourteen recipes cover bronze, iron, and steel bars; normal, oak, and willow plank production; wooden shield crafting; bronze, iron, and steel axes and pickaxes; and a bronze sword. Recipe discovery and a player recipe book exist, with discoveries represented in player-save data. Preserve discovery state when migrating recipe identifiers. [Recipe catalogue][IS-RECIPE] · [Recipe book][IS-RECIPEBOOK] · [Save data][IS-SAVE]

### Banking, shops, and item flow

Banking includes player-owned storage, an active session, revision-aware deposit and withdrawal commands, deposit-inventory behaviour, and an interactive bank window. Ground-item drop and pickup and protected recovery piles also exist. These are different storage lifecycles and should remain distinct in documentation and migration logic. [Bank][IS-BANK] · [Commands][IS-COMMAND] · [World][IS-WORLD]

Mara's supplies shop is the only registered shop. Its entries cover selected raw materials, food, a healing tonic, and basic equipment; some materials have a sell price but no buy price. Shop transactions are mediated through world/session logic. The existing Economy Ledger describes progression and money-flow intentions, but its target timings are design targets until measured with current content and a true fresh-player profile. [Shop catalogue][IS-SHOP] · [Economy Ledger][IS-ECONOMY]

### Priority integration risk: a source-derived profitable crafting cycle

The current numbers imply a repeatable buy–craft–sell opportunity: buy **3 logs for 3 coins**, turn them into **12 wooden planks**, craft **2 wooden shields**, and sell those shields for **24 coins**. The arithmetic gives **21 coins of net gain** before travel and action time. The plank and shield recipes are both in the starter recipe book, and their current definitions have no skill requirement. This is a **source-derived finding**, not a runtime-replayed exploit or a measured income rate. Verify the whole authoritative transaction sequence, then make a deliberate price/source/sink correction and add a multi-step economy regression check. Track this early under `GAME-008`; testing only recipes whose immediate ingredients are shop-traded can miss a chain whose intermediate material is untraded. [Shop catalogue][IS-SHOP] · [Recipe catalogue][IS-RECIPE] · [Starter recipe book][IS-RECIPEBOOK]

## 5. Characters, dialogue, quests, and tutorial

### Friendly cast

| Character | Defined role | Current anchor | Existing mechanical connection |
|---|---|---|---|
| Mara Vale | Warden and quartermaster | `(3, 3)` | Tutorial, starter quests, supplies trade. |
| Torren Ash | Smith and workshop keeper | `(16, 10)` | Forging guidance and the crafting-choice quest. |
| Elia Reed | Banker and record keeper | `(21, 2)` | Banking and recovery guidance; character-world context. |
| Nessa Thorne | Forester and herbalist | `(4, 6)` | Gathering guidance and alternate quest completion. |
| Brann Deepwell | Miner and surveyor | `(9, 10)` | Mining context and alternate conclusion reward. |
| Sera Flint | Hunter and road scout | `(17, 1)` | Investigation objective and combat quest. |

The source includes names, roles, wants, voice descriptions, relationships, and examine text. The current character document also states visual intentions. Defined silhouette or colour notes should not be mistaken for a completed imported sprite set. [NPC definitions][IS-NPC] · [spawn catalogue][IS-SPAWNS] · [Starter Characters][IS-CHARACTERS]

### Quest catalogue

| Quest | Objective in the current definition | Availability and reward |
|---|---|---|
| Gathering Basics | Gather 10 logs | Initially available; 10 coins. |
| Mining Basics | Gather 5 copper ore | Initially available; 10 coins. |
| Tracks in the Ash | Speak to Sera | Initially available; 15 coins. |
| Shield or Shelter | Craft a Wooden shield; choose reward contact and keep the shield | Follows Tracks in the Ash; 25 coins through Torren or 3 cooked meat through Nessa. |
| Fangs at the Gate | Defeat 3 Ashfangs | Follows Shield or Shelter; 5 cooked meat. |
| Heart of the Ash | Defeat the Cinder Warden and report the victory | Follows Fangs at the Gate; 75 coins through Mara or 2 steel bars through Brann. |

This table records definitions, not a newly completed end-to-end quest playthrough. The quest system supports progress, prerequisites, journal state, dialogue integration, alternate completions, and persistence. The later GAME-007 continuity revision applies Cinder Warden consistently and explains recorded-progress rewards, kept equipment and the two alternative contacts. Current quest details and post-tutorial guidance are implemented; [dated implementation and validation](../06-development/QUEST_CONTINUITY_2026-11-19.md). [Quest definitions][IS-QUEST] · [NPC definitions][IS-NPC]

Tutorial state advances through meeting the guide, gathering, planks, mining, smelting, weapon crafting and equipping, safe combat, loot and banking, and a final choice. Completion and skip state are persisted. The interface includes quest guidance and progression notifications, but onboarding usability still requires observation with new players. [Tutorial state][IS-TUTORIAL] · [World][IS-WORLD] · [Graphics][IS-GRAPHICS]

## 6. World representation and the current visual client

The map is flat and tile-based. Grass, dirt, and stone are traversable tile types; water, tree, wall, and station tile types block movement. Resource creation writes `TREE` to the map even for mining rocks. Station creation writes `STATION`. These names currently serve occupancy as well as appearance. Consequently, removing or moving an object requires reconstruction of the underlying ground; exporting the final occupied tile grid as pure terrain would lose that distinction. [Map][IS-MAP] · [Tile][IS-TILE] · [World][IS-WORLD]

At the original baseline the client drew procedural shapes/text without imported sprites. Later work adds eight editable batches, a renderer-owned cache, generated geometry, primitive fallback and eight-way walking/chopping/mining. A general asset importer remains open. Graphics still owns many responsibilities; targeted extraction is ENG-006. Read [current status](CURRENT_STATUS.md) and [Art and audio](../05-game/ART_AND_AUDIO.md) for current integration. [Graphics implementation][IS-GRAPHICS] · [Graphics interface][IS-GRAPHICS-H] · [Tree][IS-TREE]

The client already has camera smoothing, zoom-related transforms, a local minimap, an adjustable/collapsible side-panel presentation, contextual interaction feedback, and combat and progression feedback. These are source features, not a guarantee of visual quality on every screen size. There is no implemented higher/lower-ground topology, walk-under bridge system, or multi-floor building system in the reviewed representation. [Graphics][IS-GRAPHICS] · [World interface][IS-WORLD-H]

## 7. Saves and lifecycle guarantees

The current save model includes position, health, inventory slots, skill XP, equipment, quest state/progress, bank contents, tutorial state, selected combat style, known recipes, and a recovery pile. The codec recognises historic versions and migrates successful legacy reads to the current representation. Runtime entity IDs are assigned again rather than stored as permanent identities. [Save data][IS-SAVE] · [Codec][IS-CODEC] · [Persistence policy][IS-PERSISTENCE-DOC]

The application uses `saves/development-player.save` relative to its working directory. It loads or creates the player at startup and saves on controlled shutdown. Missing saves are not written merely by startup. Structured failure reporting propagates through the lifecycle. The file store uses temporary and backup files, verifies written content, and includes recovery handling. [Entry point][IS-MAIN] · [File store][IS-FILESTORE] · [Persistence policy][IS-PERSISTENCE-DOC]

These safeguards do not make saves crash-proof. Periodic autosave, forced-termination guarantees, and physical-media durability beyond the existing implementation are not part of the documented lifecycle. Invalid saved player coordinates fall back to the development spawn if that fallback is valid. Recovery-pile coordinates and respawn destinations need explicit treatment when maps become editable; player-position fallback alone does not define the whole content migration policy. [Persistence policy][IS-PERSISTENCE-DOC] · [World][IS-WORLD]

## 8. Validation and tests

Startup content validation and catalogue validators already cover a broad group of items, resources, recipes, rewards, NPCs, spawns, shops, dialogue, and quests. Reuse their semantics through a content-provider boundary where needed. Creating a separate editor validator that disagrees with runtime validation would be a regression. [Validator][IS-VALIDATOR] · [Entry point][IS-MAIN]

The original pinned baseline defines 81 test executables when all relevant options are enabled: 70 headless, seven graphics, and four client tests. These cover many gameplay and persistence concerns, but an executable count says nothing about assertion count, code coverage, untested branches, or current pass rate. [CMake][IS-CMAKE]

Local CMake builds support headless and SDL3 client configurations. CTest labels separate headless, client and graphics coverage; use SDL’s dummy display driver for the recorded noninteractive client and graphics tests. See [Testing](../06-development/TESTING.md).

**Executed result:** the runtime-import slice passed 77/77 headless and 89/89 Linux SDL tests; see the [runtime verification record](../06-development/RUNTIME_IMPORT_VERIFICATION_2026-11-18.md). Earlier 81-test baseline and 83-entry exporter results remain in the [baseline record](../06-development/BASELINE_VERIFICATION_2026-11-18.md). Windows execution and visible manual playtesting remain unverified.

## 9. Implemented bounded tools and remaining future work

Export/import, schemas 1-4, move/paint/place/remove/spawn settings, history, Python/Tk editing, guarded new drafts, dirty-state/reopen behavior, isolated playtests and overlays are implemented. Current checks pass; see [current status](CURRENT_STATUS.md).

General region/definition authoring, interruption recovery, versioned/in-place publication, general asset importing/editing, richer brushes, elevation and an online service remain future work. Next is REL-004 integration and fresh-player acceptance, not reimplementing the delivered tree move/editor slice.

## Related documents

- [Repository guide](REPOSITORY_GUIDE.md) explains where code and existing documents live.
- [Development history](DEVELOPMENT_HISTORY.md) records the delivered sequence and commit history.
- [Evidence ledger](EVIDENCE_LEDGER.md) defines review scope and evidence limits.

[IS-CMAKE]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/CMakeLists.txt
[IS-CLOCK]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Time/Clock.h
[IS-WORLD-CONTENT-H]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Development/DevelopmentWorldContent.h
[IS-WORLD-CONTENT]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Development/DevelopmentWorldContent.cpp
[IS-NPC]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/NPC/NpcDefinitionDatabase.cpp
[IS-SPAWNS]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/NPC/NpcSpawnDatabase.cpp
[IS-QUEST]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Quest/QuestDefinitionDatabase.cpp
[IS-ITEM]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Item/ItemDatabase.cpp
[IS-RECIPE]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Recipe/RecipeDatabase.cpp
[IS-SKILLS]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Skills/SkillType.h
[IS-SAVE]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Persistence/PlayerSaveData.h
[IS-WORLD-H]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/World.h
[IS-WORLD]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/World.cpp
[IS-COMMAND]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Command/ServerCommand.h
[IS-ENGINE]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Core/Engine.cpp
[IS-TREE]: https://github.com/Adzffs/GameEngine/tree/e9d32932e8f9b27e96283313cca320058792a81f
[IS-MOVEMENT]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Movement/MovementSystem.cpp
[IS-PATH]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Pathfinding/Pathfinder.cpp
[IS-PLAYER]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Player/Player.cpp
[IS-AI]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/AI/MonsterAISystem.cpp
[IS-STATUS]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/StatusEffect/StatusEffectManager.cpp
[IS-RESOURCE]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Object/Resource/ResourceDatabase.cpp
[IS-PACING]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Recipe/RecipeProductionPacing.h
[IS-DEVCONFIG]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Core/DevelopmentConfig.h
[IS-RECIPEBOOK]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Recipe/RecipeBook.cpp
[IS-BANK]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Bank/Bank.h
[IS-SHOP]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Shop/ShopDefinitionDatabase.cpp
[IS-ECONOMY]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/docs/EconomyLedger.md
[IS-CHARACTERS]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/docs/StarterCharacters.md
[IS-TUTORIAL]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Tutorial/TutorialState.h
[IS-GRAPHICS]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Graphics/Graphics.cpp
[IS-GRAPHICS-H]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Graphics/Graphics.h
[IS-MAP]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Map.cpp
[IS-TILE]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Tile/Tile.cpp
[IS-CODEC]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Persistence/PlayerSaveTextCodec.cpp
[IS-PERSISTENCE-DOC]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/docs/DevelopmentPlayerPersistence.md
[IS-FILESTORE]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Persistence/PlayerSaveFileStore.cpp
[IS-MAIN]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/main.cpp
[IS-VALIDATOR]: https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Content/ContentValidator.cpp

[IS-PR2]: https://github.com/Adzffs/GameEngine/pull/2

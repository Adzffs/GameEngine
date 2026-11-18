# Content catalogue and expansion register

**Status:** Source inventory at commit `e9d32932e8f9b27e96283313cca320058792a81f`, with explicitly proposed follow-up work.  
**Reviewed:** 18 November 2026.  
**Scope:** Authored definitions and current development placements. This is not a fresh runtime or balance test.

## 1. How to read the catalogue

A **definition** says what a type of content is. A **placement** says where it starts. A **runtime instance** has changing state such as health, depletion or a current position. These counts must remain separate. Ten spawn records do not prove that ten NPC instances exist at every moment; world startup and player creation have different lifecycle paths. See [save compatibility](../03-engine/SAVE_COMPATIBILITY.md) before changing either identities or locations.

| Category | Source-verified count | Meaning |
|---|---:|---|
| Registered item types | 23 | Includes Developer Godsword; excludes the `NONE` sentinel |
| Resource definitions | 7 | Three trees and four rock types |
| Current resource placements | 7 | One of each resource type in the development map |
| Current station placements | 3 | Furnace, crafting table, bank |
| Registered NPC types | 10 | Six friendly characters and four monster types |
| Starter spawn records | 10 | A separate monster-only view repeats four of these records |
| Quest definitions | 6 | Two tutorials and a linked conversation/crafting/combat sequence |
| Recipe definitions | 14 | Smelting, planks, shield and equipment |
| Shop definitions | 1 | Mara's development supplies shop |
| Skill enum entries used as skills | 5 | Attack, Defence, Woodcutting, Mining, Smithing; `NONE` excluded |

Sources: [items](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Item/ItemDatabase.cpp), [resources](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Object/Resource/ResourceDatabase.cpp), [placements](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Development/DevelopmentWorldContent.cpp), [NPC types](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/NPC/NpcDefinitionDatabase.cpp), [spawns](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/NPC/NpcSpawnDatabase.cpp), [quests](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Quest/QuestDefinitionDatabase.cpp), [recipes](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Recipe/RecipeDatabase.cpp), [shops](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Shop/ShopDefinitionDatabase.cpp), [skills](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Skills/SkillType.h).

## 2. Current resources and stations

Coordinates below belong to the existing development world. They are not the River Crossing proposal. Skill requirements come from resource definitions; minimum gathering skill is distinct from the level required to use an upgraded tool.

| Resource | Skill requirement | Output | Current position |
|---|---|---|---|
| Normal tree | Woodcutting 1 | Log | `(5,5)` |
| Oak tree | Woodcutting 5 | Oak log | `(8,5)` |
| Willow tree | Woodcutting 10 | Willow log | `(11,5)` |
| Copper rock | Mining 1 | Copper ore | `(5,9)` |
| Tin rock | Mining 1 | Tin ore | `(8,9)` |
| Iron rock | Mining 10 | Iron ore | `(11,9)` |
| Coal rock | Mining 20 | Coal | `(14,12)` |

| Station | Current position | Current design role |
|---|---|---|
| Furnace | `(14,9)` | Smelting and the current forging recipes |
| Crafting table | `(17,9)` | Planks and wooden shield |
| Bank | `(20,3)` | Storage service |

Sources: [ResourceDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Object/Resource/ResourceDatabase.cpp), [DevelopmentWorldContent.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Development/DevelopmentWorldContent.cpp) and [RecipeDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Recipe/RecipeDatabase.cpp).

An anvil may be a useful future visual or station concept, but it is not one of the three current station placements. Do not add an “anvil migration” task that assumes an existing runtime station. Give any new station its own definition, interaction contract, recipe changes and migration tests.

## 3. Starter cast and encounters

| Character | Current spawn | Existing role | Placement implication for River Crossing |
|---|---|---|---|
| Mara Vale | `(3,3)` | Warden, quartermaster, dialogue, supplies shop | Near arrival and a clear onward route |
| Torren Ash | `(16,10)` | Smith and workshop keeper | Near processing activity, with room to approach |
| Elia Reed | `(21,2)` | Banker and record keeper | Near the bank; distinguish the character from the bank station |
| Nessa Thorne | `(4,6)` | Forester and herbalist | At the managed woodland edge |
| Brann Deepwell | `(9,10)` | Miner and surveyor | Near the safe approach to the quarry |
| Sera Flint | `(17,1)` | Hunter and scout | Beside the outward road before its danger boundary |
| Mossling | `(6,1)` | Passive development monster | Optional introductory encounter |
| Ashfang | `(18,2)` | Aggressive monster | Clearly marked threat beyond safe services |
| Stoneback | `(16,11)` | Durable monster | Optional material-route challenge |
| Cinder Warden | `(22,3)` | Elite monster | Deliberate later encounter with a return route |

The first three columns are source inventory; the final column is proposed design. Sources: [NPC definitions](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/NPC/NpcDefinitionDatabase.cpp), [NPC spawns](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/NPC/NpcSpawnDatabase.cpp).

Keep a separate identity for an NPC type, a spawn record and a live entity. Moving Mara's authored spawn should not change her quest identity. Editing an enemy spawn must also consider wander bounds, aggression, leash, population cap, respawn policy and save assumptions. A decorative danger circle is not evidence that all actual pursuit paths are safe.

## 4. Items grouped by purpose

| Group | Registered content | Review focus |
|---|---|---|
| Timber | Log, Oak log, Willow log, Wooden planks | Useful outputs and inventory pressure |
| Ore and fuel | Copper ore, Tin ore, Iron ore, Coal | Paired inputs, access and progression requirements |
| Processed metal | Bronze bar, Iron bar, Steel bar | Actual equipment sinks and travel costs |
| Tools | Bronze/Iron/Steel axe; Bronze/Iron/Steel pickaxe | Upgrade paths, use requirements and recovery availability |
| Combat equipment | Bronze sword, Wooden shield | Understandable early benefit |
| Consumables | Cooked meat, Healing tonic | Healing, cost, stackability and encounter pacing |
| Currency | Coins | Repeat purchases and measured income/sinks |
| Debug content | Developer Godsword | Development-only separation and release checks |

Source: [ItemDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Item/ItemDatabase.cpp). This inventory does not establish that every acquisition path is currently healthy. Keep the debug item visible to developers in the catalogue while excluding it from normal player progression.

For each future item, add an intended player use before authoring its stats. Require at least one reachable acquisition route, one intended use, a clear icon or placeholder, explicit stack behaviour and handling for full inventory. If it is required for recovery, test that a player who loses their current copy can obtain another without already possessing the same capability.

## 5. Recipe inventory

| Recipe group | Definitions | Main input/output relationship |
|---|---|---|
| Smelting | Bronze bar, Iron bar, Steel bar | Copper + Tin → Bronze; Iron ore → Iron; Iron ore + Coal → Steel |
| Timber conversion | Wooden planks, Oak planks, Willow planks | Each recipe produces the existing Wooden planks item |
| Shield | Wooden shield | Six Wooden planks → one shield |
| Axes | Bronze axe, Iron axe, Steel axe | Matching bars plus Wooden planks |
| Pickaxes | Bronze pickaxe, Iron pickaxe, Steel pickaxe | Matching bars plus Wooden planks |
| Sword | Bronze sword | Bronze bars plus Wooden planks |

There are 14 registered recipe definitions. The “Oak planks” and “Willow planks” recipe names do not imply separate oak-plank and willow-plank item types. Current forging recipes use `FURNACE`. Source: [RecipeDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Recipe/RecipeDatabase.cpp).

Recipe editor work should preserve station requirements, quantities, skill rules, production pacing, shaped or shapeless patterns, unlock behaviour and output-capacity handling. Externalising a recipe catalogue is a distinct migration from exposing its values in a GUI. Validate one real recipe through the authoritative system before broadening the editor.

## 6. Quest inventory and progression links

| Quest | Current objective | Prerequisite | Important continuity check |
|---|---|---|---|
| Gathering Basics | Gather 10 Logs | None | Preserve objective and reward semantics during map changes |
| Mining Basics | Gather 5 Copper ore | None | Copper must be reachable without an advanced tool |
| Tracks in the Ash | Speak to Sera Flint | None | Update objective directions when Sera moves |
| Shield or Shelter | Craft a Wooden shield; alternative completion path | Tracks in the Ash | Both completion NPCs must remain reachable |
| Fangs at the Gate | Defeat three Ashfangs | Shield or Shelter | Safe preparation and return route |
| Heart of the Ash | Defeat the elite monster; alternative completion path | Fangs at the Gate | Resolve elite name inconsistency before content polish |

Source: [QuestDefinitionDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Quest/QuestDefinitionDatabase.cpp). The current elite objective text calls the target an Ashfang alpha, while the referenced elite NPC definition displays Cinder Warden. Record a deliberate naming decision and update all relevant text together; changing only a label can leave a misleading objective.

Future quest specifications should include initial state, prerequisites, objective event, required amount, giver, completion locations, alternate completions, rewards, repeatability, failure/recovery behaviour and save-version implications. Test reconnect or reload at meaningful intermediate states. Dialogue must not provide a second path that grants the same one-time reward repeatedly.

## 7. Documentation discrepancies found in the baseline

| Existing document statement | Source evidence | Required action |
|---|---|---|
| Iron ore row says bars have no equipment sink | Iron and Steel axe/pickaxe recipes are present | Update the economy explanation to describe current sinks |
| Bronze sword row calls it a Gathering Basics requirement | Gathering Basics uses a Log objective | Correct the ledger, or propose a separately reviewed quest change |
| Economy implementation list still asks for equipment recipes | Those recipes already exist in the catalogue | Reclassify as source-implemented and leave playtest validation open |
| Final quest refers to an Ashfang alpha | Elite NPC display name is Cinder Warden | Agree the intended encounter name, then synchronise text |

Sources: [EconomyLedger.md](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/docs/EconomyLedger.md), [RecipeDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Recipe/RecipeDatabase.cpp), [QuestDefinitionDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Quest/QuestDefinitionDatabase.cpp). These findings describe the reviewed baseline. The corrected [economy ledger](../../EconomyLedger.md) included in this documentation pack reconciles its prose with current source; the quest naming decision remains open. Gameplay definitions are unchanged.

The ledger also records a source-derived multi-stage profit cycle: three purchased Logs can yield twelve planks, two shields and 24 coins of sale value for three coins of input. This is a calculation from the current definitions, not a completed runtime reproduction. See the [ledger](../../EconomyLedger.md) and planned `GAME-008` investigation.

## 8. Expansion register

| Candidate | Status | Why it might help | Evidence needed before commitment |
|---|---|---|---|
| More starter resource placements | Proposed | Reduce waiting and support geographic clusters | Travel, depletion and upgrade measurements |
| A larger distributed River Crossing | Proposed | Improve place identity and route choices | First-time navigation and complete starter loop |
| Additional decorative buildings | Proposed | Explain settlement functions | Readability, footprint and route validation |
| A distinct new material sink | Future option | Keep older tiers useful | Existing item-use telemetry and a new meaningful decision |
| Another region or dungeon | Future option | Extend exploration and progression | Stable exits, return journeys, saves and content packaging |
| Companion collection | Long-term aspiration | A persistent personal goal | Explicit lifecycle, acquisition, use and persistence design |
| More bosses | Long-term aspiration | Deeper preparation and combat choices | Existing combat readability and recovery quality |

Do not treat this register as automatic scope for the first Studio release. Each accepted addition needs a small feature specification and a backlog entry with an observable outcome.

## 9. Maintaining an authoritative catalogue

The first version is a manually reviewed inventory. Once Studio has a read-only exporter, generate IDs, counts, positions and references from the same definitions the game uses. Keep design intent and balance rationale as prose beside that generated data. Avoid two hand-maintained numerical catalogues that inevitably disagree.

For every update, record the source commit, schema/content version where available, additions, removals, ID migrations and the validation report. Do not reset quest or item IDs to make the table look tidy. Keep retired IDs visible in the migration registry and link them to the replacement or explicit removal policy.

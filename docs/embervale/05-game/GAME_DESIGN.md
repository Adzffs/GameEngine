# Embervale game design

**Custom player appearance, 22 November — planned:** The player should be able to choose a coherent body/skin/hair/clothing appearance, keep it while using authoritative equipment and retain it after save/reload. Use the [custom-character design](CUSTOM_CHARACTERS.md) for the staged scope, fit/layer model, default, creator Apply/Cancel and acceptance. The [first source proof](../../../assets/embervale/custom-character-pilot/README.md) is reference art with improvements open; no game character creator, appearance persistence, cosmetic item economy or new gameplay equipment is implemented by that proof.

**Current implementation/evidence:** See [current status](../01-current-state/CURRENT_STATUS.md) and the [20 November review](../06-development/PROJECT_REVIEW_2026-11-20.md). Current starter replay reaches the prepared return at tick 454, while human timing and full combat/recovery acceptance remain open. Planned design goals below are not claims of measured completion.


**Document status:** Living design specification. Existing mechanics are identified explicitly; future designs remain proposals.\
**Reviewed:** 18 November 2026.\
**Original source baseline:** `e9d32932e8f9b27e96283313cca320058792a81f`.\
**Purpose:** Explain the experience the engine and tools are being built to support.

## 1. The intended experience

Embervale is a tile-based RPG in which a player learns a small frontier region, develops useful skills, makes equipment, meets recurring characters and chooses when to venture into danger. The user has chosen a balanced starter area: exploration, gathering, crafting and combat should all contribute to the opening experience. Settlements should grow along geography and routes, with services distributed among several places. There is no central service core through which every activity must pass.

The first complete release target is a coherent, enjoyable local adventure. Multiplayer and a much larger world are longer-term ambitions. An authoritative simulation is a valuable technical foundation, but its presence does not establish a deployed multiplayer game. The current source has a playable development-world structure; the new River Crossing layout and Studio editors are planned work. See the [implemented systems inventory](../01-current-state/IMPLEMENTED_SYSTEMS.md) and [master roadmap](../02-roadmap/MASTER_ROADMAP.md).

### Design commitments

| Commitment | What it means in practice | What to inspect in a playtest |
|---|---|---|
| A place worth learning | Roads, water, woodland and working areas help the player remember locations | Can the player return to a service after one visit? |
| Several useful activities | Gathering, processing, trading, quests and combat have visible uses | Can the player explain why they chose the next activity? |
| Progress through decisions | An upgrade changes a useful option, not only a number | Does new equipment affect the player's plan? |
| Clear consequences | The player understands blocked movement, insufficient requirements, damage and recovery | Can they explain a failed action without reading source code? |
| A distributed settlement | Services fit their local work and connect by loops | Is one junction becoming a compulsory return point? |
| Respect for player time | Travel provides orientation and choices; repetition is measured | Which trips are meaningful and which feel like chores? |
| Tool-assisted iteration | Designers can make and validate content changes without rewriting placement code | Can a small map change be reviewed and replayed safely? |

These commitments are product requirements. The numerical targets below are test hypotheses and may change with evidence.

## 2. Existing gameplay foundation

The source catalogue contains Woodcutting, Mining, Smithing, Attack and Defence. It contains resource definitions for three tree tiers and four rock types, processing and equipment recipes, friendly NPCs, monster definitions, dialogue and quests. These are source-verified building blocks, not a claim that the overall game is balanced or that this documentation run executed them. The exact catalogues are recorded in [Content catalogue](CONTENT_CATALOGUE.md). Sources: [skills](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Skills/SkillType.h), [resources](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Object/Resource/ResourceDatabase.cpp), [recipes](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Recipe/RecipeDatabase.cpp), [quests](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Quest/QuestDefinitionDatabase.cpp).

The development map is a 100×100 grid. Current resources, stations and NPCs occupy a relatively compact portion of it. Their present coordinates are migration inputs, not the desired permanent geography. Preserve that world while the content pipeline is proven; develop River Crossing as a separate candidate. Sources: [world definitions](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Development/DevelopmentWorldContent.h), [placement data](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Development/DevelopmentWorldContent.cpp).

## 3. The core play loops

### Gather, process and improve

The player identifies a useful resource, checks the tool and skill requirement, gathers an amount they can carry, chooses a processing route and uses the result. A visible next upgrade gives the trip a purpose. The workshop must make its inputs, outputs and requirements understandable before the player commits to repeated travel.

The first timber and bronze loops should be comprehensible with current content. Avoid adding five new resources before existing materials have clear uses. A new material earns its place by producing a new decision: a different route, an alternative recipe, a useful trade-off, or a reason to revisit an older area.

### Prepare, fight and recover

Combat begins with a readable target, an opportunity to prepare and a route to withdraw. A passive encounter teaches the basic interaction. Aggressive threats belong beyond an identifiable change in the environment, not beside the first bank or on the only route to beginner resources. Food, equipment and recovery instructions should be discoverable before the player needs them urgently.

Death and recovery must be tested on the actual geography. A technically reachable recovery location can still be frustrating if it repeatedly exposes an unequipped player to the same enemy. Design reviews therefore need both path validation and a human recovery playthrough.

### Meet, investigate and choose

Recurring characters connect activities to the settlement. Quests should help the player notice routes, resources, relationships and threats. Existing quest definitions include gathering, crafting, conversation, combat and alternative completion rewards. Future quests should reuse those supported actions before requiring a new quest system. Source: [quest definitions](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Quest/QuestDefinitionDatabase.cpp).

A quest reward should resolve or advance a local purpose. Avoid sending a player across the same road repeatedly just to acknowledge completion. Where practical, let the return destination be on the route to their next activity.

## 4. Opening session design

Use the following sequence as a proposed playtest script, not a rigid tutorial rail or a promised session duration.

| Step | Intended learning | Evidence of success | Common failure to watch |
|---|---|---|---|
| Arrive | Move, recognise the road and locate help | Player reaches a friendly interaction without verbal coaching | Interface obscures the route or initial objective |
| Try an activity | Understand one resource interaction and feedback | Player can predict what a second attempt will do | Silent requirement failure or unclear depletion |
| Make a useful thing | Connect raw material to a station and output | Player deliberately chooses a recipe | Requirements become visible only after a long trip |
| Manage a trip | Learn inventory, banking and supplies | Player chooses what to carry and why | Banking becomes compulsory after every small action |
| Take a risk | Read an encounter and withdraw or commit | Player notices the danger boundary | Aggression reaches the safe arrival or mandatory route |
| Choose the next goal | See at least two worthwhile options | Player names their next activity without prompting | Every objective points back to one central NPC |

The [economy ledger](../../EconomyLedger.md) targets a first deliberate upgrade in 10–15 minutes. Retain that as an initial design target and measure it; it is not an observed result. Record separate times for experienced testers and people seeing the game for the first time. First establish an explicit starter profile: current development defaults grant extra skill XP, so they cannot stand in for beginner progression measurements. See the [ledger's starter-profile requirement](../../EconomyLedger.md#starter-profile-requirement). Source: [EconomyLedger.md](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/docs/EconomyLedger.md).

## 5. Progression and economy rules

Progression has three linked costs: the actions themselves, inventory constraints and travel. A balance calculation that measures only station production cannot establish the time required for an upgrade. Record total time from deciding on the upgrade to equipping it, including failed gathering attempts, banking, navigation and requirement discovery.

Older tiers should retain a practical use. Current plank recipes already give timber tiers different conversion outputs; do not create a separate plank item for every tree solely to make the inventory larger. The source recipes should be the numerical authority, while the economy ledger explains the intended player decision. Source: [RecipeDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Recipe/RecipeDatabase.cpp).

For each added economic feature, document its sources, sinks, expected acquisition route and recovery route. Check buy–craft–sell cycles using actual shop and recipe semantics. Keep debug equipment out of ordinary rewards and shops. Rare drops may accelerate a goal; they should not be the only practical way to regain an essential starter tool.

Do not claim an economy is healthy merely because it has no arbitrage. Players also need reasons to spend, choices between activities and a tolerable amount of repetition. Measure food consumption, unused materials, coins earned and spent, and abandonment of intended upgrades.

## 6. World structure and content density

River Crossing should function as a settlement spread along useful routes. A bank near a crossing, a workshop near material roads, a forester beside managed woodland and a scout near a threatened road form a believable network. The important property is how routes connect, not geometric symmetry. See the [proposed tile blockout](RIVER_CROSSING.md).

Every landmark needs at least one job: orientation, activity, story, a route decision or a view that makes the next area legible. Decorative density should support those jobs. Empty ground is acceptable when it provides sightlines, room to turn or a change in pace. Repeated bare travel with no decision is a signal to compress or enrich the route.

Counts are a starting hypothesis. More trees can reduce waiting but also shorten progression, crowd click targets and make the ground harder to read. More enemies can create pressure but may remove the safe recovery route. Add content in measured batches and compare the same opening-session script before and after.

## 7. Narrative and character continuity

Preserve the existing starter cast and each character's mechanical role. Their relationships should appear in optional dialogue and small choices as well as the main quest sequence. Current character metadata and the older character document sometimes use slightly different wording; edit them together when a decision changes. Source: [NPC definitions](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/NPC/NpcDefinitionDatabase.cpp) and [StarterCharacters.md](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/docs/StarterCharacters.md).

**Cinder Warden** is the canonical elite encounter name. Heart of the Ash,
Brann's dialogue, objective labels and the existing NPC name now agree. Quest
completion acknowledges recorded progress: Shield or Shelter keeps the player's
shield, and Heart of the Ash uses a victory report rather than a trophy item.
Both reward contacts complete the same quest. Directions resolve named people
and services in the loaded world. [Continuity decision, current rewards and
remaining acceptance](../06-development/QUEST_CONTINUITY_2026-11-19.md).

## 8. Accessibility and interaction expectations

Communicate important states with text or shape as well as colour. Interaction targets should remain distinguishable at the supported zoom levels. A tree canopy must not hide the tile a player needs to click, and a decorative doorway must not suggest an available entrance when none exists. Offer clear feedback when movement or interaction fails.

Record expectations for keyboard access, readable text, reduced camera effects, sound controls and alternative cues as product requirements to implement and test. Do not mark them complete because the current interface provides one related feature. Include testers who do not already know the controls, and capture where they needed help.

## 9. Future expansion boundaries

| Expansion | Admission condition | First bounded proof |
|---|---|---|
| A second region | River Crossing routes and progression hold up in playtests | One exit, one return path, safe save/load across the boundary |
| Elevation and upper floors | Content identity and flat movement migration are stable | An isolated test area with explicit transitions |
| Additional boss encounters | Existing combat signals, preparation and recovery are understandable | One encounter with measurable counterplay |
| Companions or collectibles | Their player purpose and persistence cost are defined | One companion with a small, explicit lifecycle |
| Multiplayer | The game loop is worthwhile and networking requirements are agreed | A narrow authoritative session test, not a whole MMO |

These are future design directions, not commitments to a release date or assertions of existing implementation. The roadmap must allow evidence to change their order.

## 10. How to expand this design

For a new system, write the player problem, one concrete use case, the minimum supported behaviour, dependencies, save impact and a playtest that could disprove the idea. Assign a planning ID through the central backlog. Link the implementation PR and test evidence when the system ships. Update the content catalogue when definitions change, and this document only when the intended experience changes.

The design is ready for its first serious content pass when a new tester can complete one gathering-to-upgrade loop, understand one combat encounter, find the important services and describe a next goal. That outcome matters more than the number of windows in Studio or the number of places drawn on the map.

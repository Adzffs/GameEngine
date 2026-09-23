# Game Design Document

Persistent 2D RPG with grid crafting, collectible pets, and boss loot

A living design brief for turning the current C++ game-engine foundations into a distinctive, playable RPG.

| Document field | Current definition |
| --- | --- |
| Genre | Persistent 2D online RPG |
| Core inspirations | OSRS-style skills, quests and boss loot; Minecraft-style ingredient-grid crafting; Pokémon-like pet collecting |
| Primary audience | Players who enjoy long-term character goals, exploration, making gear, collecting companions, and repeating challenging fights |
| Design status | Direction established; several player-facing details remain open |
| Prototype foundation | Deterministic simulation, server-authoritative gameplay, combat, gathering, mining, NPC dialogue, shops, quests and player persistence |



Design north star: every activity should help players make progress toward a goal they chose, and the world should give them new reasons to explore, craft, collect and take risks.

## Contents

- 1. Vision and player promise
- 2. Design pillars
- 3. Core player loop
- 4. Player progression
- 5. World and exploration
- 6. Skills and resources
- 7. Grid crafting
- 8. Combat and bosses
- 9. Pets and collection
- 10. Quests, NPCs and economy
- 11. Multiplayer and technical principles
- 12. First representative content slice
- 13. Development roadmap
- 14. Open design decisions
- Appendix A. Current foundations

## 1. Vision and player promise

### High concept

A persistent 2D RPG about training skills, exploring for materials, crafting useful gear through ingredient patterns, collecting pets, and mastering repeatable bosses for a chance at rare, valuable loot.

### Player promise

Build up your character your way, make useful gear from what you find, collect companions, and take on dangerous bosses for a shot at extraordinary rewards.

### Player fantasy

The player becomes a capable adventurer in a living world. They grow through practice and discovery, make equipment with their own hands, build a personal collection of companions, and earn recognition by overcoming dangerous foes.

### Identity and originality

The inspirations describe familiar interaction patterns, not content to copy. The game needs its own setting, names, art direction, creatures, recipes, progression, boss mechanics and tone. Its identity should come from how skill growth, hands-on crafting, pets and boss hunting reinforce one another.

## 2. Design pillars

| Pillar | Player-facing rule | Design test |
| --- | --- | --- |
| Long-term progression | There is always a meaningful next goal, from a short task to a long gear or skill chase. | Can a player name a near-term and long-term goal? |
| Hands-on crafting | Players arrange ingredients in a visible grid and understand why a recipe works. | Does the layout matter for at least some recipes, and is crafting feedback clear? |
| Collectible companions | Pets are desirable, memorable and worth collecting beyond a checklist. | Does obtaining a pet create attachment or a useful new choice? |
| Mastery and rare rewards | Bosses reward learning and preparation; rare loot is exciting without making ordinary kills worthless. | Is each attempt satisfying even without the chase drop? |
| A world worth exploring | Areas contain useful resources, discoveries, threats and reasons to return. | Does exploration reveal something that changes player options? |
| Systems support play | Quests, shops, inventory and dialogue move the player through the main loop. | Does this feature create a meaningful player decision or outcome? |



## 3. Core player loop

1. Choose a goal: train a skill, follow a quest, gather a resource, seek a pet, craft an item or hunt a boss.
2. Prepare for the activity by choosing equipment, tools, supplies and a companion.
3. Travel into the world and explore, gather, mine, fight or discover.
4. Return with materials, experience, knowledge, quest progress or loot.
5. Use the gains to craft, upgrade, trade, unlock access or improve capabilities.
6. Attempt a harder activity or pursue a longer-term collection or rare-drop goal.

The loop should connect activities through cause and effect. For example, a resource found by mining can support a grid recipe; the resulting equipment can help with a boss; boss materials or loot can open a new crafting, pet or exploration goal.

## 4. Player progression

### Progression layers

| Layer | Purpose | Examples to explore |
| --- | --- | --- |
| Character skills | Reward repeated activity and open new actions or efficiencies. | Gathering, mining, crafting, combat; exact skill roster TBD. |
| Equipment | Provide understandable power and preparation choices. | Tools, weapons, armor and useful accessories. |
| Knowledge | Help players understand the world and make better choices. | Quest clues, recipe knowledge, enemy patterns, discovered routes. |
| Collection | Give players personal completion goals and expression. | Pets, rare equipment, trophies or cosmetic rewards. |
| World access | Make advancement visible through new places and activities. | Gates, routes, hazards, dungeons and boss arenas. |



Progression should not rely on one narrow path. Players should be able to alternate between skills, quests, crafting, exploration and combat, while some activities can ask for a deliberate level of preparation.

### Reward principles

- Give frequent small rewards for actions and clear milestones for longer goals.
- Make upgrades understandable: explain what an item improves and what activity it helps with.
- Avoid making a rare random drop mandatory for basic progression.
- Let different play styles contribute to progress, while keeping rewards appropriate to effort and risk.

## 5. World and exploration

The world should be built from readable, memorable regions that combine routes, resource opportunities, threats, NPCs and discoveries. Exploration should serve practical and emotional rewards: finding materials, learning a shortcut, seeing a new creature, locating a boss, or uncovering a story lead.

### Area structure

- Each region has a clear identity and a reason to visit.
- Resources, enemies and challenges communicate the intended preparation level.
- New areas can be opened through skill thresholds, quests, crafted tools, exploration or a combination.
- The world provides useful return visits through resources, changing goals or repeatable encounters.

### World scale

Start with a compact authored region. Prove navigation, resource placement, safe and dangerous spaces, and progression gates before committing to a large world or procedural generation.

## 6. Skills and resources

Skills should be chosen around activities that players want to repeat and around clear unlocks they can understand. Resources should have connected uses so that gathering, mining, crafting, quests and trade support one another.

### Resource design rules

- Every common resource should have at least one useful sink, such as crafting, repair, quest use or trade.
- Higher-tier resources should signal where they come from and what level of preparation is expected.
- Avoid resource abundance with no use and crafting recipes that consume materials without a satisfying result.
- Use rewards and recipes to create a readable chain from starter materials toward stronger gear or new activities.

The exact skill list, experience curve, level caps, resource catalogue and economy values are not yet defined.

## 7. Grid crafting

### Target interaction

The player opens a crafting grid, places ingredients into individual slots, and receives an output when the arrangement matches a known valid recipe. This makes crafting a spatial interaction rather than only selecting a recipe from a list.

### Recipe types

| Recipe type | Recognition rule | Good uses |
| --- | --- | --- |
| Shaped | Relative position of ingredients matters; the pattern may be mirrored only if explicitly allowed. | Tools, weapons, armor pieces and distinctive components. |
| Shapeless | Required ingredients and quantities matter; their positions do not. | Mixtures, simple processed goods and flexible combinations. |



### Interaction requirements

- Make grid slots, ingredient counts and output easy to read.
- Show whether the current arrangement matches a known recipe and what will be consumed.
- Allow players to take back ingredients safely; define behavior for closing the interface, disconnects and full inventory.
- Prevent crafting from consuming ingredients unless the authoritative server accepts the operation.
- Use the same recipe definition for validation, output creation, and any recipe book display.

### Discovery and recipe guidance

Recipe discovery is an open choice. A useful starting approach is to let common recipes be learned or shown clearly, while reserving experimentation for optional discoveries. Pure guessing can frustrate players when recipes are numerous or ingredients are costly.

### Crafting scope for first slice

Implement a small set of recipes that proves the grid: at least one shaped recipe, one shapeless recipe, and one meaningful equipment upgrade that supports the next challenge. Defer many stations, automation and complex recipe families until the basic interaction feels good.

## 8. Combat and bosses

### Combat goals

Combat should reward preparation, awareness and learning. The current deterministic tick-based foundation is an implementation constraint, but the desired player-facing rhythm, control feel and encounter pacing still need to be defined through playtesting.

### Boss encounter principles

- Give each boss a readable identity, arena, attack language and learnable behavior.
- Let equipment, supplies, skill and pet choices influence preparation without making one exact build mandatory.
- Use dependable rewards so every successful kill has value.
- Use a low-probability, high-value chase drop for excitement and long-term goals.
- Do not put an essential progression key behind an extreme random drop unless there is a guaranteed alternative path.
- Communicate encounter start, important attacks, victory, defeat, rewards and retry conditions clearly.

### Loot structure

| Reward band | Role | Design intent |
| --- | --- | --- |
| Every kill | Reliable baseline value | Supplies, common materials, currency or steady progression. |
| Occasional drops | Useful variety | Crafting inputs, consumables, equipment components or collection items. |
| Rare chase drop | Memorable high-value reward | A powerful item, distinctive pet, cosmetic trophy or valuable trade good. |



Exact probabilities and values should be tuned through play data. Communicate rarity honestly and avoid building expectations around unsupported guarantees.

## 9. Pets and collection

Pets should add personality and a satisfying collection chase. Their gameplay role remains an explicit design decision; it will affect combat balance, encounter design and the value of pet drops.

### Possible pet roles

| Role option | What it adds | Main tradeoff |
| --- | --- | --- |
| Companion and cosmetic | Attachment, expression and collection without affecting combat balance. | Less mechanical impact during play. |
| Utility partner | Small gathering, travel or support benefits. | Benefits need to avoid becoming mandatory or harming the economy. |
| Combat partner | A meaningful tactical choice during fights and boss hunts. | Requires deeper AI, balance and encounter support. |
| Mixed roles | Some pets are expressive while others have clear abilities. | Needs clear categories and careful collection balance. |



### Collection principles

- Make each pet visually and thematically distinct.
- Use several acquisition routes over time, such as exploration, quests, creature encounters or boss rewards.
- Let players understand how a pet is obtained and what makes it special.
- If pets can provide power, provide reasonable alternatives so a particular rare pet is not required for content.

## 10. Quests, NPCs and economy

Quests and NPCs give context and direction to the sandbox progression. They should introduce places, skills, recipes, creatures and boss leads, then reward players with useful progress or new choices.

- Use quests to teach mechanics naturally and point toward meaningful activities.
- Use NPC services to solve real player needs, such as trading, crafting knowledge or preparation.
- Keep rewards connected to the economy and progression instead of adding currency without a sink.
- Preserve authoritative validation for quest, shop and reward outcomes.

## 11. Multiplayer and technical principles

The project has a server-authoritative, deterministic architecture and multiplayer-oriented foundations. The player-facing multiplayer promise still needs confirmation. Until then, design gameplay outcomes so they can be validated authoritatively without assuming a large-scale MMO feature set.

- The server owns gameplay outcomes such as item creation, crafting consumption, combat results, pet ownership, boss loot and quest completion.
- Client interfaces present options and send player intent; they do not decide rewards or inventory outcomes.
- Persist meaningful character progress reliably, with versioned saves and safe migrations.
- Keep random reward generation deterministic and server-side; make loot rolls once per valid boss reward event.
- Test lifecycle and failure cases such as cancellation, stale commands, full inventory and disconnects wherever they affect item or reward integrity.

## 12. First representative content slice

The first slice should prove that the design feels like a small version of the intended game, rather than another isolated fundamentals test.

### Slice contents

- One authored starter region and one route into a more dangerous area.
- A small number of gathering and mining resources with visible uses.
- A simple skill goal that opens or improves an activity.
- A crafting interface with a shaped recipe and a shapeless recipe.
- One useful crafted equipment upgrade.
- One collectible pet, with a deliberately chosen role.
- One boss with readable mechanics, dependable rewards and a rare high-value chase drop.
- An NPC or quest that leads the player through preparation and points toward a next goal.
- A saved progress path that survives a restart.

### Player journey

1. Player receives a clear lead.
2. Player chooses what to gather or train before leaving.
3. Player explores and obtains materials while facing a manageable risk.
4. Player uses the crafting grid to make an upgrade.
5. Player prepares for and defeats the boss.
6. Player receives dependable rewards and can pursue the rare drop over repeat attempts.
7. Player sees a new goal or area become available; progress is saved.

### Slice success criteria

- A new player can identify a goal and understand the next step without developer instruction.
- The grid recipe interaction is understandable and ingredient handling is trustworthy.
- The crafted upgrade is relevant to the encounter or a clear later goal.
- The boss is learnable and a kill feels worthwhile without the rare drop.
- The pet acquisition and role are understandable.
- The player can explain what they can pursue next.
- Important progress remains after restart.

## 13. Development roadmap

| Stage | Milestone outcome | Exit evidence |
| --- | --- | --- |
| A. Confirm direction | Settle the player promise, pet role, combat feel, multiplayer scope and initial world premise. | One-page design decisions agreed; no major identity ambiguity. |
| B. Connect progression | Connect a compact resource chain, skill goal, grid crafting and equipment upgrade. | Player can gather, craft and see a concrete capability increase. |
| C. Add creature collection | Implement one pet end-to-end using its chosen role and acquisition method. | Pet is obtained, owned, presented clearly and persists correctly. |
| D. Deliver first boss loop | Create one boss, dependable loot and a rare chase reward. | Repeat fights work; rewards are authoritative, satisfying and testable. |
| E. Build authored slice | Join the pieces with an area, NPC/quest context and follow-on goal. | Fresh player can complete the representative journey unaided. |
| F. Test and expand | Playtest, tune and then add more regions, skills, pets, recipes and bosses. | Expansion builds on a validated loop and does not destabilize the economy. |



Recommended immediate decision milestone: answer the open design questions below before committing to large content production. In parallel, the technical quest work can continue as foundation work, but it should not be mistaken for the main player-facing milestone.

## 14. Open design decisions

| Decision | Current position | Why it matters |
| --- | --- | --- |
| Pet role | Undecided | Changes combat, collecting motivation, boss balance and pet UI. |
| Pet acquisition | Undecided | Shapes exploration, quest and boss reward loops. |
| Combat feel | Tick-based engine foundation; player-facing feel undecided | Determines timing, controls, boss patterns and animation/feedback needs. |
| Skill roster and progression | Undecided | Defines the main long-term activity structure. |
| Recipe discovery | Undecided | Balances experimentation against clarity and accessibility. |
| World premise and tone | Undecided | Creates the game’s original identity and guides art, creatures and story. |
| Multiplayer promise | Server-authoritative architecture exists; product scope undecided | Affects world persistence, social play, economy and content requirements. |
| Boss loot economy | Rare valuable drops desired; exact probabilities and sinks undecided | Affects replay motivation, trade value, balance and fairness. |



## Appendix A. Current foundations

Based on the project brief available for this design document, the prototype already includes a deterministic world tick, eight-direction movement, gathering and mining foundations, inventory and equipment, melee combat and enemy AI, NPC dialogue and choices, server-authoritative shops and trading, quest infrastructure, versioned player persistence, and a headless/server plus SDL client architecture.

These foundations show technical progress; they do not establish that all systems are finished, fully tested, or already connected into the intended game loop. Current source and Git history remain authoritative when planning implementation.

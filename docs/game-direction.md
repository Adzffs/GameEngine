# Game Direction

**Status:** Working direction based on Adam's stated inspirations and goals. The mechanics below are original game design targets, not a claim that any particular implementation is complete.

## High concept

A 2D online RPG built around long-term character progression, hands-on grid crafting, collectible creature companions, and repeatable boss hunts for rare, valuable loot.

The game takes inspiration from the persistent world, skills, quests, gear chase and boss hunting of Old School RuneScape; Minecraft's tactile ingredient-grid crafting; and Pokémon-style creature collecting and attachment. It should combine those ideas into its own world, progression, creatures, recipes and combat identity.

## Player promise

“Build up your character your way, make useful gear from what you find, collect companions, and take on dangerous bosses for a shot at extraordinary rewards.”

Players should always have a worthwhile next goal: train a skill, explore farther, complete a quest, discover a recipe, improve equipment, find a pet, or prepare for a boss.

## Core gameplay loop

1. **Choose a goal:** follow a quest, train a skill, gather a material, seek a pet, or hunt a boss.
2. **Prepare:** select gear, supplies, tools and possibly a companion.
3. **Go out into the world:** explore, gather, mine, fight creatures and discover materials or opportunities.
4. **Make progress:** gain skill experience, complete objectives, collect loot and learn or use crafting recipes.
5. **Craft and improve:** arrange ingredients in a crafting grid to make gear, tools, consumables and other useful items.
6. **Take on harder challenges:** use improved skills and equipment to reach new places and fight tougher bosses.
7. **Repeat for mastery and rare rewards:** boss hunts remain worthwhile through dependable rewards, while rare drops create exciting long-term goals.

## Major pillars

### 1. Long-term RPG progression

Skills, quests, equipment and access to new activities provide a steady range of short- and long-term goals. Players should be able to pursue different skills and goals at their own pace while still seeing tangible advancement.

### 2. Hands-on grid crafting

Crafting uses a visible ingredient grid. Players place items into slots and the game recognizes valid arrangements to produce an item. Recipes can use shaped patterns, where relative placement matters, and shapeless combinations, where ingredient counts matter but placement does not.

The design should support intuitive feedback, ingredient removal and return rules, recipe discovery or reference, and room for station-specific grids or recipe families if the game needs them. The goal is the tactile pattern-making interaction, implemented with this game's own interface, recipes and rules.

### 3. Collectible pets

Creatures are desirable companions players can discover, earn or obtain through play. Collection should feel personal and rewarding. Pets may have utility or combat roles, but their exact role should be decided deliberately so that collection, character combat and boss balance work together.

### 4. Repeatable boss hunting

Bosses are memorable, challenging fights that reward preparation, execution and repeated mastery. Each boss should have useful dependable rewards plus a low-probability high-value drop that gives players a compelling long-term chase. Rare drops should be exciting without making ordinary kills feel wasted or making essential progression depend on luck.

### 5. A persistent, explorable world

The world supports gathering, combat, quests, crafting and boss access. New areas and activities should open as players build their skills, equipment and knowledge. The world should feel coherent and rewarding to revisit, with a distinct identity beyond its inspirations.

## How existing systems fit

The current prototype provides useful foundations: deterministic simulation, movement, gathering and mining, inventory, equipment, combat and enemies, NPC dialogue, shops, quests, persistence, and server-authoritative gameplay.

Those systems are the groundwork. The major design work ahead is connecting them into the intended player experience: skill progression, grid crafting, pets, authored world progression, and satisfying repeatable boss encounters with a coherent loot economy.

## Recommended first representative content slice

Build one compact progression slice that proves the pillars can work together:

- One starter region and one route into a more dangerous area.
- A few gathering and mining resources with clear uses.
- A small skill progression that unlocks or improves a useful activity.
- A crafting interface with a small set of shaped and shapeless grid recipes.
- One crafted equipment upgrade that is useful for the next challenge.
- One collectible pet, with its role clearly defined for this slice.
- One boss with learnable mechanics, dependable rewards and a rare chase drop.
- A quest or NPC lead that gives context and points players toward the next goal.
- Saved progress across a restart.

Keep the slice small enough to finish and playtest. It should feel like a miniature version of the intended game, rather than a collection of unrelated feature demonstrations.

## Success criteria

A new player can understand how to set a goal, gather ingredients, use the grid to craft, prepare for a challenge, and improve their character. After defeating the boss, they receive a useful reward even without the rare drop, understand what the rare drop is for, and have a reason to continue playing. Their meaningful progress persists.

## Decisions to settle as the design develops

- Are pets mostly companions and collectibles, combat partners, or both?
- How are pets obtained: exploration, quests, creature encounters, breeding, boss drops, or a mix?
- Is combat real-time, tick-based, turn-based, or another approach? Existing deterministic ticks do not by themselves settle the player-facing combat feel.
- How are skills trained, and which skills define this game's identity?
- Are crafting recipes discovered through experimentation, learned from NPCs, shown in a journal, or supported in several ways?
- What makes this world's setting, creatures and bosses recognizable as its own?
- Is cooperative multiplayer part of the initial player promise, or a later expansion of the server-authoritative foundation?

## Direction in one line

**A persistent 2D RPG about training skills, exploring for materials, crafting through ingredient patterns, collecting pets, and mastering repeatable bosses for rare loot.**

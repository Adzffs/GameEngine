# Fresh-player profile

**Updated:** 19 November 2026  
**Work package:** GAME-001  
**Status:** Implemented; game/tool builds passed. Runtime and visible acceptance
are deferred at the owner's request. This is not a starter-balance signoff.

## Starting state

Use the fresh-player profile to author and assess the starter journey without
the development character's initial XP grants or developer weapon.

| State | Fresh character |
|---|---|
| Attack, Defence, Woodcutting, Mining, Smithing | Level 1, 0 XP each |
| Inventory | One bronze axe and one bronze pickaxe |
| Equipment, bank, coins and food | Empty; no developer godsword |
| Health | Full normal health: 100 under the current player stat rules |
| Running | Off; stamina full at 100 |
| Combat style | Accurate, the normal starting style |
| Tutorial | Meet Guide; neither completed nor skipped |
| Quests | Catalogue initial availability, zero progress, no claimed rewards |
| Known recipes | Bronze bar, wooden planks, wooden shield, bronze axe, bronze pickaxe, bronze sword |
| Location | The selected world's player spawn; `(0,0)` in compiled starter content |
| Progression | Existing gameplay XP/reward rules; no initial development XP grants |

This preserves the engine's existing health, tutorial, recipe and quest defaults.
It does not rebalance enemies, rewards, shop prices or travel. The bronze tools
allow the existing woodcutting/mining and tutorial production loop to begin.

## Launch the game

From the repository root on Windows:

```powershell
.\build-client\GameEngine.exe --profile fresh
.\build-client\GameEngine.exe --world path\world.json --profile fresh --no-save
```

A fresh launch has no implicit player save, even with the compiled world. Each
temporary run begins again from this starting state. Studio's **Play this draft**
now defaults to **Fresh player**, with a **Development** option next to the button.
Both Studio options use temporary progress and a private validated world copy.

To deliberately keep progress, select a separate save:

```powershell
.\build-client\GameEngine.exe --world path\world.json --profile fresh --save saves\river-crossing-fresh.save
```

Choose a new filename for a new persistent character. If a valid selected save
already exists, its skills, items, quests, tutorial and other progression are
restored. The profile controls missing-save/new-character initialization; it
never resets an existing character. An invalid save fails through the existing
load path instead of being replaced with a fresh character.

## Development and compatibility

No-argument game startup retains the development profile and normal development
save. `--profile development` explicitly selects those new-character defaults:
the configured Woodcutting/Mining/Smithing XP grants and the client engine's
developer godsword. `--no-save` can make that development run temporary too.

Fresh initialization is a distinct `PlayerInitializationMode::FRESH_START`.
The existing `EMPTY` mode remains dedicated to reconstructing saves. The selected
new-character mode is passed through launch options, engine configuration, World,
EntityManager and missing-save creation in the persistence lifecycle. The engine
only grants development equipment for a newly created development character.

No global development switch is changed. No profile field is added to player
saves or world snapshots; their formats are unchanged. Existing saves preserve
their own state regardless of the selected new-character profile.

[Implementation and deferred acceptance](../06-development/FRESH_PLAYER_PROFILE_2026-11-19.md).

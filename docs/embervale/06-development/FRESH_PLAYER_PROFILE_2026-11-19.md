# Fresh-player profile implementation record

**Date:** 19 November 2026  
**Base:** master `c3fdc47141350dcdbfe9b870c4fc7bba55a13b71` (PR #26).  
**Scope:** GAME-001, explicit normal starting progression and disposable Studio play.

The [profile contract](../05-game/FRESH_PLAYER_PROFILE.md) specifies skills, tools,
equipment, health, tutorial/quest/recipe state, spawn and persistence behaviour.

## Delivered change

- Append `FRESH_START` without changing existing initialization-mode values.
- Seed bronze gathering tools for a fresh character, with no development XP grants.
- Carry the selected mode through normal World/EntityManager creation and the
  missing-save path. Existing save reconstruction still uses `EMPTY` and restores
  its saved data without applying the selected new-character defaults.
- Grant/equip the developer godsword only for a newly created development client
  character, covering the engine's separate equipment injection path.
- Add strict `--profile fresh|development` launch parsing. Duplicate, missing and
  unknown profiles are rejected before startup; `--help` describes the contract.
- Keep fresh launches temporary unless an explicit save is selected, including
  compiled-world play. Existing development no-argument startup is retained.
- Default Studio to **Fresh player**, expose a **Development** selector, and pass
  the chosen profile alongside the existing private world and `--no-save` launch.

Player-save and world-content formats are unchanged. This work does not establish
measured first-upgrade pacing, resolve the suspected economy loop or implement
the River Crossing geography.

## Execution evidence

| Check | Outcome |
|---|---|
| C++ core, SDL game and editing-tool build | Passed, 23 build steps, exit 0 |
| Editor Python syntax check | Passed via `ast.parse` |
| Unit/integration and Python test suites | Deferred at the owner's request; not run |
| Visible fresh-character and Studio acceptance | Deferred at the owner's request; not run |

Build command in the existing Windows UCRT64 configuration:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build build-client --target GameEngine EmbervaleWorldMove --parallel 6
```

The owner requested continued implementation without testing for now. No earlier
test totals are attributed to this change. GAME-001 records the implemented
profile with runtime/visible acceptance outstanding; it is not a complete R0/R3
or release-gate claim.

## Deferred acceptance

Check fresh starting skills/tools/tutorial/quests/recipes, no godsword grant or
equip command, unchanged development defaults, and unchanged loaded saves. Cover
temporary versus explicit-save launch in both world modes, option order and
profile rejection, missing-save creation, invalid-save failure, save/reload and
Studio's two profile selections with temporary-directory cleanup. Observe the
fresh tutorial/first-upgrade loop before making balance claims.

The next implementation work package is GAME-002: the flat River Crossing
geography, followed by deliberate service/resource/actor placement.

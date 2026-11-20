# Editor workflows, recovery and playtesting

**20 November evidence:** The [project review](../06-development/PROJECT_REVIEW_2026-11-20.md) executes the current CLI/bridge and editor checks. Use [current status](../01-current-state/CURRENT_STATUS.md) to distinguish implemented workflows from proposed recovery/publication scope.


**Status:** Bounded edit/history/draft/reopen/playtest workflows exist; broader recovery/publication workflows below remain targets.\
**Updated:** 18 November 2026\
**Audience:** Designers, developers and AI agents changing Embervale content.

## 1. Start from a known revision

Open a specific region or published content revision and create an editable draft. The editor should display the source revision, current semantic hash, schema version and dirty/validation state. Confirm the map's purpose before editing: the exported development baseline exists to prove compatibility; River Crossing is the current authored starter candidate.

A draft is not selected automatically by the game. A preview is not a publication. A playtest uses a selected candidate and isolated test state. Keeping these steps visible makes it possible to experiment without accidentally changing the world used by normal development saves.

Until Studio is implemented, these instructions are acceptance criteria and workflow design, not a current user manual. Refer to [Implemented systems](../01-current-state/IMPLEMENTED_SYSTEMS.md) for existing capabilities.

## 2. Move a resource or workstation

1. Select the placement by clicking its footprint or searching its durable ID.
2. Inspect the definition and terrain beneath it; turn on collision and interaction overlays.
3. Move the candidate to the intended tile, using typed coordinates if precise placement is easier.
4. Review bounds, terrain compatibility, overlap and approach diagnostics.
5. Commit one command to the draft and inspect the semantic before/after diff.
6. Undo and redo once during the first workflow test to confirm identity and base terrain are restored correctly.
7. Validate required routes, save the draft and reopen it from disk.
8. Preview and launch an isolated test using the exact saved hash.

For a resource, test reaching it, beginning gathering, completion and the expected depletion/respawn behaviour. For a workstation, test opening/using the correct interaction and leaving it. Moving a sprite without moving its authoritative interaction position is a failed edit.

The present game writes resource/station occupancy into blocked tile types. The first adapter must reconstruct affected blocking from base terrain plus placements, so moving an object does not erase the pond or leave a phantom blocker. [Sources: World.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/World.cpp), [Tile.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Tile/Tile.cpp).

## 3. Paint geography without breaking access

Begin with the route and terrain overlays, then paint a small bounded region. Preview the full affected footprint before applying a fill operation. A batch that crosses a player spawn, mandatory interaction approach or region exit should surface those dependencies clearly.

It is legitimate to save an unfinished, structurally valid terrain draft while a bridge or route is still being designed. The draft must show gameplay errors and cannot be published until the required connections exist. This supports actual design iteration without weakening runtime safety.

After changing water or walls, test the player's route using the same movement semantics as the game. A diagonal-looking gap may not be traversable. Verify routes to required services, quest actors and exits. Optional inaccessible decorative spaces can remain intentional when they are not required gameplay destinations.

For River Crossing, preserve the agreed distributed-settlement direction: the road and river connect several useful places rather than forcing every activity through a central square. That design goal should be evaluated in playtests through navigation, travel and activity flow, not imposed as an engine rule.

## 4. Place a named NPC or monster

Select an existing definition and distinguish the placement's home/spawn from runtime movement. Show relevant wander, population, respawn and aggression information only when the current runtime supports those properties. Unsupported fields should be disabled with an explanation rather than saved and ignored.

For a named NPC, verify its dialogue, shop or quest references and a reachable approach. Keep its durable identity when it moves. For a monster, validate the spawn tile, home/leash relationship, active-count rules and respawn location, then test a controlled encounter with a known seed where practical.

The current world checks supplied spawn data against the canonical spawn database and creates some starter actors only after player registration. A new editor workflow is not complete until the runtime catalogue adapter honours the selected validated content and preserves those lifecycle rules. [Sources: World.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/World.cpp), [NpcSpawnDatabase.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/NPC/NpcSpawnDatabase.cpp).

## 5. Add or replace artwork

Import artwork through an asset record with a stable asset ID, path, dimensions, visual anchor and source/licence information. Preview it at the real in-game tile scale, then at representative zoom levels. Check transparency, edges, selection visibility and overlap with other objects.

Replacing a tree's image should not change its resource definition, occupied tile, gathering requirements or authored placement ID. If the desired art implies a larger building or new bridge, record the corresponding gameplay footprint/transition work separately. Image dimensions do not create that behaviour.

Keep debug rectangle rendering available while the sprite pipeline matures. A missing texture should produce an actionable diagnostic and visible fallback during development rather than making the object disappear. Validate both the game view and the editor/preview interpretation.

## 6. Edit quests, dialogue or recipes

Each content editor should present the existing runtime model before adding new abstractions. Make one bounded definition change, inspect references, validate it and run a scenario that exercises the change. For a quest, include the prerequisites, start state, progress transition and reward. For dialogue, exercise each changed branch and its session conditions. For a recipe, test valid inputs, invalid inputs, inventory space and completion behaviour.

Changing a display name is different from changing a stable ID. Before deleting a referenced definition, inspect the dependency report and the save-compatibility implications. Do not treat a successful JSON parse as sufficient validation of a quest graph or reward path.

Simulation tools should state their assumptions. A combat/economy report needs the definition revision, scenario, seeds or sampling method and the quantities measured. Use such reports to compare alternatives; use playtests to evaluate actual comprehension and pacing.

For the experimental in-memory history implementation, a sequence of supported
resource/workstation moves can be grouped into one reversible command.
Undo and redo run the same validated move operation in reverse/forward order;
any externally changed semantic revision produces an explicit conflict and leaves
both the caller snapshot and history cursor untouched. Undo does not yet survive
process restart and does not bypass guarded draft publication. See
[bounded undo/redo](BOUNDED_UNDO_REDO.md).

## 7. Save, recover and resolve conflicts

### Ordinary save

Save writes the draft through the shared command service with the expected base hash. It preserves the previous file on failure and reports the saved revision/hash. A gameplay-invalid draft may be saved when the draft policy allows it, but its validation status must remain clearly invalid.

### Interrupted session

On reopening, the editor compares recoverable autosaves with the last saved draft. Show the candidate, timestamp as editor metadata, source hash and change summary. The creator may recover it to a draft or discard it. Recovery does not activate game content and does not write a normal player save.

### External modification

If another tool or person changes the file, stop the attempted overwrite and show the current revision. Reload and reapply the intended bounded command, or resolve the semantic differences explicitly. Do not hide a conflict by automatically changing the expected hash or choosing “last writer wins.”

### Undo after a conflict

An inverse command applies to a known state. If that state has changed, validate the inverse against the current content rather than restoring an old whole-file snapshot over unrelated work. A conflict may require a new deliberate edit.

## 8. Validate and publish a candidate

Publication requires the exact candidate hash to pass the selected validation profile. Review structural checks, reference integrity, supported placement rules, mandatory routes and compatibility scenarios. Open the preview and inspect the semantic diff. Resolve any required errors before publishing an immutable content package.

The publication record should identify its source commit or draft origin, content hash, schema, validator version, required test evidence and known limitations. A newly edited candidate cannot reuse an earlier report simply because the filename is the same.

Publishing content in Studio does not automatically push to GitHub, release a build or deploy a public server. It creates the package that those separately scoped processes can consume. See [CLI contract](CLI_AND_AI_CONTRACT.md) for the proposed transaction and output rules.

## 9. Conduct an isolated playtest

Use a synthetic or copied test save, explicitly selected content and a known starting state. The current application persistence path is relative to its process working directory, so careless launch locations can select the wrong save. The future test launcher must make the save path visible and isolate test state deliberately. [Source: DevelopmentPlayerPersistence.md](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/docs/DevelopmentPlayerPersistence.md).

Record the content hash, game commit, platform, scenario, route taken and result. For geography, observe where the player hesitates, gets blocked, retraces steps or misses an important service. For economy, record travel and interaction time separately from gathering/crafting time. For combat, record encounter location, escape space and recovery behaviour.

The minimum first-editor test covers starting safely, reaching the moved resource, gathering, reaching a station or bank, interacting with a named NPC, completing a controlled combat/recovery scenario and saving/reloading. Later map changes can use narrower tests when the affected behaviour is well understood; do not run every possible test merely to produce a longer report.

## 10. Review handoff

A content change is ready for review when it includes:

- A short purpose statement explaining the creator/player benefit.
- The exact base and candidate revisions/hashes.
- A semantic diff listing moved, added, removed or redefined content.
- Validation results and their tool/profile versions.
- Before/after previews when appearance or geography changed.
- Relevant playtest observations, including failures and unresolved limitations.
- Save/content compatibility notes and a reversible rollback route.

Keep a single source of truth for each fact. The roadmap records completion status; the content file records placements; the validation report records checks; the playtest report records observations. Updating one does not automatically update the others.

Continue with [Save compatibility](../03-engine/SAVE_COMPATIBILITY.md) for persistent-state rules and [Studio design](STUDIO_DESIGN.md) for workspace scope.

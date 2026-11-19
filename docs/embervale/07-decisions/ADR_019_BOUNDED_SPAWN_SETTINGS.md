# ADR-019: Bounded per-spawn monster settings

**Date:** 19 November 2026  
**Status:** Implemented; runtime and visible acceptance deferred by the owner.  
**Scope:** STUDIO-041, existing starter-region actor identities only.

## Decision

Introduce starter-world snapshot schema 3. It retains schema 2's object allocation
reservations and adds `respawn_delay_ticks` to every actor's `spawn` object.
Versions 1 and 2 retain their exact historical spawn settings and reject this new
field. Capturing the compiled baseline still emits version 1 unchanged.

The first actual spawn-settings edit upgrades a draft to version 3. Moving a home
alone needs no upgrade. Further object edits and undo retain version 3, and the
allocation counter never rewinds. Undo compares normalized world state, treating
an absent historical delay as zero, without replacing an old whole-world snapshot.

| Field | Supported authored values |
|---|---|
| Wander radius | 0–32 tiles; aggressive monsters must remain within their combat leash |
| Wander interval | 0 with radius 0; otherwise 1–86400 game ticks |
| Maximum active count | 1–8 for monsters; named friendly NPCs remain at 1 |
| Respawns | Boolean for monsters; false for friendly NPCs |
| Respawn delay | 0 inherits the enemy type's delay; 1–86400 overrides it; disabled respawning requires 0 |

Named friendly NPCs remain stationary, unique and non-respawning because the
current runtime implements idle wandering and death respawning only for monsters.
Their home is editable; their unsupported behaviour fields are read-only in Studio.
Actor additions/removals, definition/type changes, quest/dialogue/shop references,
spawn IDs and initialization stage/order remain outside this command.

The maximum active count is an authority limit, not a population target. Each
existing record still creates one initial actor at its original lifecycle stage.
Raising the cap permits additional authorized runtime creation but does not spawn
more monsters automatically or alter the existing creation order. Dead monsters
awaiting their existing entity's respawn continue to occupy their registered slot.

## Shared authority and presentation

`StarterWorldEdit::ConfigureSpawn` first validates the source, edits a private
typed candidate and validates the whole resulting world. Its history entry stores
only the target spawn's before/after settings. The terminal session's `spawn`
command and Tk settings dialog share this command and history.

The Python bridge verifies the complete canonical snapshot, including the schema
upgrade and default fields on every actor. Settings-only undo/redo is checked
against full world checkpoints even when no coordinates or terrain change.
Saving, reopening and isolated draft launch reuse the existing guarded workflow.

`World::CreateMonster` compares the requested delay against the selected world's
spawn catalogue, then passes the selected effective delay to the existing monster
respawn scheduler. Compiled enemy definitions and normal compiled-world defaults
are unchanged. This is a map-content revision, not a player-save revision.

## Consequences

Older tools reject schema 3 explicitly. Use matching game and editor binaries for
new drafts. This change does not provide arbitrary enemy definitions, automatic
population filling, friendly wandering, multi-region identity or a new save format.
Builds succeeded; tests and visible acceptance were deliberately deferred. See the
[implementation record](../06-development/STUDIO_SPAWN_SETTINGS_2026-11-19.md).

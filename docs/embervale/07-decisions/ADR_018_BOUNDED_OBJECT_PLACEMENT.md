# ADR-018: Bounded resource and station membership

**Date:** 19 November 2026  
**Scope:** STUDIO-040; the existing flat 100×100 development region.

## Context

The schema-1 starter snapshot requires exactly the original 20 identities.
Relaxing that historical parser would make old files mean something different.
New resource/station placement also needs an explicit allocation rule and
reversible membership changes, rather than deriving identity from coordinates.

## Decision

Keep schema 1 and its canonical export bytes unchanged. Moving objects and
painting terrain in a schema-1 session keep that version. The first successful
object creation or removal promotes its candidate to schema 2. Schema 2 adds
the required integer `next_placement_id`, initially 1, to the existing fields.

Only the seven compiled resource definitions and three compiled station
definitions may be added. The ten original NPC/monster records remain required,
with their existing stable IDs, bindings, stages, orders and spawn settings.
Original resources/stations retain their reviewed bindings when present and
can be removed. No actor creation/removal or definition authoring is introduced.

New object identities are `resource.authored.NNNNNN` or
`station.authored.NNNNNN`, where the six-digit sequence is allocated globally
within this region. Allocation starts at 000001, ends at 999999, and increments
only after successful validation. The counter may reach 1000000, at which point
further creation fails explicitly. A sequence cannot be shared across kinds.
Every live authored sequence must be lower than the persisted counter.

An authored object is created at `world_start`, with order `12 + sequence`.
This preserves the original creation order of retained legacy records and
puts new objects after them. Resource/station runtime numbers remain transient;
they are not the authored identity or a new player-save field.

Removal and undo never rewind the allocation counter. Undo/redo restore the
same ID and complete binding through validated inverse operations. Creating a
new branch after undo allocates a fresh sequence. History checks the exact
current canonical revision, including allocation state; its inverse-world
comparison separately normalises schema/allocation bookkeeping. This lets an
earlier move or paint undo correctly after a later creation has been undone.

Consequently, undo can restore the visible map while the draft remains dirty:
the format promotion and reserved IDs still need saving. This behaviour avoids
silently recycling a previously saved object identity. Save/reopen retains the
reservation even if no newly created objects remain.

The shared C++ importer validates all edits, bounds, occupied/water tiles,
clear player spawn and reachable actor/object approaches. Removing an object
leaves its underlying authored ground unchanged. The runtime constructs each
remaining/new object through the existing resource/station factories.

Studio requests a complete canonical `snapshot` response after every replay.
It checks that JSON against the placement list, terrain, requested transition
and observed undo checkpoints. New-object definitions and lifecycle metadata
therefore come from C++, rather than being guessed for saved content by Python.
Guarded sibling publication and the exact saved-byte/semantic receipts remain
the only save path.

## Compatibility and boundaries

Player-save codecs and versions do not change. Existing save recovery rules
still handle positions blocked by the selected world. Older engine versions
reject schema-2 maps explicitly; they must use their original schema-1 content.
The canonical development fixture remains available for that purpose.

This is a region-local allocator for scoped draft authoring. Independently
edited forks still need explicit Git/content conflict resolution; the counter
does not solve collaborative allocation. Alias migrations, arbitrary regions,
new NPC/enemy records and persistent editor history remain separate work.
Removing the last instance of a resource/station is allowed: the geographic
validator proves access to remaining content, not a complete playable quest or
economy loop. Starter-area playtests must establish that broader game coverage.

## Evidence

See [placement verification](../06-development/STUDIO_PLACEMENT_VERIFICATION_2026-11-19.md)
and the [editor workflow](../04-studio/VISUAL_WORLD_EDITOR.md).

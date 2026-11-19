# ADR-020: bounded authored static building shells

**Date:** 19 November 2026\
**Status:** Accepted in implementation; first workshop integrated.\
**Scope:** Flat starter-world structures and the modular building-art pilot.

A building image alone would let players walk through its pictured walls.
Treat collision as authored content and keep roof visibility in presentation.

Schema 4 adds a required `buildings` array (up to 64 entries) alongside existing
ground, actor and resource/station placements. Each entry has exactly `id`,
`definition_id`, `x`, `y`, `width`, `height`, `door_offset`. The supported definition
is `building.timber_stone`. Width is 5..16 tiles, depth 4..12, with a one-tile
margin inside the map and an interior offset for one open south doorway.
All footprint ground must be stone. Stable IDs begin with `building.`; IDs,
footprints and existing object/actor occupancy cannot overlap invalidly.

The shared `Building` value defines perimeter walls and the doorway. Runtime
validation uses those walls in its blocking/reachability grid before accepting
the world. It requires a clear reachable entrance/outside approach, reachable
interior floor and existing actor/service access. World applies the same wall
cells to Map after ground and before placements. Existing pathfinding therefore
uses real walls; no graphics-dependent collision or second world interpretation
is introduced. Ground underlays remain intact.

Canonical output sorts buildings by ID. Versions 1..3 keep their historical root
fields and reject building data. Version 4 retains schema-3 spawn fields. The
River Crossing upgrade explicitly writes zero/default respawn delays, preserving
existing actor behaviour. Player save formats do not change; existing saved-
position walkability/reachability recovery remains applicable to the new walls.

Studio preserves building data through existing commands, history and guarded
drafts; spawn edits and history comparisons retain schema 4. Collision/access
overlays and map/overview views include shells. Building creation/movement in
the GUI is not implemented by this change; shell geometry is inspected read-only.

Rendering tiles the roof at native scale, orders walls by ground contact, and
fades roofs when their image overlaps the player or the player is inside. Roof
alpha and any fading wall pixels never open or close a wall. F6 and missing-
image fallback preserve the same collision model.

This bounded model proves one workshop and a reusable kit. Arbitrary polygons,
multiple floors, door state/locking, north/east/west entrances, furnishing tools
and broader building authoring remain separate decisions. [Integration and
evidence](../06-development/BUILDING_RENDERING_2026-11-19.md).

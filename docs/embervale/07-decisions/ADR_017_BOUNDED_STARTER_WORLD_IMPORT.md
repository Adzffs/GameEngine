# ADR-017: Explicit bounded starter-world import

**Date:** 18 November 2026\
**Status:** Implemented runtime-import decision; wider authoring and publication remain open.\
**Related work:** STUDIO-003, STUDIO-010–015, STUDIO-050, STUDIO-051 and STUDIO-053.\
**Extends:** [ADR-016](ADR_016_READ_ONLY_WORLD_EXPORT.md).\
**Usage and source:** [Starter-world import](../04-studio/STARTER_WORLD_IMPORT.md).

## Context

The existing exporter captures ground, stable placement identities and delayed creation stages. The next useful capability is loading that content through the real game while preserving current gameplay. The current typed spawn methods also validate against an authoritative catalogue, and player loading/death/recovery depend on map positions. Accepting arbitrary JSON coordinates without addressing these paths would leave an incomplete runtime integration.

## Decision

Extend the existing schema-1 snapshot with a strict bounded C++ input path. The supported runtime scope is the existing 100×100 starter region and complete 20-entry registry. Permit terrain, placement coordinates and player-spawn coordinates to vary; retain definition bindings, spawn settings, stable IDs and creation stages. Reject unsupported content explicitly. Use the same runtime validator for file input and directly supplied typed snapshots.

Keep the parser and importer in `GameEngineCore`, which requires no SDL. Retain `EmbervaleWorldSnapshot` as a compatibility target that links the core. `GAMEENGINE_BUILD_CONTENT_TOOLS` controls the standalone export/validation executables, not runtime input support. This avoids a core-to-tool dependency cycle and permits headless validation.

Use the current restricted JSON representation with explicit byte, depth, string, container and total-value budgets. The parser has no new third-party dependency. This limited contract is not a general-purpose JSON implementation: ASCII and the existing schema are the supported input, and future encoding/schema support requires an explicit change with regression evidence.

Validate complete initial occupancy and access before creating a runtime map. Check placement overlaps, walkable ground, player-spawn clearance, actor access and a reachable cardinal approach for every placement. Construct ground first and existing occupied tiles second. Register a world-local selected spawn catalogue before creating actors; preserve typed definition, population and lifecycle checks.

Make import explicit with `GameEngine --world <file>`. No-argument startup retains the compiled world and normal development save. Imported-world startup uses no player save unless `--save` explicitly selects one. `--compiled-world` makes the original content an explicit return path. Invalid selected content fails startup; there is no silent compiled fallback.

Player saves remain version 13. Selected spawn and active-map reachability govern player and recovery fallback within the single legacy-compatible region. Preserve unrelated progression and item quantities. Reject world/save destination aliases, including the save transaction’s temporary and backup sidecars, before startup.

## Alternatives considered

| Alternative | Reason not selected |
|---|---|
| Replace the default map source immediately | Makes every existing launch depend on a new file and mixes migration with opt-in authoring |
| Mutate the global spawn database | Lets one imported world affect another and weakens ownership of canonical definitions |
| Skip spawn validation for external coordinates | Bypasses existing type, population and lifecycle contracts |
| Accept arbitrary sizes, IDs and spawn properties | Expands the compatibility surface beyond the tested starter-region slice |
| Automatically reuse the normal player save | Couples geography experiments to ordinary progression |
| Build persistent editing and GUI at the same time | Delays a reviewable runtime boundary and combines separate failure modes |

## Consequences and limits

A validated starter snapshot can drive actual runtime construction, with original ground beneath moved objects and stable creation stages. The validator and game share interpretation; the compiled world remains available. Exact input bounds and command behaviour are documented in the [usage guide](../04-studio/STARTER_WORLD_IMPORT.md).

The registry still describes existing placements. This decision does not permit arbitrary new entities, editable gameplay definitions, larger worlds, surfaces, exits or new region identities. Renderer labels and some narrative guidance remain compiled, so valid coordinates alone do not establish a coherent redesigned settlement.

There is no authored-content writer, shared move operation, expected-hash guard, publication manifest, undo history, preview renderer or visual editor in this slice. File alias checks occur at startup and do not claim protection against an independently racing filesystem writer. General save/content migration and cross-platform playtest evidence remain separately scoped.

## Verification contract

Compare an unchanged imported snapshot with an independently constructed compiled world across the complete grid, creation stages, identities and representative interactions. Test changed coordinates, preserved base terrain, authoritative spawn rejection, death/respawn and saved-location/recovery handling. Exercise bounded malformed files and the real validator command; verify launch/save defaults and path aliases. Record the executed environment and results separately from the presence of test source.

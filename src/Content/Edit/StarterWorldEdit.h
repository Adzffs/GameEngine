#pragma once

#include "../Export/StarterWorldSnapshot.h"

#include <string>
#include <string_view>

// Pure, transactional edits of the bounded starter-world content model.
// A successful result is runtime-valid, but no live World or disk file is changed.
namespace StarterWorldEdit
{
    struct MoveResult
    {
        StarterWorldExport::Snapshot candidate;
        std::string placementId;
        Position previousPosition;
        Position newPosition;
        bool changed;
    };

    struct PaintResult
    {
        StarterWorldExport::Snapshot candidate;
        Position position;
        char previousTerrain;
        char newTerrain;
        bool changed;
    };

    struct PlacementResult
    {
        StarterWorldExport::Snapshot candidate;
        StarterWorldExport::Placement placement;
    };

    PlacementResult PlaceDefinition(const StarterWorldExport::Snapshot &source,
        std::string_view definitionId, const Position &destination);
    PlacementResult RemovePlacement(const StarterWorldExport::Snapshot &source,
        std::string_view placementId);

    // A history inverse restores the same binding/ID; it never allocates one.
    StarterWorldExport::Snapshot RestorePlacement(
        const StarterWorldExport::Snapshot &source,
        const StarterWorldExport::Placement &placement);

    // Paint exactly one existing base-ground tile with G (grass), D (dirt),
    // S (stone) or W (water). The source and resulting world must both be
    // runtime-valid; route/spawn/occupancy checks cannot be bypassed.
    // No live map, player save or file is modified.
    PaintResult PaintTerrain(const StarterWorldExport::Snapshot &source,
        const Position &tile, char ground);

    // Relocate an existing object or actor spawn/home by durable authored ID.
    // Actor moves update both placement and typed spawn coordinates. Preserve
    // definition, lifecycle, spawn settings, base ground and other placements.
    // Reject invalid input without returning a partially edited candidate.
    MoveResult MovePlacement(const StarterWorldExport::Snapshot &source,
        std::string_view placementId, const Position &destination);
}

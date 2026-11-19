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

    // Paint exactly one existing base-ground tile with G (grass), D (dirt),
    // S (stone) or W (water). The source and resulting world must both be
    // runtime-valid; route/spawn/occupancy checks cannot be bypassed.
    // No live map, player save or file is modified.
    PaintResult PaintTerrain(const StarterWorldExport::Snapshot &source,
        const Position &tile, char ground);

    // First supported command: relocate an existing resource or station by
    // durable authored ID. Preserve its definition, stage, order, base ground
    // and every other placement. Reject invalid source/destination and unknown
    // or unsupported identities; never return a partially edited candidate.
    MoveResult MovePlacement(const StarterWorldExport::Snapshot &source,
        std::string_view placementId, const Position &destination);
}

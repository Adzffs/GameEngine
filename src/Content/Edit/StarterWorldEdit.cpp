#include "StarterWorldEdit.h"

#include "../Import/StarterWorldRuntimeInput.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace StarterWorldEdit
{
    SpawnSettingsResult ConfigureSpawn(const StarterWorldExport::Snapshot &source,
        std::string_view placementId, const SpawnSettings &settings)
    {
        auto candidate = StarterWorldImport::ValidateForRuntime(source);
        const auto found = std::find_if(candidate.placements.begin(), candidate.placements.end(),
            [placementId](const auto &placement) { return placement.id == placementId; });
        if (found == candidate.placements.end())
            throw std::runtime_error("starter world edit: unknown spawn ID: " + std::string(placementId));
        auto *spawn = std::get_if<NpcSpawnDefinition>(&found->legacyDefinition);
        if (spawn == nullptr)
            throw std::runtime_error("starter world edit: select an actor spawn to configure");
        const SpawnSettings before{spawn->wanderRadius, spawn->wanderIntervalTicks,
            spawn->maximumActiveCount, spawn->respawns, spawn->respawnDelayTicks};
        const bool changed = before != settings;
        if (changed)
        {
            spawn->wanderRadius = settings.wanderRadius;
            spawn->wanderIntervalTicks = settings.wanderIntervalTicks;
            spawn->maximumActiveCount = settings.maximumActiveCount;
            spawn->respawns = settings.respawns;
            spawn->respawnDelayTicks = settings.respawnDelayTicks;
            candidate.schemaVersion = std::max(3, candidate.schemaVersion);
            candidate = StarterWorldImport::ValidateForRuntime(candidate);
        }
        return {std::move(candidate), std::string(placementId), before, settings, changed};
    }

    PlacementResult PlaceDefinition(const StarterWorldExport::Snapshot &source,
        std::string_view definitionId, const Position &destination)
    {
        using namespace StarterWorldExport;
        auto candidate = StarterWorldImport::ValidateForRuntime(source);
        const auto placement = AllocatePlacement(definitionId,
            candidate.nextPlacementId, destination);
        candidate.schemaVersion = std::max(2, candidate.schemaVersion);
        ++candidate.nextPlacementId;
        candidate.placements.push_back(placement);
        candidate = StarterWorldImport::ValidateForRuntime(candidate);
        return {std::move(candidate), placement};
    }

    PlacementResult RemovePlacement(const StarterWorldExport::Snapshot &source,
        std::string_view placementId)
    {
        using namespace StarterWorldExport;
        auto candidate = StarterWorldImport::ValidateForRuntime(source);
        const auto found = std::find_if(candidate.placements.begin(), candidate.placements.end(),
            [placementId](const Placement &placement) { return placement.id == placementId; });
        if (found == candidate.placements.end())
            throw std::runtime_error("starter world edit: unknown placement ID: " + std::string(placementId));
        if (found->kind != PlacementKind::Resource && found->kind != PlacementKind::Station)
            throw std::runtime_error("starter world edit: actor spawns cannot be removed: " + found->id);
        const auto placement = *found;
        candidate.schemaVersion = std::max(2, candidate.schemaVersion);
        candidate.placements.erase(found);
        candidate = StarterWorldImport::ValidateForRuntime(candidate);
        return {std::move(candidate), placement};
    }

    StarterWorldExport::Snapshot RestorePlacement(
        const StarterWorldExport::Snapshot &source,
        const StarterWorldExport::Placement &placement)
    {
        using namespace StarterWorldExport;
        if (placement.kind != PlacementKind::Resource && placement.kind != PlacementKind::Station)
            throw std::runtime_error("starter world edit: only object history can be restored");
        auto candidate = StarterWorldImport::ValidateForRuntime(source);
        candidate.placements.push_back(placement);
        return StarterWorldImport::ValidateForRuntime(candidate);
    }

    PaintResult PaintTerrain(const StarterWorldExport::Snapshot &source,
        const Position &tile, char ground)
    {
        using namespace StarterWorldExport;
        // Check the original content first: an edit may not make a broken map
        // appear valid by incidentally repairing an unrelated defect.
        Snapshot candidate = StarterWorldImport::ValidateForRuntime(source);
        const int x = tile.GetX();
        const int y = tile.GetY();
        if (x < 0 || y < 0 || x >= candidate.width || y >= candidate.height)
            throw std::runtime_error("starter world edit: paint tile is outside the starter region");
        if (ground != 'G' && ground != 'D' && ground != 'S' && ground != 'W')
            throw std::runtime_error("starter world edit: unsupported terrain glyph (use G, D, S or W)");

        char &existing = candidate.terrainRows[static_cast<std::size_t>(y)]
            [static_cast<std::size_t>(x)];
        const char previous = existing;
        const bool changed = previous != ground;
        if (changed)
        {
            existing = ground;
            // Revalidates everything, not just the target tile: water may
            // destroy connectivity to distant actors or their approaches.
            candidate = StarterWorldImport::ValidateForRuntime(candidate);
        }
        return PaintResult{std::move(candidate), tile, previous, ground, changed};
    }

    MoveResult MovePlacement(const StarterWorldExport::Snapshot &source,
        std::string_view placementId, const Position &destination)
    {
        using namespace StarterWorldExport;

        // Validate the source before applying a change. Never bless an unsafe
        // input just because the requested position happens to be legal.
        Snapshot candidate = StarterWorldImport::ValidateForRuntime(source);
        const auto found = std::find_if(candidate.placements.begin(),
            candidate.placements.end(), [placementId](const Placement &placement) {
                return placement.id == placementId;
            });
        if (found == candidate.placements.end())
            throw std::runtime_error("starter world edit: unknown placement ID: " +
                std::string(placementId));
        const Position previous = found->position;
        const bool changed = previous.GetX() != destination.GetX() ||
            previous.GetY() != destination.GetY();
        if (changed)
        {
            found->position = destination;
            // Keep the authored spawn/home and its typed runtime definition in
            // step. All other identity, lifecycle and behaviour fields remain
            // unchanged; the full-world validator checks the new home.
            if (auto *spawn = std::get_if<NpcSpawnDefinition>(&found->legacyDefinition))
                spawn->spawnPosition = destination;
            // Includes the entire world's occupancy, access routes and spawn
            // semantics. A failed move only destroys this private candidate.
            candidate = StarterWorldImport::ValidateForRuntime(candidate);
        }
        return MoveResult{std::move(candidate), std::string(placementId),
            previous, destination, changed};
    }
}

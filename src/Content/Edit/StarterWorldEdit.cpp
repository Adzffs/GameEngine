#include "StarterWorldEdit.h"

#include "../Import/StarterWorldRuntimeInput.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace StarterWorldEdit
{
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
        if (found->kind != PlacementKind::Resource &&
            found->kind != PlacementKind::Station)
            throw std::runtime_error("starter world edit: moving actor spawns is not yet supported: " +
                found->id);

        const Position previous = found->position;
        const bool changed = previous.GetX() != destination.GetX() ||
            previous.GetY() != destination.GetY();
        if (changed)
        {
            found->position = destination;
            // Includes the entire world's occupancy, access routes and spawn
            // semantics. A failed move only destroys this private candidate.
            candidate = StarterWorldImport::ValidateForRuntime(candidate);
        }
        return MoveResult{std::move(candidate), std::string(placementId),
            previous, destination, changed};
    }
}

#pragma once

#include "../../NPC/NpcSpawnDefinition.h"
#include "../../World/Object/Resource/ResourceType.h"
#include "../../World/Object/Station/StationType.h"

#include <string>
#include <variant>
#include <vector>

// Export-only observations of the compiled starter content. This is not a
// runtime loader, player save, or a supported general-purpose editing model.
namespace StarterWorldExport
{
    enum class PlacementKind
    {
        Resource,
        Station,
        NpcSpawn,
        MonsterSpawn
    };

    enum class InitializationStage
    {
        WorldStart,
        FirstPlayerRegistration
    };

    struct Placement
    {
        std::string id;
        std::string definitionId;
        PlacementKind kind;
        InitializationStage stage;
        int order;
        Position position;
        // Typed bindings are retained for parity checks. The JSON contains
        // explicit string identities, never enum ordinals or runtime IDs.
        std::variant<ResourceType, StationType, NpcSpawnDefinition> legacyDefinition;
    };

    struct Snapshot
    {
        int width = 0;
        int height = 0;
        std::vector<std::string> terrainRows;
        Position playerSpawn{0, 0};
        std::vector<Placement> placements;
    };

    // Reads compiled definitions and a separate base Map, never a live World or
    // a save file. Throws std::runtime_error if the baseline registry is stale.
    Snapshot Capture();

    // Validates the snapshot's structural invariants and returns canonical JSON
    // with a final LF. Sorting does not mutate the supplied snapshot. This is a
    // writer only; it does not establish import or gameplay-validation support.
    std::string SerializeCanonical(const Snapshot &snapshot);
}

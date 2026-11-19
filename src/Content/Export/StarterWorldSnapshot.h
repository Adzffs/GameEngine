#pragma once

#include "../../NPC/NpcSpawnDefinition.h"
#include "../../World/Object/Resource/ResourceType.h"
#include "../../World/Object/Station/StationType.h"

#include <string>
#include <string_view>
#include <variant>
#include <vector>

// Typed observations of the compiled starter registry, shared by the exporter
// and the opt-in runtime bridge. This is not a player save or a general-purpose
// editor model; runtime input has additional validation and a fixed scope.
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
        // Typed bindings support parity checks and runtime spawn authority. The JSON contains
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
        // v1 retains the exact compiled registry. v2 permits object membership
        // changes and persists an allocation high-water mark, including undo.
        int schemaVersion = 1;
        int nextPlacementId = 1;
    };

    constexpr int MaximumNextPlacementId = 1000000;
    constexpr int AuthoredPlacementOrderBase = 12;

    // Only the seven resource and three station definitions already supported
    // by the runtime may be instantiated. Actor identities remain compiled.
    Placement BindAuthoredPlacement(std::string_view id,
        std::string_view definitionId, const Position &position);
    Placement AllocatePlacement(std::string_view definitionId,
        int sequence, const Position &position);
    int AuthoredPlacementSequence(std::string_view id);

    // Reads compiled definitions and a separate base Map, never a live World or
    // a save file. Throws std::runtime_error if the baseline registry is stale.
    Snapshot Capture();

    // Validates the snapshot's structural invariants and returns canonical JSON
    // with a final LF. Sorting does not mutate the supplied snapshot. This is a
    // writer only; it does not establish import or gameplay-validation support.
    std::string SerializeCanonical(const Snapshot &snapshot);
}

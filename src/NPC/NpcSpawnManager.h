#pragma once

#include "NpcSpawnId.h"

#include <map>
#include <optional>
#include <set>

struct NpcSpawnDefinition;

class NpcSpawnManager
{
public:
    bool AddSpawn(const NpcSpawnDefinition &definition);
    bool HasSpawn(NpcSpawnId spawnId) const;
    bool HasCapacity(NpcSpawnId spawnId) const;
    bool RegisterEntity(NpcSpawnId spawnId, int entityId, int currentTick);
    bool UnregisterEntity(int entityId);
    std::optional<NpcSpawnId> GetSpawnId(int entityId) const;
    int GetActiveCount(NpcSpawnId spawnId) const;

    bool IsWanderDue(int entityId, int currentTick) const;
    std::optional<int> BeginWanderAttempt(int entityId, int currentTick);
    bool ResetWanderState(int entityId, int currentTick);

private:
    struct SpawnRuntime
    {
        int maximumActiveCount = 0;
        int wanderIntervalTicks = 0;
        std::set<int> entityIds;
    };

    struct WanderRuntime
    {
        int nextEligibleTick = 0;
        int candidateIndex = 0;
    };

    std::map<NpcSpawnId, SpawnRuntime> spawns;
    std::map<int, NpcSpawnId> entitySpawns;
    std::map<int, WanderRuntime> wanderStates;
};

#include "NpcSpawnManager.h"

#include "NpcSpawnDefinition.h"

#include <limits>

namespace
{
    int NextTick(int currentTick, int interval)
    {
        if (interval <= 0 || currentTick > std::numeric_limits<int>::max() - interval)
            return std::numeric_limits<int>::max();
        return currentTick + interval;
    }
}

bool NpcSpawnManager::AddSpawn(const NpcSpawnDefinition &definition)
{
    if (!IsValidNpcSpawnId(definition.spawnId) ||
        definition.maximumActiveCount <= 0 ||
        definition.wanderRadius < 0 ||
        (definition.wanderRadius > 0 && definition.wanderIntervalTicks <= 0) ||
        (definition.wanderRadius == 0 && definition.wanderIntervalTicks != 0))
        return false;

    return spawns.emplace(
        definition.spawnId,
        SpawnRuntime{definition.maximumActiveCount, definition.wanderIntervalTicks, {}}).second;
}

bool NpcSpawnManager::HasSpawn(NpcSpawnId spawnId) const
{
    return IsValidNpcSpawnId(spawnId) && spawns.contains(spawnId);
}

bool NpcSpawnManager::HasCapacity(NpcSpawnId spawnId) const
{
    auto found = spawns.find(spawnId);
    return found != spawns.end() &&
           static_cast<int>(found->second.entityIds.size()) < found->second.maximumActiveCount;
}

bool NpcSpawnManager::RegisterEntity(NpcSpawnId spawnId, int entityId, int currentTick)
{
    auto spawn = spawns.find(spawnId);
    if (entityId <= 0 || spawn == spawns.end() || !HasCapacity(spawnId) ||
        entitySpawns.contains(entityId))
        return false;

    try
    {
        const auto [member, memberInserted] =
            spawn->second.entityIds.insert(entityId);
        (void)member;
        if (!memberInserted)
            return false;

        const auto [association, associationInserted] =
            entitySpawns.emplace(entityId, spawnId);
        (void)association;
        if (!associationInserted)
        {
            spawn->second.entityIds.erase(entityId);
            return false;
        }

        if (spawn->second.wanderIntervalTicks > 0)
        {
            const auto [state, stateInserted] = wanderStates.emplace(
                entityId, WanderRuntime{NextTick(currentTick, spawn->second.wanderIntervalTicks), 0});
            (void)state;
            if (!stateInserted)
            {
                entitySpawns.erase(entityId);
                spawn->second.entityIds.erase(entityId);
                return false;
            }
        }
    }
    catch (...)
    {
        wanderStates.erase(entityId);
        entitySpawns.erase(entityId);
        spawn->second.entityIds.erase(entityId);
        throw;
    }

    return true;
}

bool NpcSpawnManager::UnregisterEntity(int entityId)
{
    auto association = entitySpawns.find(entityId);
    if (association == entitySpawns.end())
        return false;
    auto spawn = spawns.find(association->second);
    if (spawn != spawns.end())
        spawn->second.entityIds.erase(entityId);
    entitySpawns.erase(association);
    wanderStates.erase(entityId);
    return true;
}

std::optional<NpcSpawnId> NpcSpawnManager::GetSpawnId(int entityId) const
{
    auto found = entitySpawns.find(entityId);
    return found == entitySpawns.end() ? std::nullopt : std::optional<NpcSpawnId>{found->second};
}

int NpcSpawnManager::GetActiveCount(NpcSpawnId spawnId) const
{
    auto found = spawns.find(spawnId);
    return found == spawns.end() ? 0 : static_cast<int>(found->second.entityIds.size());
}

bool NpcSpawnManager::IsWanderDue(int entityId, int currentTick) const
{
    auto association = entitySpawns.find(entityId);
    auto state = wanderStates.find(entityId);
    if (association == entitySpawns.end() || state == wanderStates.end())
        return false;
    auto spawn = spawns.find(association->second);
    return spawn != spawns.end() && spawn->second.wanderIntervalTicks > 0 &&
           currentTick >= state->second.nextEligibleTick;
}

std::optional<int> NpcSpawnManager::BeginWanderAttempt(int entityId, int currentTick)
{
    if (!IsWanderDue(entityId, currentTick))
        return std::nullopt;
    WanderRuntime &state = wanderStates.at(entityId);
    const int candidateIndex = state.candidateIndex;
    state.candidateIndex = (state.candidateIndex + 1) % 4;
    const NpcSpawnId spawnId = entitySpawns.at(entityId);
    state.nextEligibleTick = NextTick(currentTick, spawns.at(spawnId).wanderIntervalTicks);
    return candidateIndex;
}

bool NpcSpawnManager::ResetWanderState(int entityId, int currentTick)
{
    auto association = entitySpawns.find(entityId);
    auto state = wanderStates.find(entityId);
    if (association == entitySpawns.end() || state == wanderStates.end())
        return false;
    state->second = WanderRuntime{
        NextTick(currentTick, spawns.at(association->second).wanderIntervalTicks), 0};
    return true;
}

#include "CraftingSessionSystem.h"

#include "../Inventory/InventoryStackOperations.h"

#include <limits>

const CraftingSessionSnapshot &CraftingSessionSystem::Open(
    int actorEntityID, int stationObjectID)
{
    auto existing = sessions.find(actorEntityID);
    if (existing != sessions.end())
    {
        return existing->second;
    }

    CraftingSessionSnapshot session;
    session.sessionId = nextSessionId++;
    if (session.sessionId == InvalidCraftingSessionId)
        session.sessionId = nextSessionId++;
    session.actorEntityID = actorEntityID;
    session.stationObjectID = stationObjectID;
    sessions[actorEntityID] = session;
    return sessions.at(actorEntityID);
}

const CraftingSessionSnapshot *CraftingSessionSystem::Get(int actorEntityID) const
{
    auto found = sessions.find(actorEntityID);
    return found == sessions.end() ? nullptr : &found->second;
}

CraftingSessionSnapshot *CraftingSessionSystem::GetMutable(int actorEntityID)
{
    auto found = sessions.find(actorEntityID);
    return found == sessions.end() ? nullptr : &found->second;
}

bool CraftingSessionSystem::ReplaceState(
    int actorEntityID, CraftingSessionId sessionId,
    std::uint64_t expectedRevision, const InventorySlot &cursor,
    const std::array<InventorySlot, 9> &inputSlots)
{
    CraftingSessionSnapshot *session = GetMutable(actorEntityID);
    if (session == nullptr || sessionId == InvalidCraftingSessionId ||
        session->sessionId != sessionId || session->revision != expectedRevision ||
        expectedRevision == std::numeric_limits<std::uint64_t>::max() ||
        !InventoryStackOperations::IsCanonicalStack(cursor))
        return false;

    for (const InventorySlot &slot : inputSlots)
        if (!InventoryStackOperations::IsCanonicalStack(slot))
            return false;

    session->cursor = cursor;
    session->inputSlots = inputSlots;
    ++session->revision;
    return true;
}

void CraftingSessionSystem::Remove(int actorEntityID)
{
    sessions.erase(actorEntityID);
}

std::vector<int> CraftingSessionSystem::GetActorEntityIDs() const
{
    std::vector<int> actors;
    actors.reserve(sessions.size());
    for (const auto &[actorEntityID, session] : sessions)
    {
        (void)session;
        actors.push_back(actorEntityID);
    }
    return actors;
}

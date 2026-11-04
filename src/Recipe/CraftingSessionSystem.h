#pragma once

#include <map>
#include <vector>

#include "CraftingSession.h"

class CraftingSessionSystem
{
public:
    const CraftingSessionSnapshot &Open(int actorEntityID, int stationObjectID);
    const CraftingSessionSnapshot *Get(int actorEntityID) const;
    CraftingSessionSnapshot *GetMutable(int actorEntityID);
    bool ReplaceState(int actorEntityID, CraftingSessionId sessionId,
                      std::uint64_t expectedRevision,
                      const InventorySlot &cursor,
                      const std::array<InventorySlot, 9> &inputSlots);
    void Remove(int actorEntityID);
    std::vector<int> GetActorEntityIDs() const;

private:
    CraftingSessionId nextSessionId = 1;
    std::map<int, CraftingSessionSnapshot> sessions;
};

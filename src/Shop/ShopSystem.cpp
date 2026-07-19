#include "ShopSystem.h"

#include <limits>

ShopSystem::ShopSystem(ShopSessionId firstSessionId)
    : nextSessionId(firstSessionId)
{
}

ShopSessionId ShopSystem::Start(int actorEntityID, int npcEntityID,
    NpcType npcType, ShopId shopId, int openedTick)
{
    if (actorEntityID <= 0 || npcEntityID <= 0 || npcType == NpcType::NONE ||
        shopId == ShopId::NONE || nextSessionId == InvalidShopSessionId)
        return InvalidShopSessionId;

    const ShopSessionId allocated = nextSessionId;
    if (nextSessionId == std::numeric_limits<ShopSessionId>::max())
        nextSessionId = InvalidShopSessionId;
    else
        ++nextSessionId;

    sessionsByActorID.insert_or_assign(actorEntityID,
        ActiveShopSession{allocated, actorEntityID, npcEntityID, npcType,
                          shopId, openedTick});
    return allocated;
}

bool ShopSystem::Close(int actorEntityID, ShopSessionId sessionId)
{
    const auto found = sessionsByActorID.find(actorEntityID);
    if (sessionId == InvalidShopSessionId || found == sessionsByActorID.end() ||
        found->second.sessionId != sessionId)
        return false;
    sessionsByActorID.erase(found);
    return true;
}

bool ShopSystem::CancelActor(int actorEntityID)
{
    return sessionsByActorID.erase(actorEntityID) != 0;
}

std::size_t ShopSystem::CancelTarget(int npcEntityID)
{
    std::size_t removed = 0;
    for (auto it = sessionsByActorID.begin(); it != sessionsByActorID.end();)
        if (it->second.npcEntityID == npcEntityID)
        {
            it = sessionsByActorID.erase(it);
            ++removed;
        }
        else ++it;
    return removed;
}

const ActiveShopSession *ShopSystem::GetSession(int actorEntityID) const
{
    const auto found = sessionsByActorID.find(actorEntityID);
    return found == sessionsByActorID.end() ? nullptr : &found->second;
}

std::size_t ShopSystem::GetSessionCount() const
{
    return sessionsByActorID.size();
}

#pragma once

#include "ShopId.h"
#include "ShopSessionId.h"
#include "../NPC/NpcType.h"

#include <cstddef>
#include <unordered_map>

struct ActiveShopSession
{
    ShopSessionId sessionId = InvalidShopSessionId;
    int actorEntityID = 0;
    int npcEntityID = 0;
    NpcType npcType = NpcType::NONE;
    ShopId shopId = ShopId::NONE;
    int openedTick = 0;
};

class ShopSystem
{
public:
    explicit ShopSystem(ShopSessionId firstSessionId = 1);
    ShopSessionId Start(int actorEntityID, int npcEntityID, NpcType npcType,
                        ShopId shopId, int openedTick);
    bool Close(int actorEntityID, ShopSessionId sessionId);
    bool CancelActor(int actorEntityID);
    std::size_t CancelTarget(int npcEntityID);
    const ActiveShopSession *GetSession(int actorEntityID) const;
    std::size_t GetSessionCount() const;

private:
    std::unordered_map<int, ActiveShopSession> sessionsByActorID;
    ShopSessionId nextSessionId;
};

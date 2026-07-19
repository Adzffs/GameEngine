#pragma once

#include "ShopId.h"
#include "ShopSessionId.h"
#include "../Inventory/ItemType.h"
#include "../NPC/NpcType.h"

#include <optional>
#include <string>
#include <vector>

struct ShopEventEntry
{
    ItemType itemType;
    std::optional<int> buyPrice;
    std::optional<int> sellPrice;
};

struct ShopOpenedEvent
{
    int actorEntityID;
    int npcEntityID;
    NpcType npcType;
    ShopSessionId sessionId;
    ShopId shopId;
    std::string shopName;
    ItemType currencyItemType;
    std::vector<ShopEventEntry> entries;
};

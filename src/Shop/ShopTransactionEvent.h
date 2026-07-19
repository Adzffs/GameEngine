#pragma once

#include "ShopId.h"
#include "ShopSessionId.h"
#include "ShopTransactionType.h"
#include "../Inventory/ItemType.h"

struct ShopTransactionEvent
{
    int actorEntityID;
    int npcEntityID;
    ShopSessionId sessionId;
    ShopId shopId;
    ShopTransactionType transactionType;
    ItemType itemType;
    int quantity;
    ItemType currencyItemType;
    int unitPrice;
    int totalPrice;
};

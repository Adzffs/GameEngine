#pragma once

#include "ShopDefinition.h"

namespace ShopDefinitionDatabase
{
    const std::vector<ShopId> &GetAllShopIds();
    const ShopDefinition *TryGet(ShopId id);
    const ShopEntryDefinition *TryGetEntry(ShopId id, ItemType itemType);
}

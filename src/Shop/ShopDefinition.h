#pragma once

#include "ShopId.h"
#include "../Inventory/ItemType.h"

#include <optional>
#include <string>
#include <vector>

struct ShopEntryDefinition
{
    ItemType itemType = ItemType::NONE;
    std::optional<int> buyPrice;
    std::optional<int> sellPrice;
};

struct ShopDefinition
{
    ShopId id = ShopId::NONE;
    std::string name;
    ItemType currencyItemType = ItemType::NONE;
    std::vector<ShopEntryDefinition> entries;
};

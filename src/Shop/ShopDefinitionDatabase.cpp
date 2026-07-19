#include "ShopDefinitionDatabase.h"

#include <algorithm>

namespace
{
    const ShopDefinition DevelopmentGuideSupplies{
        ShopId::DEVELOPMENT_GUIDE_SUPPLIES,
        "Development supplies",
        ItemType::COINS,
        {
            {ItemType::LOG, std::nullopt, 1},
            {ItemType::COPPER_ORE, std::nullopt, 2},
            {ItemType::TIN_ORE, std::nullopt, 2},
            {ItemType::COAL, std::nullopt, 4},
            {ItemType::COOKED_MEAT, 8, 3},
            {ItemType::BRONZE_AXE, 25, 10},
            {ItemType::BRONZE_PICKAXE, 25, 10},
            {ItemType::BRONZE_SWORD, 40, 15},
            {ItemType::WOODEN_SHIELD, 30, 12},
        }};
}

namespace ShopDefinitionDatabase
{
    const std::vector<ShopId> &GetAllShopIds()
    {
        static const std::vector<ShopId> ids{
            ShopId::DEVELOPMENT_GUIDE_SUPPLIES};
        return ids;
    }

    const ShopDefinition *TryGet(ShopId id)
    {
        switch (id)
        {
        case ShopId::DEVELOPMENT_GUIDE_SUPPLIES:
            return &DevelopmentGuideSupplies;
        case ShopId::NONE:
        default:
            return nullptr;
        }
    }

    const ShopEntryDefinition *TryGetEntry(ShopId id, ItemType itemType)
    {
        const ShopDefinition *shop = TryGet(id);
        if (shop == nullptr || itemType == ItemType::NONE)
            return nullptr;
        const auto found = std::find_if(shop->entries.begin(), shop->entries.end(),
            [itemType](const ShopEntryDefinition &entry)
            { return entry.itemType == itemType; });
        return found == shop->entries.end() ? nullptr : &*found;
    }
}

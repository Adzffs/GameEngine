#include "ItemTagDatabase.h"

bool ItemTagDatabase::IsValid(ItemTag tag)
{
    return tag == ItemTag::LOGS || tag == ItemTag::PLANKS ||
           tag == ItemTag::ORES || tag == ItemTag::COALS;
}

bool ItemTagDatabase::Contains(ItemTag tag, ItemType itemType)
{
    switch (tag)
    {
    case ItemTag::LOGS:
        return itemType == ItemType::LOG || itemType == ItemType::OAK_LOG ||
               itemType == ItemType::WILLOW_LOG;
    case ItemTag::PLANKS:
        return itemType == ItemType::WOODEN_PLANKS;
    case ItemTag::ORES:
        return itemType == ItemType::COPPER_ORE ||
               itemType == ItemType::TIN_ORE || itemType == ItemType::IRON_ORE;
    case ItemTag::COALS:
        return itemType == ItemType::COAL;
    case ItemTag::NONE:
    default:
        return false;
    }
}

#include "ItemDatabase.h"

const ItemDefinition &ItemDatabase::Get(
    ItemType itemType)
{
    static const ItemDefinition none{
        ItemType::NONE,
        "None",
        false};

    static const ItemDefinition log{
        ItemType::LOG,
        "Log",
        false};

    static const ItemDefinition coins{
        ItemType::COINS,
        "Coins",
        true};

    static const ItemDefinition bronzeAxe{
        ItemType::BRONZE_AXE,
        "Bronze axe",
        false};

    switch (itemType)
    {
    case ItemType::LOG:
        return log;

    case ItemType::COINS:
        return coins;

    case ItemType::BRONZE_AXE:
        return bronzeAxe;

    case ItemType::NONE:
    default:
        return none;
    }
}
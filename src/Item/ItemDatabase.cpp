#include "ItemDatabase.h"

const ItemDefinition &ItemDatabase::Get(
    ItemType itemType)
{
    static const ItemDefinition none{
        ItemType::NONE,
        "None",
        false,
        EquipmentSlotType::NONE,
        ToolType::NONE,
        SkillType::NONE,
        0,
        0};

    static const ItemDefinition log{
        ItemType::LOG,
        "Log",
        false,
        EquipmentSlotType::NONE,
        ToolType::NONE,
        SkillType::NONE,
        0,
        0};

    static const ItemDefinition coins{
        ItemType::COINS,
        "Coins",
        true,
        EquipmentSlotType::NONE,
        ToolType::NONE,
        SkillType::NONE,
        0,
        0};

    static const ItemDefinition bronzeAxe{
        ItemType::BRONZE_AXE,
        "Bronze axe",
        false,
        EquipmentSlotType::WEAPON,
        ToolType::AXE,
        SkillType::WOODCUTTING,
        1,
        5};

    static const ItemDefinition ironAxe{
        ItemType::IRON_AXE,
        "Iron axe",
        false,
        EquipmentSlotType::WEAPON,
        ToolType::AXE,
        SkillType::WOODCUTTING,
        5,
        4};

    static const ItemDefinition steelAxe{
        ItemType::STEEL_AXE,
        "Steel axe",
        false,
        EquipmentSlotType::WEAPON,
        ToolType::AXE,
        SkillType::WOODCUTTING,
        10,
        3};

    switch (itemType)
    {
    case ItemType::LOG:
        return log;

    case ItemType::COINS:
        return coins;

    case ItemType::BRONZE_AXE:
        return bronzeAxe;

    case ItemType::IRON_AXE:
        return ironAxe;

    case ItemType::STEEL_AXE:
        return steelAxe;

    case ItemType::NONE:
    default:
        return none;
    }
}
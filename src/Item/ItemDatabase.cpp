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

    static const ItemDefinition oakLog{
        ItemType::OAK_LOG,
        "Oak log",
        false,
        EquipmentSlotType::NONE,
        ToolType::NONE,
        SkillType::NONE,
        0,
        0};

    static const ItemDefinition willowLog{
        ItemType::WILLOW_LOG,
        "Willow log",
        false,
        EquipmentSlotType::NONE,
        ToolType::NONE,
        SkillType::NONE,
        0,
        0};

    static const ItemDefinition copperOre{
        ItemType::COPPER_ORE,
        "Copper ore",
        false,
        EquipmentSlotType::NONE,
        ToolType::NONE,
        SkillType::NONE,
        0,
        0};

    static const ItemDefinition tinOre{
        ItemType::TIN_ORE,
        "Tin ore",
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

    static const ItemDefinition bronzePickaxe{
        ItemType::BRONZE_PICKAXE,
        "Bronze pickaxe",
        false,
        EquipmentSlotType::WEAPON,
        ToolType::PICKAXE,
        SkillType::MINING,
        1,
        5};

    static const ItemDefinition ironPickaxe{
        ItemType::IRON_PICKAXE,
        "Iron pickaxe",
        false,
        EquipmentSlotType::WEAPON,
        ToolType::PICKAXE,
        SkillType::MINING,
        5,
        4};

    static const ItemDefinition steelPickaxe{
        ItemType::STEEL_PICKAXE,
        "Steel pickaxe",
        false,
        EquipmentSlotType::WEAPON,
        ToolType::PICKAXE,
        SkillType::MINING,
        10,
        3};

    switch (itemType)
    {
    case ItemType::LOG:
        return log;

    case ItemType::OAK_LOG:
        return oakLog;

    case ItemType::WILLOW_LOG:
        return willowLog;

    case ItemType::COPPER_ORE:
        return copperOre;

    case ItemType::TIN_ORE:
        return tinOre;

    case ItemType::COINS:
        return coins;

    case ItemType::BRONZE_AXE:
        return bronzeAxe;

    case ItemType::IRON_AXE:
        return ironAxe;

    case ItemType::STEEL_AXE:
        return steelAxe;

    case ItemType::BRONZE_PICKAXE:
        return bronzePickaxe;

    case ItemType::IRON_PICKAXE:
        return ironPickaxe;

    case ItemType::STEEL_PICKAXE:
        return steelPickaxe;

    case ItemType::NONE:
    default:
        return none;
    }
}
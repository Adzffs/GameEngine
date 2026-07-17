#include "ResourceDatabase.h"

const ResourceDefinition &ResourceDatabase::Get(
    ResourceType resourceType)
{
    static const ResourceDefinition normalTree{
        ResourceType::NORMAL_TREE,
        "Normal tree",
        SkillType::WOODCUTTING,
        1,
        ToolType::AXE,
        25,
        ItemType::LOG,
        1,
        70,
        1,
        10};

    static const ResourceDefinition oakTree{
        ResourceType::OAK_TREE,
        "Oak tree",
        SkillType::WOODCUTTING,
        5,
        ToolType::AXE,
        38,
        ItemType::OAK_LOG,
        1,
        55,
        3,
        15};

    static const ResourceDefinition willowTree{
        ResourceType::WILLOW_TREE,
        "Willow tree",
        SkillType::WOODCUTTING,
        10,
        ToolType::AXE,
        68,
        ItemType::WILLOW_LOG,
        1,
        45,
        5,
        20};

    static const ResourceDefinition copperRock{
        ResourceType::COPPER_ROCK,
        "Copper rock",
        SkillType::MINING,
        1,
        ToolType::PICKAXE,
        18,
        ItemType::COPPER_ORE,
        1,
        70,
        1,
        8};

    static const ResourceDefinition tinRock{
        ResourceType::TIN_ROCK,
        "Tin rock",
        SkillType::MINING,
        1,
        ToolType::PICKAXE,
        18,
        ItemType::TIN_ORE,
        1,
        65,
        1,
        8};

    switch (resourceType)
    {
    case ResourceType::OAK_TREE:
        return oakTree;

    case ResourceType::WILLOW_TREE:
        return willowTree;

    case ResourceType::COPPER_ROCK:
        return copperRock;

    case ResourceType::TIN_ROCK:
        return tinRock;

    case ResourceType::NORMAL_TREE:
    default:
        return normalTree;
    }
}
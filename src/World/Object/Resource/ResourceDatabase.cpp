#include "ResourceDatabase.h"

const ResourceDefinition &ResourceDatabase::Get(
    ResourceType resourceType)
{
    static const ResourceDefinition normalTree{
        ResourceType::NORMAL_TREE,
        "Normal tree",
        SkillType::WOODCUTTING,
        1,
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
        68,
        ItemType::WILLOW_LOG,
        1,
        45,
        5,
        20};

    switch (resourceType)
    {
    case ResourceType::OAK_TREE:
        return oakTree;

    case ResourceType::WILLOW_TREE:
        return willowTree;

    case ResourceType::NORMAL_TREE:
    default:
        return normalTree;
    }
}
#include "ItemDefinition.h"

#include <utility>

ItemDefinition::ItemDefinition(
    ItemType itemType,
    std::string name,
    bool stackable)
    : itemType(itemType),
      name(std::move(name)),
      stackable(stackable)
{
}

ItemType ItemDefinition::GetItemType() const
{
    return itemType;
}

const std::string &ItemDefinition::GetName() const
{
    return name;
}

bool ItemDefinition::IsStackable() const
{
    return stackable;
}
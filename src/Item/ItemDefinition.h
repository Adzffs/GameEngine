#pragma once

#include "../Inventory/ItemType.h"

#include <string>

class ItemDefinition
{
public:
    ItemDefinition(
        ItemType itemType,
        std::string name,
        bool stackable);

    ItemType GetItemType() const;
    const std::string &GetName() const;
    bool IsStackable() const;

private:
    ItemType itemType;
    std::string name;
    bool stackable;
};
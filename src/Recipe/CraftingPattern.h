#pragma once

#include <array>
#include <cstddef>

#include "../Inventory/ItemType.h"
#include "../Item/ItemTag.h"

enum class CraftingPatternType
{
    NONE,
    SHAPELESS,
    SHAPED
};

enum class CraftingIngredientType
{
    EMPTY,
    ITEM,
    TAG
};

struct CraftingIngredient
{
    CraftingIngredientType type = CraftingIngredientType::EMPTY;
    ItemType itemType = ItemType::NONE;
    ItemTag itemTag = ItemTag::NONE;

    constexpr CraftingIngredient() = default;
    constexpr CraftingIngredient(ItemType item)
        : type(item == ItemType::NONE ? CraftingIngredientType::EMPTY
                                     : CraftingIngredientType::ITEM),
          itemType(item)
    {
    }

    static constexpr CraftingIngredient ForTag(ItemTag tag)
    {
        CraftingIngredient result;
        result.type = CraftingIngredientType::TAG;
        result.itemTag = tag;
        return result;
    }

    constexpr bool IsEmpty() const
    {
        return type == CraftingIngredientType::EMPTY;
    }
};

struct CraftingPattern
{
    static constexpr std::size_t GridSize = 9;

    CraftingPatternType type = CraftingPatternType::NONE;
    std::array<CraftingIngredient, GridSize> slots{};
    bool allowHorizontalMirror = false;
    int priority = 0;
};

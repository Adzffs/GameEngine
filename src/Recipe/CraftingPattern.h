#pragma once

#include <array>
#include <cstddef>

#include "../Inventory/ItemType.h"

enum class CraftingPatternType
{
    NONE,
    SHAPELESS,
    SHAPED
};

struct CraftingPattern
{
    static constexpr std::size_t GridSize = 9;

    CraftingPatternType type = CraftingPatternType::NONE;
    std::array<ItemType, GridSize> slots{};
};

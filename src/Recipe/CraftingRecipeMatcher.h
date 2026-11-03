#pragma once

#include <array>

#include "CraftingPattern.h"

class RecipeDefinition;

class CraftingRecipeMatcher
{
public:
    using Grid = std::array<ItemType, CraftingPattern::GridSize>;

    static bool Matches(
        const CraftingPattern &pattern,
        const Grid &grid);

    static const RecipeDefinition *FindMatch(
        const Grid &grid);
};

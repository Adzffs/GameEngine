#include "CraftingRecipeMatcher.h"

#include <algorithm>
#include <map>
#include <optional>

#include "RecipeDatabase.h"
#include "RecipeDefinition.h"

namespace
{
    struct Bounds
    {
        int minimumColumn = 0;
        int minimumRow = 0;
        int width = 0;
        int height = 0;
    };

    std::optional<Bounds> GetOccupiedBounds(
        const CraftingRecipeMatcher::Grid &slots)
    {
        int minimumColumn = 3;
        int minimumRow = 3;
        int maximumColumn = -1;
        int maximumRow = -1;

        for (int index = 0;
             index < static_cast<int>(slots.size());
             ++index)
        {
            if (slots[index] == ItemType::NONE)
            {
                continue;
            }

            const int column = index % 3;
            const int row = index / 3;
            minimumColumn = std::min(minimumColumn, column);
            minimumRow = std::min(minimumRow, row);
            maximumColumn = std::max(maximumColumn, column);
            maximumRow = std::max(maximumRow, row);
        }

        if (maximumColumn < 0)
        {
            return std::nullopt;
        }

        return Bounds{
            minimumColumn,
            minimumRow,
            maximumColumn - minimumColumn + 1,
            maximumRow - minimumRow + 1};
    }

    std::map<ItemType, int> CountItems(
        const CraftingRecipeMatcher::Grid &slots)
    {
        std::map<ItemType, int> counts;

        for (ItemType itemType : slots)
        {
            if (itemType != ItemType::NONE)
            {
                counts[itemType]++;
            }
        }

        return counts;
    }
}

bool CraftingRecipeMatcher::Matches(
    const CraftingPattern &pattern,
    const Grid &grid)
{
    if (pattern.type != CraftingPatternType::SHAPELESS &&
        pattern.type != CraftingPatternType::SHAPED)
    {
        return false;
    }

    if (pattern.type == CraftingPatternType::SHAPELESS)
    {
        const std::map<ItemType, int> expected =
            CountItems(pattern.slots);

        return !expected.empty() &&
               expected == CountItems(grid);
    }

    const std::optional<Bounds> patternBounds =
        GetOccupiedBounds(pattern.slots);
    const std::optional<Bounds> gridBounds =
        GetOccupiedBounds(grid);

    if (!patternBounds.has_value() ||
        !gridBounds.has_value() ||
        patternBounds->width != gridBounds->width ||
        patternBounds->height != gridBounds->height)
    {
        return false;
    }

    for (int row = 0; row < patternBounds->height; ++row)
    {
        for (int column = 0; column < patternBounds->width; ++column)
        {
            const int patternIndex =
                (patternBounds->minimumRow + row) * 3 +
                patternBounds->minimumColumn + column;
            const int gridIndex =
                (gridBounds->minimumRow + row) * 3 +
                gridBounds->minimumColumn + column;

            if (pattern.slots[patternIndex] != grid[gridIndex])
            {
                return false;
            }
        }
    }

    return true;
}

const RecipeDefinition *CraftingRecipeMatcher::FindMatch(
    const Grid &grid)
{
    for (RecipeType recipeType : RecipeDatabase::GetAllRecipeTypes())
    {
        const RecipeDefinition &definition =
            RecipeDatabase::Get(recipeType);

        if (Matches(
                definition.GetCraftingPattern(),
                grid))
        {
            return &definition;
        }
    }

    return nullptr;
}

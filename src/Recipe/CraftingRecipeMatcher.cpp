#include "CraftingRecipeMatcher.h"

#include <algorithm>
#include <optional>
#include <vector>

#include "RecipeDatabase.h"
#include "RecipeDefinition.h"
#include "../Item/ItemTagDatabase.h"

namespace
{
struct Bounds { int column; int row; int width; int height; };

template <typename Slots, typename Empty>
std::optional<Bounds> OccupiedBounds(const Slots &slots, Empty empty)
{
    int minColumn = 3, minRow = 3, maxColumn = -1, maxRow = -1;
    for (int index = 0; index < static_cast<int>(slots.size()); ++index)
    {
        if (empty(slots[index])) continue;
        minColumn = std::min(minColumn, index % 3);
        minRow = std::min(minRow, index / 3);
        maxColumn = std::max(maxColumn, index % 3);
        maxRow = std::max(maxRow, index / 3);
    }
    if (maxColumn < 0) return std::nullopt;
    return Bounds{minColumn, minRow, maxColumn - minColumn + 1,
                  maxRow - minRow + 1};
}

bool MatchesShaped(const CraftingPattern &pattern,
                   const CraftingRecipeMatcher::Grid &grid,
                   const Bounds &patternBounds,
                   const Bounds &gridBounds,
                   bool mirrored)
{
    for (int row = 0; row < patternBounds.height; ++row)
        for (int column = 0; column < patternBounds.width; ++column)
        {
            const int sourceColumn = mirrored
                ? patternBounds.width - column - 1 : column;
            const int patternIndex = (patternBounds.row + row) * 3 +
                                     patternBounds.column + sourceColumn;
            const int gridIndex = (gridBounds.row + row) * 3 +
                                  gridBounds.column + column;
            if (!CraftingRecipeMatcher::IngredientMatches(
                    pattern.slots[patternIndex], grid[gridIndex]))
                return false;
        }
    return true;
}

bool MatchShapeless(const std::vector<CraftingIngredient> &ingredients,
                    const std::vector<ItemType> &items,
                    std::vector<bool> &used,
                    std::size_t ingredientIndex)
{
    if (ingredientIndex == ingredients.size()) return true;
    for (std::size_t index = 0; index < items.size(); ++index)
    {
        if (used[index] || !CraftingRecipeMatcher::IngredientMatches(
                               ingredients[ingredientIndex], items[index]))
            continue;
        used[index] = true;
        if (MatchShapeless(ingredients, items, used, ingredientIndex + 1))
            return true;
        used[index] = false;
    }
    return false;
}

int ExactCount(const CraftingPattern &pattern)
{
    return static_cast<int>(std::count_if(
        pattern.slots.begin(), pattern.slots.end(),
        [](const CraftingIngredient &ingredient)
        { return ingredient.type == CraftingIngredientType::ITEM; }));
}
}

bool CraftingRecipeMatcher::IngredientMatches(
    const CraftingIngredient &ingredient, ItemType itemType)
{
    switch (ingredient.type)
    {
    case CraftingIngredientType::EMPTY:
        return itemType == ItemType::NONE;
    case CraftingIngredientType::ITEM:
        return ingredient.itemType == itemType;
    case CraftingIngredientType::TAG:
        return ItemTagDatabase::Contains(ingredient.itemTag, itemType);
    default:
        return false;
    }
}

bool CraftingRecipeMatcher::Matches(
    const CraftingPattern &pattern, const Grid &grid)
{
    if (pattern.type == CraftingPatternType::SHAPELESS)
    {
        std::vector<CraftingIngredient> ingredients;
        std::vector<ItemType> items;
        for (const auto &ingredient : pattern.slots)
            if (!ingredient.IsEmpty()) ingredients.push_back(ingredient);
        for (ItemType item : grid)
            if (item != ItemType::NONE) items.push_back(item);
        if (ingredients.empty() || ingredients.size() != items.size())
            return false;
        std::stable_sort(ingredients.begin(), ingredients.end(),
            [](const CraftingIngredient &left,
               const CraftingIngredient &right)
            { return static_cast<int>(left.type) < static_cast<int>(right.type); });
        std::vector<bool> used(items.size(), false);
        return MatchShapeless(ingredients, items, used, 0);
    }
    if (pattern.type != CraftingPatternType::SHAPED) return false;

    const auto patternBounds = OccupiedBounds(pattern.slots,
        [](const CraftingIngredient &ingredient) { return ingredient.IsEmpty(); });
    const auto gridBounds = OccupiedBounds(grid,
        [](ItemType item) { return item == ItemType::NONE; });
    if (!patternBounds || !gridBounds ||
        patternBounds->width != gridBounds->width ||
        patternBounds->height != gridBounds->height)
        return false;
    return MatchesShaped(pattern, grid, *patternBounds, *gridBounds, false) ||
           (pattern.allowHorizontalMirror &&
            MatchesShaped(pattern, grid, *patternBounds, *gridBounds, true));
}

const RecipeDefinition *CraftingRecipeMatcher::FindMatch(const Grid &grid)
{
    const RecipeDefinition *best = nullptr;
    for (RecipeType type : RecipeDatabase::GetAllRecipeTypes())
    {
        const RecipeDefinition &candidate = RecipeDatabase::Get(type);
        if (!Matches(candidate.GetCraftingPattern(), grid)) continue;
        if (best == nullptr) { best = &candidate; continue; }
        const auto &left = candidate.GetCraftingPattern();
        const auto &right = best->GetCraftingPattern();
        if (left.priority < right.priority ||
            (left.priority == right.priority && ExactCount(left) > ExactCount(right)) ||
            (left.priority == right.priority && ExactCount(left) == ExactCount(right) &&
             static_cast<int>(candidate.GetRecipeType()) <
                 static_cast<int>(best->GetRecipeType())))
            best = &candidate;
    }
    return best;
}

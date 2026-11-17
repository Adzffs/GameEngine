#include "RecipeBook.h"

#include <algorithm>

#include "RecipeDatabase.h"

RecipeBook::RecipeBook()
    : knownRecipes(GetStarterRecipes())
{
}

bool RecipeBook::IsKnown(RecipeType recipeType) const
{
    return std::find(knownRecipes.begin(), knownRecipes.end(), recipeType) !=
        knownRecipes.end();
}

bool RecipeBook::Discover(RecipeType recipeType)
{
    if (recipeType == RecipeType::NONE ||
        RecipeDatabase::TryGet(recipeType) == nullptr ||
        IsKnown(recipeType))
        return false;
    knownRecipes.push_back(recipeType);
    newDiscoveries.push_back(recipeType);
    return true;
}

const std::vector<RecipeType> &RecipeBook::GetKnownRecipes() const
{
    return knownRecipes;
}

std::vector<RecipeType> RecipeBook::ConsumeNewDiscoveries()
{
    std::vector<RecipeType> discoveries = std::move(newDiscoveries);
    newDiscoveries.clear();
    return discoveries;
}

bool RecipeBook::TryRestore(const std::vector<RecipeType> &recipeTypes)
{
    std::vector<RecipeType> restored;
    restored.reserve(recipeTypes.size());
    for (RecipeType recipeType : recipeTypes)
    {
        if (recipeType == RecipeType::NONE ||
            RecipeDatabase::TryGet(recipeType) == nullptr ||
            std::find(restored.begin(), restored.end(), recipeType) !=
                restored.end())
            return false;
        restored.push_back(recipeType);
    }
    knownRecipes = std::move(restored);
    newDiscoveries.clear();
    return true;
}

const std::vector<RecipeType> &RecipeBook::GetStarterRecipes()
{
    static const std::vector<RecipeType> recipes{
        RecipeType::BRONZE_BAR,
        RecipeType::WOODEN_PLANKS,
        RecipeType::WOODEN_SHIELD,
        RecipeType::BRONZE_AXE,
        RecipeType::BRONZE_PICKAXE,
        RecipeType::BRONZE_SWORD};
    return recipes;
}

#pragma once

#include <vector>

#include "RecipeType.h"

class RecipeBook
{
public:
    RecipeBook();

    bool IsKnown(RecipeType recipeType) const;
    bool Discover(RecipeType recipeType);
    const std::vector<RecipeType> &GetKnownRecipes() const;
    std::vector<RecipeType> ConsumeNewDiscoveries();

    // Restore replaces starter knowledge and never emits discovery feedback.
    bool TryRestore(const std::vector<RecipeType> &recipeTypes);

    static const std::vector<RecipeType> &GetStarterRecipes();

private:
    std::vector<RecipeType> knownRecipes;
    std::vector<RecipeType> newDiscoveries;
};

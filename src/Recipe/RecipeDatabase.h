#pragma once

#include "RecipeDefinition.h"

#include <vector>

class RecipeDatabase
{
public:
    static const RecipeDefinition &
    Get(RecipeType recipeType);

    static const RecipeDefinition *
    TryGet(RecipeType recipeType);

    static const std::vector<RecipeType> &
    GetAllRecipeTypes();
};

#pragma once

#include "RecipeDefinition.h"

class RecipeDatabase
{
public:
    static const RecipeDefinition &
    Get(RecipeType recipeType);
};

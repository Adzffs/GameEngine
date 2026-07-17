#pragma once

#include "RecipeType.h"

class Player;

class RecipeSystem
{
public:
    static bool CanCreateRecipe(
        const Player &player,
        RecipeType recipeType);
};

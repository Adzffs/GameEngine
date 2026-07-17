#pragma once

#include "RecipeType.h"

class Player;

class RecipeSystem
{
public:
    static bool CanCreateRecipe(
        const Player &player,
        RecipeType recipeType);

    static bool TryCreateRecipe(
        Player &player,
        RecipeType recipeType);
};

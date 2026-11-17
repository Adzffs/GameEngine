#pragma once

#include "../Item/ItemDatabase.h"
#include "../Recipe/RecipeProductionPacing.h"
#include "../Shop/ShopDefinition.h"

#include <algorithm>
#include <optional>

class EconomyBalanceModel
{
public:
    static constexpr int TicksPerMinute = 100;

    static double GetRecipeCyclesPerMinute(
        const RecipeDefinition &recipe,
        int skillLevel)
    {
        const int duration = RecipeProductionPacing::GetDurationTicks(
            recipe, skillLevel);
        return static_cast<double>(TicksPerMinute) / duration;
    }

    static double GetRecipeItemsPerMinute(
        const RecipeDefinition &recipe,
        int skillLevel)
    {
        return GetRecipeCyclesPerMinute(recipe, skillLevel) *
            recipe.GetOutputAmount();
    }

    static double GetRecipeXPPerMinute(
        const RecipeDefinition &recipe,
        int skillLevel)
    {
        return GetRecipeCyclesPerMinute(recipe, skillLevel) *
            recipe.GetXPReward();
    }

    static std::optional<double> GetHealingPerCoin(
        ItemType itemType,
        const ShopDefinition &shop)
    {
        const ItemDefinition &item = ItemDatabase::Get(itemType);
        const FoodDefinition *food = item.GetFoodDefinition();
        const ShopEntryDefinition *entry = FindEntry(shop, itemType);
        if (food == nullptr || entry == nullptr ||
            !entry->buyPrice.has_value() || *entry->buyPrice <= 0)
            return std::nullopt;
        return static_cast<double>(food->healAmount) / *entry->buyPrice;
    }

    static std::optional<int> GetCraftingMargin(
        const RecipeDefinition &recipe,
        const ShopDefinition &shop)
    {
        int ingredientCost = 0;
        for (const RecipeIngredient &ingredient : recipe.GetIngredients())
        {
            const ShopEntryDefinition *entry =
                FindEntry(shop, ingredient.itemType);
            if (entry == nullptr || !entry->buyPrice.has_value())
                return std::nullopt;
            ingredientCost += *entry->buyPrice * ingredient.amount;
        }

        const ShopEntryDefinition *output =
            FindEntry(shop, recipe.GetOutputItem());
        if (output == nullptr || !output->sellPrice.has_value())
            return std::nullopt;
        return *output->sellPrice * recipe.GetOutputAmount() - ingredientCost;
    }

private:
    static const ShopEntryDefinition *FindEntry(
        const ShopDefinition &shop,
        ItemType itemType)
    {
        const auto found = std::find_if(shop.entries.begin(), shop.entries.end(),
            [itemType](const ShopEntryDefinition &entry)
            { return entry.itemType == itemType; });
        return found == shop.entries.end() ? nullptr : &*found;
    }
};

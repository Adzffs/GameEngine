#pragma once

#include "../Item/ItemDatabase.h"
#include "../Recipe/RecipeProductionPacing.h"
#include "../Recipe/RecipeDatabase.h"
#include "../Shop/ShopDefinition.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <optional>

class EconomyBalanceModel
{
public:
    static constexpr int TicksPerMinute = 100;
    static constexpr long double PriceTolerance = 0.000000001L;

    struct ProductionCosts
    {
        std::map<ItemType, long double> unitCosts;
        bool converged = false;
    };

    // Lowest fully shop-funded material cost, including intermediate recipes.
    // Fractional unit costs describe repeatable whole-batch economics, not an
    // immediately purchasable quantity. Unknown gathered inputs remain unknown.
    // Bound relaxation by catalogue size; unresolved improving cycles require
    // review rather than being reported as a passing economy check.
    static ProductionCosts GetProductionCosts(const ShopDefinition &shop)
    {
        ProductionCosts result;
        if (shop.currencyItemType != ItemType::NONE)
            result.unitCosts.emplace(shop.currencyItemType, 1.0L);
        for (const auto &entry : shop.entries)
            if (entry.itemType != ItemType::NONE && entry.itemType != shop.currencyItemType &&
                entry.buyPrice && *entry.buyPrice > 0)
            {
                const auto found = result.unitCosts.find(entry.itemType);
                if (found == result.unitCosts.end() || *entry.buyPrice < found->second)
                    result.unitCosts[entry.itemType] = *entry.buyPrice;
            }

        const std::size_t maximumPasses = ItemDatabase::GetAllItemTypes().size() + 1;
        for (std::size_t pass = 0; pass < maximumPasses; ++pass)
        {
            const auto previous = result.unitCosts;
            bool changed = false;
            for (const auto type : RecipeDatabase::GetAllRecipeTypes())
            {
                const auto &recipe = RecipeDatabase::Get(type);
                if (recipe.GetOutputAmount() <= 0 || recipe.GetOutputItem() == ItemType::NONE ||
                    recipe.GetIngredients().empty())
                    continue;
                long double cost = 0;
                bool available = true;
                for (const auto &ingredient : recipe.GetIngredients())
                {
                    const auto input = previous.find(ingredient.itemType);
                    if (ingredient.amount <= 0 || input == previous.end())
                    { available = false; break; }
                    cost += input->second * ingredient.amount;
                }
                if (!available || !std::isfinite(cost)) continue;
                cost /= recipe.GetOutputAmount();
                const auto existing = result.unitCosts.find(recipe.GetOutputItem());
                if (existing == result.unitCosts.end() || cost < existing->second - PriceTolerance)
                {
                    result.unitCosts[recipe.GetOutputItem()] = cost;
                    changed = true;
                }
            }
            if (!changed)
            {
                result.converged = true;
                break;
            }
        }
        return result;
    }

    // nullopt means a material has no shop-funded route or the output has no
    // cash-out price. It does not mean the production chain passed validation.
    static std::optional<long double> GetProductionMargin(
        const RecipeDefinition &recipe, const ShopDefinition &shop,
        const ProductionCosts &costs)
    {
        if (!costs.converged || recipe.GetOutputAmount() <= 0 || recipe.GetIngredients().empty())
            return std::nullopt;
        long double cost = 0;
        for (const auto &ingredient : recipe.GetIngredients())
        {
            const auto input = costs.unitCosts.find(ingredient.itemType);
            if (ingredient.amount <= 0 || input == costs.unitCosts.end()) return std::nullopt;
            cost += input->second * ingredient.amount;
        }
        if (recipe.GetOutputItem() == shop.currencyItemType)
            return recipe.GetOutputAmount() - cost;
        const auto *output = FindEntry(shop, recipe.GetOutputItem());
        if (output == nullptr || !output->sellPrice || *output->sellPrice <= 0)
            return std::nullopt;
        return static_cast<long double>(*output->sellPrice) * recipe.GetOutputAmount() - cost;
    }

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

#include "TestSupport.h"
#include "../src/Content/ContentValidator.h"
#include "../src/Economy/EconomyBalanceModel.h"
#include "../src/Recipe/RecipeDatabase.h"
#include "../src/Shop/ShopDefinitionDatabase.h"

int main()
{
    TestContext test;
    const ShopDefinition *shop = ShopDefinitionDatabase::TryGet(
        ShopId::DEVELOPMENT_GUIDE_SUPPLIES);
    test.Expect(shop != nullptr, "Development shop resolves");
    test.Expect(ShopDefinitionDatabase::TryGet(ShopId::NONE) == nullptr,
                "NONE does not resolve");
    test.Expect(ShopDefinitionDatabase::TryGet(static_cast<ShopId>(999)) == nullptr,
                "Unknown shop does not resolve");
    test.Expect(shop->name == "Development supplies", "Name is exact");
    test.Expect(shop->currencyItemType == ItemType::COINS, "Currency is coins");
    const ItemType order[] = {ItemType::LOG, ItemType::COPPER_ORE,
        ItemType::TIN_ORE, ItemType::COAL, ItemType::COOKED_MEAT,
        ItemType::HEALING_TONIC, ItemType::BRONZE_AXE, ItemType::BRONZE_PICKAXE,
        ItemType::BRONZE_SWORD, ItemType::WOODEN_SHIELD};
    test.ExpectEqual(shop->entries.size(), std::size_t{10}, "Ten ordered entries");
    for (std::size_t i = 0; i < shop->entries.size(); ++i)
        test.Expect(shop->entries[i].itemType == order[i], "Entry order is authored");
    const std::optional<int> buys[] = {10, std::nullopt,
        std::nullopt, std::nullopt, 8, 20, 25, 25, 40, 20};
    const std::optional<int> sells[] = {1, 2, 2, 4, 3, 6, 10, 10, 15, 12};
    for (std::size_t i = 0; i < shop->entries.size(); ++i)
    {
        test.Expect(shop->entries[i].buyPrice == buys[i], "Exact authored buy price");
        test.Expect(shop->entries[i].sellPrice == sells[i], "Exact authored sell price");
    }
    test.Expect(ContentValidator::ValidateShopDefinition(shop->id, *shop).IsValid(),
                "Authored shop validates");
    ShopDefinition legacyTimber = *shop;
    legacyTimber.entries.front().buyPrice = 1;
    test.Expect(!ContentValidator::ValidateShopDefinition(legacyTimber.id, legacyTimber).IsValid(),
        "Multi-step Log/plank/shield profit rejects even without a plank shop entry");
    legacyTimber.entries.front().buyPrice = 8;
    test.Expect(!ContentValidator::ValidateShopDefinition(legacyTimber.id, legacyTimber).IsValid(),
        "Material-complete break-even production is rejected as well as a positive loop");

    const std::optional<double> meatHealing =
        EconomyBalanceModel::GetHealingPerCoin(ItemType::COOKED_MEAT, *shop);
    const std::optional<double> tonicHealing =
        EconomyBalanceModel::GetHealingPerCoin(ItemType::HEALING_TONIC, *shop);
    test.Expect(
        meatHealing.has_value() && tonicHealing.has_value() &&
            *meatHealing > *tonicHealing,
        "Cooked meat remains the efficient option and tonic pays for convenience");

    const RecipeDefinition &bronzeBar =
        RecipeDatabase::Get(RecipeType::BRONZE_BAR);
    test.Expect(
        EconomyBalanceModel::GetRecipeItemsPerMinute(bronzeBar, 1) > 33.3 &&
            EconomyBalanceModel::GetRecipeItemsPerMinute(bronzeBar, 1) < 33.4 &&
            EconomyBalanceModel::GetRecipeXPPerMinute(bronzeBar, 1) == 200.0,
        "Balance model records the station-only Bronze production ceiling");

    ShopDefinition arbitrageFixture = *shop;
    arbitrageFixture.entries[1].buyPrice = 2;
    arbitrageFixture.entries[2].buyPrice = 2;
    arbitrageFixture.entries.push_back({ItemType::BRONZE_BAR, std::nullopt, 5});
    const std::optional<int> safeMargin =
        EconomyBalanceModel::GetCraftingMargin(bronzeBar, arbitrageFixture);
    test.Expect(safeMargin.has_value() && *safeMargin == 1,
        "Balance model exposes a profitable buy-craft-sell loop");
    arbitrageFixture.entries.back().sellPrice = 3;
    const std::optional<int> safeLoss =
        EconomyBalanceModel::GetCraftingMargin(bronzeBar, arbitrageFixture);
    test.Expect(safeLoss.has_value() && *safeLoss == -1,
        "Balance model exposes a safely lossy crafting trade");

    ShopDefinition invalid = *shop;
    invalid.entries.push_back(invalid.entries.front());
    test.Expect(!ContentValidator::ValidateShopDefinition(invalid.id, invalid).IsValid(),
                "Duplicate entries reject");
    invalid = *shop;
    invalid.entries.clear();
    test.Expect(!ContentValidator::ValidateShopDefinition(invalid.id, invalid).IsValid(),
                "Empty shop rejects");
    invalid = *shop;
    invalid.currencyItemType = ItemType::LOG;
    test.Expect(!ContentValidator::ValidateShopDefinition(invalid.id, invalid).IsValid(),
                "Non-stackable currency rejects");
    invalid = *shop;
    invalid.entries[4].sellPrice = 9;
    test.Expect(!ContentValidator::ValidateShopDefinition(invalid.id, invalid).IsValid(),
                "Sell above buy rejects");
    invalid = *shop;
    invalid.name.clear();
    test.Expect(!ContentValidator::ValidateShopDefinition(invalid.id, invalid).IsValid(),
                "Empty name rejects");
    invalid = *shop;
    invalid.entries[0].buyPrice = 0;
    test.Expect(!ContentValidator::ValidateShopDefinition(invalid.id, invalid).IsValid(),
                "Zero price rejects");
    invalid = *shop;
    invalid.entries[0].sellPrice = -1;
    test.Expect(!ContentValidator::ValidateShopDefinition(invalid.id, invalid).IsValid(),
                "Negative price rejects");
    invalid = *shop;
    invalid.entries[0] = {ItemType::LOG, std::nullopt, std::nullopt};
    test.Expect(!ContentValidator::ValidateShopDefinition(invalid.id, invalid).IsValid(),
                "Entry without operation rejects");
    invalid = *shop;
    invalid.entries[0].itemType = ItemType::COINS;
    test.Expect(!ContentValidator::ValidateShopDefinition(invalid.id, invalid).IsValid(),
                "Currency stock entry rejects");
    invalid = *shop;
    invalid.entries[0].itemType = static_cast<ItemType>(999);
    test.Expect(!ContentValidator::ValidateShopDefinition(invalid.id, invalid).IsValid(),
                "Unknown entry item rejects");
    test.Expect(!ContentValidator::ValidateShopDefinitions({*shop, *shop}).IsValid(),
                "Duplicate shop IDs reject");
    return test.Finish();
}

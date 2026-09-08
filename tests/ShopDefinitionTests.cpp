#include "TestSupport.h"
#include "../src/Content/ContentValidator.h"
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
        ItemType::BRONZE_AXE, ItemType::BRONZE_PICKAXE,
        ItemType::BRONZE_SWORD, ItemType::WOODEN_SHIELD};
    test.ExpectEqual(shop->entries.size(), std::size_t{9}, "Nine ordered entries");
    for (std::size_t i = 0; i < shop->entries.size(); ++i)
        test.Expect(shop->entries[i].itemType == order[i], "Entry order is authored");
    const std::optional<int> buys[] = {1, std::nullopt,
        std::nullopt, std::nullopt, 8, 25, 25, 40, 30};
    const std::optional<int> sells[] = {1, 2, 2, 4, 3, 10, 10, 15, 12};
    for (std::size_t i = 0; i < shop->entries.size(); ++i)
    {
        test.Expect(shop->entries[i].buyPrice == buys[i], "Exact authored buy price");
        test.Expect(shop->entries[i].sellPrice == sells[i], "Exact authored sell price");
    }
    test.Expect(ContentValidator::ValidateShopDefinition(shop->id, *shop).IsValid(),
                "Authored shop validates");

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

#include "TestSupport.h"

#include "../src/Content/ContentValidator.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Item/ItemDatabase.h"
#include "../src/Persistence/PersistenceTokenCodec.h"
#include "../src/Player/Player.h"
#include "../src/Player/PlayerInitializationMode.h"
#include "../src/Recipe/CraftingRecipeMatcher.h"
#include "../src/Recipe/RecipeDatabase.h"
#include "../src/Recipe/RecipeSystem.h"

int main()
{
    TestContext test;

    test.ExpectEqual(
        static_cast<int>(RecipeType::WOODEN_PLANKS),
        4,
        "Wooden-planks recipe keeps a stable appended numeric ID");
    test.ExpectEqual(
        static_cast<int>(RecipeType::WOODEN_SHIELD),
        5,
        "Wooden-shield recipe keeps a stable appended numeric ID");
    test.ExpectEqual(
        static_cast<int>(ItemType::WOODEN_PLANKS),
        22,
        "Wooden planks keep a stable appended numeric item ID");
    test.ExpectEqual(
        static_cast<int>(StationType::CRAFTING_TABLE),
        2,
        "Crafting Table keeps a stable appended station ID");
    test.Expect(
        ItemDatabase::Get(ItemType::WOODEN_PLANKS).IsStackable(),
        "Wooden planks are a stackable material");
    test.Expect(
        PersistenceTokenCodec::TryGetItemToken(
            ItemType::WOODEN_PLANKS) == "WOODEN_PLANKS" &&
            PersistenceTokenCodec::TryParseItemToken(
                "WOODEN_PLANKS") == ItemType::WOODEN_PLANKS,
        "Wooden planks have a stable persistence token");

    const RecipeDefinition &planks =
        RecipeDatabase::Get(RecipeType::WOODEN_PLANKS);
    const RecipeDefinition &shield =
        RecipeDatabase::Get(RecipeType::WOODEN_SHIELD);

    test.Expect(
        planks.GetRequiredStationType() ==
                StationType::CRAFTING_TABLE &&
            planks.GetCraftingPattern().type ==
                CraftingPatternType::SHAPELESS &&
            planks.GetOutputItem() == ItemType::WOODEN_PLANKS &&
            planks.GetOutputAmount() == 4 &&
            !planks.RepeatsWhileValid(),
        "One Log authoritatively defines four non-repeating Wooden planks");
    test.Expect(
        shield.GetRequiredStationType() ==
                StationType::CRAFTING_TABLE &&
            shield.GetCraftingPattern().type ==
                CraftingPatternType::SHAPED &&
            shield.GetIngredients().size() == 1 &&
            shield.GetIngredients()[0].itemType ==
                ItemType::WOODEN_PLANKS &&
            shield.GetIngredients()[0].amount == 6 &&
            !shield.RepeatsWhileValid(),
        "Wooden shield is a six-plank shaped non-repeating recipe");

    CraftingRecipeMatcher::Grid grid{};
    grid[8] = ItemType::LOG;
    test.Expect(
        CraftingRecipeMatcher::FindMatch(grid) == &planks,
        "Shapeless Log recipe matches in any grid slot");
    grid[0] = ItemType::LOG;
    test.Expect(
        CraftingRecipeMatcher::FindMatch(grid) == nullptr,
        "Shapeless recipe rejects extra materials");

    grid = shield.GetCraftingPattern().slots;
    test.Expect(
        CraftingRecipeMatcher::FindMatch(grid) == &shield,
        "Exact authored shield grid matches the shield recipe");
    const CraftingRecipeMatcher::Grid rotatedShield{
        ItemType::NONE, ItemType::WOODEN_PLANKS, ItemType::NONE,
        ItemType::WOODEN_PLANKS, ItemType::WOODEN_PLANKS, ItemType::WOODEN_PLANKS,
        ItemType::WOODEN_PLANKS, ItemType::NONE, ItemType::WOODEN_PLANKS};
    test.Expect(
        CraftingRecipeMatcher::FindMatch(rotatedShield) == nullptr,
        "Shaped recipes do not rotate implicitly");

    const CraftingPattern translatedPattern{
        CraftingPatternType::SHAPED,
        {
            ItemType::LOG, ItemType::LOG, ItemType::NONE,
            ItemType::LOG, ItemType::NONE, ItemType::NONE,
            ItemType::NONE, ItemType::NONE, ItemType::NONE,
        }};
    const CraftingRecipeMatcher::Grid translatedGrid{
        ItemType::NONE, ItemType::NONE, ItemType::NONE,
        ItemType::NONE, ItemType::LOG, ItemType::LOG,
        ItemType::NONE, ItemType::LOG, ItemType::NONE};
    test.Expect(
        CraftingRecipeMatcher::Matches(
            translatedPattern,
            translatedGrid),
        "Shaped patterns can translate within the grid");
    const CraftingRecipeMatcher::Grid rotatedPatternGrid{
        ItemType::NONE, ItemType::NONE, ItemType::NONE,
        ItemType::NONE, ItemType::LOG, ItemType::LOG,
        ItemType::NONE, ItemType::NONE, ItemType::LOG};
    test.Expect(
        !CraftingRecipeMatcher::Matches(
            translatedPattern,
            rotatedPatternGrid),
        "Translated shaped patterns still reject rotation and mirroring");

    Player player(1, PlayerInitializationMode::EMPTY);
    player.GetInventory().AddItem(ItemType::LOG, 1);
    test.Expect(
        RecipeSystem::TryCreateRecipe(
            player,
            RecipeType::WOODEN_PLANKS),
        "Plank transaction succeeds with one Log");
    test.Expect(
        player.GetInventory().GetItemAmount(ItemType::LOG) == 0 &&
            player.GetInventory().GetItemAmount(
                ItemType::WOODEN_PLANKS) == 4,
        "Plank transaction consumes one Log and adds exactly four planks");

    player.GetInventory().AddItem(ItemType::WOODEN_PLANKS, 2);
    test.Expect(
        RecipeSystem::TryCreateRecipe(
            player,
            RecipeType::WOODEN_SHIELD),
        "Shield transaction succeeds with six Wooden planks");
    test.Expect(
        player.GetInventory().GetItemAmount(
            ItemType::WOODEN_PLANKS) == 0 &&
            player.GetInventory().GetItemAmount(
                ItemType::WOODEN_SHIELD) == 1,
        "Shield transaction consumes exactly six planks and adds one shield");

    Player fullPlayer(2, PlayerInitializationMode::EMPTY);
    fullPlayer.GetInventory().AddItem(ItemType::WOODEN_PLANKS, 7);
    while (fullPlayer.GetInventory().AddItem(ItemType::LOG, 1))
    {
    }
    test.Expect(
        !RecipeSystem::TryCreateRecipe(
            fullPlayer,
            RecipeType::WOODEN_SHIELD),
        "Full inventory rejects shield output when the plank stack remains");
    test.Expect(
        fullPlayer.GetInventory().GetItemAmount(
            ItemType::WOODEN_PLANKS) == 7 &&
            fullPlayer.GetInventory().GetItemAmount(
                ItemType::WOODEN_SHIELD) == 0,
        "Failed output insertion rolls back all ingredient removal atomically");

    test.Expect(
        RecipeDatabase::TryGet(static_cast<RecipeType>(999)) == nullptr,
        "Unknown recipe IDs are rejected without throwing");
    test.Expect(
        ContentValidator::ValidateAll().IsValid(),
        "Complete crafting content catalogue validates");

    return test.Finish();
}

#include "TestSupport.h"

#include "../src/Player/Player.h"
#include "../src/Player/PlayerInitializationMode.h"
#include "../src/Recipe/RecipeBook.h"

int main()
{
    TestContext test;

    Player player(1, PlayerInitializationMode::EMPTY);
    RecipeBook &book = player.GetRecipeBook();
    test.Expect(
        book.GetKnownRecipes() == RecipeBook::GetStarterRecipes(),
        "Every new player receives the canonical starter recipe set");
    test.Expect(
        book.IsKnown(RecipeType::BRONZE_BAR) &&
            book.IsKnown(RecipeType::WOODEN_PLANKS) &&
            book.IsKnown(RecipeType::BRONZE_SWORD) &&
            !book.IsKnown(RecipeType::IRON_BAR),
        "Starter knowledge covers the first loop without revealing upgrades");

    test.Expect(
        book.Discover(RecipeType::IRON_BAR) &&
            book.IsKnown(RecipeType::IRON_BAR),
        "A valid unknown recipe can be discovered once");
    test.Expect(
        !book.Discover(RecipeType::IRON_BAR) &&
            !book.Discover(RecipeType::NONE) &&
            !book.Discover(static_cast<RecipeType>(999)),
        "Duplicate and invalid discoveries are rejected");
    const std::vector<RecipeType> discoveries = book.ConsumeNewDiscoveries();
    test.Expect(
        discoveries == std::vector<RecipeType>{RecipeType::IRON_BAR} &&
            book.ConsumeNewDiscoveries().empty(),
        "New discovery feedback is emitted exactly once");

    const std::vector<RecipeType> restored{
        RecipeType::WOODEN_PLANKS,
        RecipeType::OAK_PLANKS,
        RecipeType::IRON_AXE};
    test.Expect(
        book.TryRestore(restored) &&
            book.GetKnownRecipes() == restored &&
            book.ConsumeNewDiscoveries().empty(),
        "Restore replaces starter knowledge without replaying notifications");
    test.Expect(
        !book.TryRestore({RecipeType::IRON_BAR, RecipeType::IRON_BAR}) &&
            book.GetKnownRecipes() == restored,
        "Invalid restore data cannot partially mutate known recipes");

    return test.Finish();
}

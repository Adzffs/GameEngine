#include "TestSupport.h"

#include "../src/Player/Player.h"
#include "../src/Recipe/RecipeDatabase.h"
#include "../src/World/Object/Station/CraftingStation.h"
#include "../src/World/World.h"

int main()
{
    TestContext test;
    World world;
    const int playerID = world.CreatePlayer();
    Player *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
    CraftingStation *furnace = world.GetStationAt(14, 9);
    test.Expect(player != nullptr && furnace != nullptr,
        "Discovery integration fixture has a player and forge");
    if (player == nullptr || furnace == nullptr) return test.Finish();

    player->GetPosition().SetPosition(13, 9);
    world.QueueStationInteraction(playerID, furnace->GetID());
    world.Update();
    player->GetInventory().AddItem(ItemType::IRON_ORE, 1);

    test.Expect(
        !player->GetRecipeBook().IsKnown(RecipeType::IRON_BAR) &&
            !world.TryStartRecipeAction(playerID, RecipeType::IRON_BAR),
        "An undiscovered forge recipe is rejected authoritatively");

    test.Expect(
        player->GetRecipeBook().Discover(RecipeType::IRON_BAR) &&
            world.TryStartRecipeAction(playerID, RecipeType::IRON_BAR),
        "A discovered forge recipe becomes available immediately");
    for (int tick = 0;
         tick < RecipeDatabase::Get(RecipeType::IRON_BAR)
                    .GetActionDurationTicks();
         ++tick)
        world.Update();

    test.Expect(
        player->GetInventory().GetItemAmount(ItemType::IRON_BAR) == 1 &&
            player->GetRecipeBook().IsKnown(RecipeType::IRON_AXE) &&
            player->GetRecipeBook().IsKnown(RecipeType::IRON_PICKAXE),
        "Processing the first Iron bar reveals its equipment recipes");
    const std::vector<RecipeType> discoveries =
        player->GetRecipeBook().ConsumeNewDiscoveries();
    test.Expect(
        discoveries == std::vector<RecipeType>{
            RecipeType::IRON_BAR,
            RecipeType::IRON_AXE,
            RecipeType::IRON_PICKAXE},
        "Discovery events preserve the authored progression order");

    return test.Finish();
}

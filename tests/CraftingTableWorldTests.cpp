#include "TestSupport.h"

#include "../src/Action/Action.h"
#include "../src/Command/CommandProcessingResult.h"
#include "../src/Player/Player.h"
#include "../src/Recipe/CraftingRecipeMatcher.h"
#include "../src/Recipe/RecipeDatabase.h"
#include "../src/World/World.h"

namespace
{
    CommandResultCode FindResult(
        const World &world,
        std::uint64_t commandID)
    {
        for (const CommandProcessingResult &result :
             world.GetCommandProcessingResults())
        {
            if (result.commandID == commandID)
            {
                return result.resultCode;
            }
        }

        return CommandResultCode::GAMEPLAY_REJECTED;
    }

    Player *CreatePlayer(World &world)
    {
        return dynamic_cast<Player *>(
            world.GetEntityByID(world.CreatePlayer()));
    }

    bool OpenStation(
        World &world,
        Player &player,
        int stationX,
        int stationY,
        StationType expectedType)
    {
        CraftingStation *station =
            world.GetStationAt(stationX, stationY);

        if (station == nullptr)
        {
            return false;
        }

        player.GetPosition().SetPosition(
            stationX - 1,
            stationY);
        world.QueueStationInteraction(
            player.GetID(),
            station->GetID());
        world.Update();

        StationType openedType = StationType::NONE;
        return world.ConsumeOpenedStation(
                   player.GetID(),
                   openedType) &&
               openedType == expectedType;
    }

    CraftingRecipeMatcher::Grid PlankGrid()
    {
        CraftingRecipeMatcher::Grid grid{};
        grid[4] = ItemType::LOG;
        return grid;
    }
}

int main()
{
    TestContext test;

    {
        World world;
        Player *player = CreatePlayer(world);
        test.Expect(
            player != nullptr &&
                OpenStation(
                    world,
                    *player,
                    17,
                    9,
                    StationType::CRAFTING_TABLE),
            "Development Crafting Table opens through the real interaction path");

        player->GetInventory().AddItem(ItemType::LOG, 2);
        const std::uint64_t commandID =
            world.EnqueueCommand(
                CraftRecipeCommand{
                    player->GetID(),
                    PlankGrid()});
        world.Update();

        test.Expect(
            FindResult(world, commandID) ==
                CommandResultCode::ACCEPTED,
            "Server accepts a valid authoritative Log grid");
        test.Expect(
            player->GetInventory().GetItemAmount(ItemType::LOG) == 1 &&
                player->GetInventory().GetItemAmount(
                    ItemType::WOODEN_PLANKS) == 4,
            "One-tick World action consumes one Log and creates four planks");
        test.Expect(
            world.GetActionForEntity(player->GetID()) == nullptr,
            "Crafting Table recipes complete once instead of auto-repeating");

        player->GetInventory().AddItem(ItemType::WOODEN_PLANKS, 2);
        const std::uint64_t shieldCommand =
            world.EnqueueCommand(
                CraftRecipeCommand{
                    player->GetID(),
                    RecipeDatabase::Get(
                        RecipeType::WOODEN_SHIELD)
                        .GetCraftingPattern()
                        .slots});
        world.Update();
        test.Expect(
            FindResult(world, shieldCommand) ==
                    CommandResultCode::ACCEPTED &&
                world.GetActionForEntity(player->GetID()) != nullptr,
            "Server resolves the exact shield grid and starts its action");
        world.Update();
        test.Expect(
            player->GetInventory().GetItemAmount(
                ItemType::WOODEN_PLANKS) == 0 &&
                player->GetInventory().GetItemAmount(
                    ItemType::WOODEN_SHIELD) == 1 &&
                world.GetActionForEntity(player->GetID()) == nullptr,
            "Shield completes once with an exact six-plank transaction");
    }

    {
        World world;
        Player *player = CreatePlayer(world);
        OpenStation(
            world,
            *player,
            17,
            9,
            StationType::CRAFTING_TABLE);

        CraftingRecipeMatcher::Grid invalidGrid{};
        invalidGrid[0] = ItemType::COPPER_ORE;
        const std::uint64_t noMatch =
            world.EnqueueCommand(
                CraftRecipeCommand{
                    player->GetID(),
                    invalidGrid});
        world.Update();
        test.Expect(
            FindResult(world, noMatch) ==
                    CommandResultCode::GAMEPLAY_REJECTED &&
                world.GetActionForEntity(player->GetID()) == nullptr,
            "Valid item tokens with no exact recipe are gameplay-rejected");

        invalidGrid.fill(ItemType::NONE);
        invalidGrid[0] = static_cast<ItemType>(999);
        const std::uint64_t invalidToken =
            world.EnqueueCommand(
                CraftRecipeCommand{
                    player->GetID(),
                    invalidGrid});
        world.Update();
        test.Expect(
            FindResult(world, invalidToken) ==
                CommandResultCode::INVALID_COMMAND_DATA,
            "Unknown grid item tokens are invalid command data");

        player->GetInventory().AddItem(ItemType::LOG, 1);
        const std::uint64_t bypass =
            world.EnqueueCommand(
                StartRecipeCommand{
                    player->GetID(),
                    RecipeType::WOODEN_PLANKS});
        world.Update();
        test.Expect(
            FindResult(world, bypass) ==
                    CommandResultCode::INVALID_COMMAND_DATA &&
                player->GetInventory().GetItemAmount(
                    ItemType::WOODEN_PLANKS) == 0,
            "Direct recipe command cannot bypass Crafting Table grid matching");
    }

    {
        World world;
        Player *player = CreatePlayer(world);
        test.Expect(
            OpenStation(
                world,
                *player,
                14,
                9,
                StationType::FURNACE),
            "Furnace fixture opens authoritatively");
        player->GetInventory().AddItem(ItemType::LOG, 1);
        const std::uint64_t wrongStation =
            world.EnqueueCommand(
                CraftRecipeCommand{
                    player->GetID(),
                    PlankGrid()});
        world.Update();
        test.Expect(
            FindResult(world, wrongStation) ==
                    CommandResultCode::GAMEPLAY_REJECTED &&
                player->GetInventory().GetItemAmount(ItemType::LOG) == 1 &&
                player->GetInventory().GetItemAmount(
                    ItemType::WOODEN_PLANKS) == 0,
            "Crafting grid cannot execute at the wrong station");
    }

    {
        World world;
        Player *player = CreatePlayer(world);
        OpenStation(
            world,
            *player,
            17,
            9,
            StationType::CRAFTING_TABLE);
        player->GetInventory().AddItem(ItemType::LOG, 1);
        player->GetPosition().SetPosition(0, 0);
        const std::uint64_t staleStation =
            world.EnqueueCommand(
                CraftRecipeCommand{
                    player->GetID(),
                    PlankGrid()});
        world.Update();
        test.Expect(
            FindResult(world, staleStation) ==
                    CommandResultCode::GAMEPLAY_REJECTED &&
                !world.HasActiveStation(player->GetID()) &&
                player->GetInventory().GetItemAmount(ItemType::LOG) == 1,
            "Moving away invalidates the authoritative station before crafting");
    }

    {
        World world;
        Player *player = CreatePlayer(world);
        OpenStation(
            world,
            *player,
            17,
            9,
            StationType::CRAFTING_TABLE);
        const std::uint64_t missingMaterial =
            world.EnqueueCommand(
                CraftRecipeCommand{
                    player->GetID(),
                    PlankGrid()});
        world.Update();
        test.Expect(
            FindResult(world, missingMaterial) ==
                    CommandResultCode::GAMEPLAY_REJECTED &&
                player->GetInventory().GetItemAmount(
                    ItemType::WOODEN_PLANKS) == 0,
            "Grid intent never substitutes for missing server inventory");
    }

    {
        World world;
        Player *player = CreatePlayer(world);
        OpenStation(
            world,
            *player,
            17,
            9,
            StationType::CRAFTING_TABLE);
        player->GetInventory().AddItem(ItemType::LOG, 2);
        const std::uint64_t first = world.EnqueueCommand(
            CraftRecipeCommand{player->GetID(), PlankGrid()});
        const std::uint64_t second = world.EnqueueCommand(
            CraftRecipeCommand{player->GetID(), PlankGrid()});
        world.Update();
        test.Expect(
            FindResult(world, first) == CommandResultCode::ACCEPTED &&
                FindResult(world, second) ==
                    CommandResultCode::GAMEPLAY_REJECTED,
            "World FIFO accepts only the first same-tick crafting command");
        test.Expect(
            player->GetInventory().GetItemAmount(ItemType::LOG) == 1 &&
                player->GetInventory().GetItemAmount(
                    ItemType::WOODEN_PLANKS) == 4,
            "Same-tick duplicate commands cannot double-spend ingredients");
    }

    return test.Finish();
}

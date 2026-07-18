#include "TestSupport.h"

#include "../src/Equipment/EquipmentSlotType.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Item/ItemDatabase.h"
#include "../src/Player/Player.h"
#include "../src/Recipe/RecipeDatabase.h"
#include "../src/Recipe/RecipeType.h"
#include "../src/World/World.h"

namespace
{
    Player *GetPlayer(
        World &world,
        int playerID)
    {
        return dynamic_cast<Player *>(
            world.GetEntityByID(playerID));
    }

    int FindInventorySlot(
        const Inventory &inventory,
        ItemType itemType)
    {
        const auto &slots = inventory.GetSlots();

        for (int index = 0; index < static_cast<int>(slots.size()); ++index)
        {
            if (!slots[index].IsEmpty() &&
                slots[index].GetItemType() == itemType)
            {
                return index;
            }
        }

        return -1;
    }

    void AdvanceWorldTicks(
        World &world,
        int ticks)
    {
        for (int index = 0; index < ticks; ++index)
        {
            world.Update();
        }
    }

    void EquipItem(
        TestContext &test,
        World &world,
        int playerID,
        ItemType itemType)
    {
        Player *player = GetPlayer(world, playerID);
        int slotIndex = FindInventorySlot(
            player->GetInventory(),
            itemType);

        test.Expect(
            slotIndex >= 0,
            "Expected item exists in inventory for equip test setup");
        test.Expect(
            world.TryEquipInventoryItem(
                playerID,
                slotIndex),
            "Expected item equips successfully for test setup");
    }

    void OpenFurnace(
        TestContext &test,
        World &world,
        int playerID)
    {
        CraftingStation *station =
            world.GetStationAt(14, 9);

        test.Expect(
            station != nullptr,
            "Furnace exists in the world");

        world.QueueStationInteraction(
            playerID,
            station->GetID());
        world.Update();
    }
}

int main()
{
    TestContext test;

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(4, 5);

        ResourceNode *resource = world.GetResourceAt(5, 5);

        test.Expect(
            resource != nullptr,
            "Normal tree exists in the world");

        world.QueueResourceInteraction(
            playerID,
            resource->GetID());
        world.Update();

        test.Expect(
            world.GetActionForEntity(playerID) == nullptr,
            "Missing tool prevents gathering from starting");
        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::LOG),
            0,
            "Missing tool does not award gathering rewards");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(4, 5);
        EquipItem(
            test,
            world,
            playerID,
            ItemType::BRONZE_AXE);

        ResourceNode *resource = world.GetResourceAt(5, 5);
        int initialLogCount =
            player->GetInventory().GetItemAmount(ItemType::LOG);

        world.QueueResourceInteraction(
            playerID,
            resource->GetID());
        world.Update();

        test.Expect(
            world.GetActionForEntity(playerID) != nullptr,
            "Valid gathering interaction starts an action");

        player->GetEquipment().Unequip(EquipmentSlotType::WEAPON);

        AdvanceWorldTicks(
            world,
            ItemDatabase::Get(ItemType::BRONZE_AXE)
                .GetActionDurationTicks());

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::LOG),
            initialLogCount,
            "Removing the tool before completion prevents gathering rewards");
        test.Expect(
            world.GetActionForEntity(playerID) == nullptr,
            "Tool removal before completion stops gathering repetition");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(4, 5);
        EquipItem(
            test,
            world,
            playerID,
            ItemType::BRONZE_AXE);

        while (player->GetInventory().AddItem(
            ItemType::LOG,
            1))
        {
        }

        int fullInventoryLogCount =
            player->GetInventory().GetItemAmount(ItemType::LOG);

        ResourceNode *resource = world.GetResourceAt(5, 5);

        world.QueueResourceInteraction(
            playerID,
            resource->GetID());
        world.Update();

        AdvanceWorldTicks(
            world,
            ItemDatabase::Get(ItemType::BRONZE_AXE)
                .GetActionDurationTicks());

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::LOG),
            fullInventoryLogCount,
            "Full inventory prevents gathering rewards");
        test.Expect(
            world.GetActionForEntity(playerID) == nullptr,
            "Full inventory prevents gathering repetition");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(4, 5);
        EquipItem(
            test,
            world,
            playerID,
            ItemType::BRONZE_AXE);

        ResourceNode *resource = world.GetResourceAt(5, 5);

        world.QueueResourceInteraction(
            playerID,
            resource->GetID());
        world.Update();

        resource->ConsumeUse();

        AdvanceWorldTicks(
            world,
            ItemDatabase::Get(ItemType::BRONZE_AXE)
                .GetActionDurationTicks());

        test.Expect(
            !resource->IsActive(),
            "Test setup depleted the resource before completion handling");
        test.Expect(
            world.GetActionForEntity(playerID) == nullptr,
            "Depleted resources prevent gathering repetition");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(13, 9);
        OpenFurnace(
            test,
            world,
            playerID);

        player->GetInventory().AddItem(ItemType::COPPER_ORE, 1);
        player->GetInventory().AddItem(ItemType::TIN_ORE, 1);

        test.Expect(
            world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Recipe starts before ingredient revalidation");

        player->GetInventory().RemoveItem(
            ItemType::TIN_ORE,
            1);

        AdvanceWorldTicks(
            world,
            RecipeDatabase::Get(RecipeType::BRONZE_BAR)
                .GetActionDurationTicks());

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::BRONZE_BAR),
            0,
            "Removing an ingredient before completion prevents smithing output");
        test.Expect(
            world.GetActionForEntity(playerID) == nullptr,
            "Ingredient revalidation stops the recipe action after completion");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(13, 9);
        OpenFurnace(
            test,
            world,
            playerID);

        player->GetInventory().AddItem(ItemType::COPPER_ORE, 1);
        player->GetInventory().AddItem(ItemType::TIN_ORE, 1);

        test.Expect(
            world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Valid smithing action starts");

        AdvanceWorldTicks(
            world,
            RecipeDatabase::Get(RecipeType::BRONZE_BAR)
                .GetActionDurationTicks());

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COPPER_ORE),
            0,
            "Smithing consumes the correct copper input");
        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::TIN_ORE),
            0,
            "Smithing consumes the correct tin input");
        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::BRONZE_BAR),
            1,
            "Smithing awards one output per completed cycle");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(13, 9);
        OpenFurnace(
            test,
            world,
            playerID);

        player->GetInventory().AddItem(ItemType::COPPER_ORE, 2);
        player->GetInventory().AddItem(ItemType::TIN_ORE, 2);

        test.Expect(
            world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Repeating smithing action starts with enough materials");

        AdvanceWorldTicks(
            world,
            RecipeDatabase::Get(RecipeType::BRONZE_BAR)
                    .GetActionDurationTicks() *
                2);

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::BRONZE_BAR),
            2,
            "Repeating smithing produces one bar per cycle until materials run out");
        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COPPER_ORE),
            0,
            "Repeating smithing stops after consuming the last copper ore");
        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::TIN_ORE),
            0,
            "Repeating smithing stops after consuming the last tin ore");
        test.Expect(
            world.GetActionForEntity(playerID) == nullptr,
            "Smithing does not restart once materials are exhausted");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(0, 0);
        player->GetInventory().AddItem(ItemType::COPPER_ORE, 1);
        player->GetInventory().AddItem(ItemType::TIN_ORE, 1);

        test.Expect(
            !world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Smithing cannot start without the required station in range");
        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::BRONZE_BAR),
            0,
            "Failed smithing validation does not award output");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(13, 9);
        OpenFurnace(
            test,
            world,
            playerID);

        player->GetInventory().AddItem(ItemType::COPPER_ORE, 1);
        player->GetInventory().AddItem(ItemType::TIN_ORE, 1);

        test.Expect(
            world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Smithing action starts before range validation changes");

        player->GetPosition().SetPosition(0, 0);

        AdvanceWorldTicks(
            world,
            RecipeDatabase::Get(RecipeType::BRONZE_BAR)
                .GetActionDurationTicks());

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::COPPER_ORE),
            1,
            "Range validation failure prevents consuming copper ore");
        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::TIN_ORE),
            1,
            "Range validation failure prevents consuming tin ore");
        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::BRONZE_BAR),
            0,
            "Range validation failure prevents smithing rewards");
    }

    return test.Finish();
}
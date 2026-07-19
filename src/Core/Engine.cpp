#include "Engine.h"
#include <chrono>
#include <thread>
#include "Core/Logger.h"
#include "../Action/ActionCancelReason.h"
#include "../Equipment/EquipmentSlotType.h"
#include "../Movement/MovementDestinationRequest.h"
#include "../Inventory/ItemType.h"
#include "../Player/Player.h"
#include "../Skills/SkillType.h"
#include "../Recipe/RecipeType.h"
#include "EngineClickRouting.h"

namespace
{
    int FindInventorySlot(
        const Inventory &inventory,
        ItemType itemType)
    {
        const auto &slots = inventory.GetSlots();

        for (int slotIndex = 0;
             slotIndex < static_cast<int>(slots.size());
             ++slotIndex)
        {
            if (!slots[slotIndex].IsEmpty() &&
                slots[slotIndex].GetItemType() == itemType)
            {
                return slotIndex;
            }
        }

        return -1;
    }
}

Engine::Engine()
    : playerID(-1)
{
    if (!graphics.Initialize())
    {
        running = false;
        return;
    }

    playerID = world.CreatePlayer();

    Player *player =
        dynamic_cast<Player *>(
            world.GetEntityByID(playerID));

    if (player != nullptr &&
        player->GetInventory().AddItem(
            ItemType::DEVELOPER_GODSWORD,
            1))
    {
        int weaponSlotIndex = FindInventorySlot(
            player->GetInventory(),
            ItemType::DEVELOPER_GODSWORD);

        if (weaponSlotIndex >= 0)
        {
            world.TryEquipInventoryItem(
                playerID,
                weaponSlotIndex);
        }
    }
}

void Engine::Run()
{
    Logger::Info("Server Started");

    while (running)
    {
        graphics.ProcessEvents(running);

        if (!running)
        {
            break;
        }

        RecipeType requestedRecipe =
            RecipeType::NONE;

        if (graphics.ConsumeRecipeRequest(
                requestedRecipe))
        {
            world.TryStartRecipeAction(
                playerID,
                requestedRecipe);
        }

        int clickedInventorySlot = -1;

        if (graphics.ConsumeInventorySlotClick(
                clickedInventorySlot))
        {
            if (!world.TryConsumeFood(
                    playerID,
                    clickedInventorySlot))
            {
                world.TryEquipInventoryItem(
                    playerID,
                    clickedInventorySlot);
            }
        }

        if (graphics.ConsumeWeaponSlotClick())
        {
            world.TryUnequipItem(
                playerID,
                EquipmentSlotType::WEAPON);
        }

        int clickedTileX;
        int clickedTileY;
        int clickedMouseX;
        int clickedMouseY;

        if (graphics.ConsumeClickedTile(
                clickedTileX,
                clickedTileY,
                clickedMouseX,
                clickedMouseY))
        {
            EngineClickRouting::HandleWorldClick(
                graphics,
                world,
                playerID,
                clickedTileX,
                clickedTileY,
                clickedMouseX,
                clickedMouseY);
        }

        if (graphics.ConsumeStationMenuClose())
        {
            world.CancelActionsForEntity(
                playerID,
                ActionCancelReason::INTERFACE_CLOSED);

            world.CloseStationInteraction(
                playerID);
        }

        if (clock.ShouldTick())
        {
            Update();
        }
        else
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1));
        }

        Player *player =
            dynamic_cast<Player *>(
                world.GetEntityByID(playerID));

        StationType openedStationType =
            StationType::NONE;

        if (world.ConsumeOpenedStation(
                playerID,
                openedStationType))
        {
            graphics.OpenStationMenu(
                openedStationType);
        }

        if (player != nullptr)
        {
            const Skill &woodcutting =
                player->GetSkills().GetSkill(
                    SkillType::WOODCUTTING);

            const Skill &mining =
                player->GetSkills().GetSkill(
                    SkillType::MINING);

            graphics.Render(
                world.GetMap(),
                world.GetEntities(),
                world.GetResources(),
                world.GetStations(),
                world.GetMeleeCombatFeedbacks(),
                player->GetPosition().GetX(),
                player->GetPosition().GetY(),
                player->GetInventory(),
                player->GetEquipment(),
                *player,
                world.GetActionForEntity(
                    playerID));
        }
    }
    Logger::Info("Server stopped");
}

void Engine::Update()
{
    tick++;
    world.Update();
}
#include "Engine.h"
#include <chrono>
#include <thread>
#include "Core/Logger.h"
#include "../Action/ActionCancelReason.h"
#include "../Movement/MovementDestinationRequest.h"
#include "../Player/Player.h"
#include "../Skills/SkillType.h"
#include "../Recipe/RecipeType.h"

Engine::Engine()
    : playerID(-1)
{
    if (!graphics.Initialize())
    {
        running = false;
        return;
    }

    playerID = world.CreatePlayer();
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
            world.TryEquipInventoryItem(
                playerID,
                clickedInventorySlot);
        }

        if (graphics.ConsumeWeaponSlotClick())
        {
            world.TryUnequipWeapon(
                playerID);
        }

        int clickedTileX;
        int clickedTileY;

        if (graphics.ConsumeClickedTile(
                clickedTileX,
                clickedTileY))
        {
            world.CancelActionsForEntity(
                playerID,
                ActionCancelReason::PLAYER_MOVED);

            world.CloseStationInteraction(
                playerID);

            world.ClearPendingResourceInteraction(
                playerID);

            ResourceNode *resource =
                world.GetResourceAt(
                    clickedTileX,
                    clickedTileY);

            CraftingStation *station =
                world.GetStationAt(
                    clickedTileX,
                    clickedTileY);

            if (resource != nullptr &&
                resource->IsActive())
            {
                world.QueueResourceInteraction(
                    playerID,
                    resource->GetID());
            }
            else if (station != nullptr)
            {
                world.QueueStationInteraction(
                    playerID,
                    station->GetID());
            }

            MovementDestinationRequest request(
                playerID,
                clickedTileX,
                clickedTileY);

            world.QueueMovementDestination(request);
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
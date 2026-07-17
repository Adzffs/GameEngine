#include "Engine.h"
#include <chrono>
#include <thread>
#include "Core/Logger.h"
#include "../Movement/MovementDestinationRequest.h"
#include "../Player/Player.h"
#include "../Skills/SkillType.h"

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
            world.CancelActionsForEntity(playerID);

            world.ClearPendingResourceInteraction(
                playerID);

            ResourceNode *resource =
                world.GetResourceAt(
                    clickedTileX,
                    clickedTileY);

            if (resource != nullptr &&
                resource->IsActive())
            {
                world.QueueResourceInteraction(
                    playerID,
                    resource->GetID());
            }

            MovementDestinationRequest request(
                playerID,
                clickedTileX,
                clickedTileY);

            world.QueueMovementDestination(request);
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
                player->GetPosition().GetX(),
                player->GetPosition().GetY(),
                player->GetInventory(),
                player->GetEquipment(),
                *player);
        }
    }
    Logger::Info("Server stopped");
}

void Engine::Update()
{
    tick++;
    world.Update();
}
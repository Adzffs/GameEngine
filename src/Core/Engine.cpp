#include "Engine.h"
#include <chrono>
#include <iostream>
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
        int clickedTileX;
        int clickedTileY;

        if (graphics.ConsumeClickedTile(
                clickedTileX,
                clickedTileY))
        {
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

                std::cout
                    << "Resource interaction queued for Entity "
                    << playerID
                    << " and Resource "
                    << resource->GetID()
                    << std::endl;
            }
            else
            {
                world.ClearPendingResourceInteraction(
                    playerID);

                if (resource != nullptr)
                {
                    std::cout
                        << "Resource "
                        << resource->GetID()
                        << " is depleted."
                        << std::endl;
                }
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

            graphics.Render(
                world.GetMap(),
                world.GetEntities(),
                world.GetResources(),
                player->GetPosition().GetX(),
                player->GetPosition().GetY(),
                player->GetInventory(),
                woodcutting.GetLevel(),
                woodcutting.GetXP(),
                woodcutting.GetXPForCurrentLevel(),
                woodcutting.GetXPForNextLevel());
        }
    }
}

void Engine::Update()
{
    tick++;
    world.Update();
}
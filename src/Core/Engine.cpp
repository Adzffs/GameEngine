#include "Engine.h"
#include <chrono>
#include <iostream>
#include <thread>
#include "Core/Logger.h"
#include "../Movement/MovementDestinationRequest.h"

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

        Entity *player =
            world.GetEntityByID(playerID);

        if (player != nullptr)
        {
            graphics.Render(
                world.GetMap(),
                world.GetEntities(),
                world.GetResources(),
                player->GetPosition().GetX(),
                player->GetPosition().GetY());
        }
    }
}

void Engine::Update()
{
    tick++;
    world.Update();
}
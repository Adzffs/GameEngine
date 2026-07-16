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
            MovementDestinationRequest request(
                playerID,
                clickedTileX,
                clickedTileY);

            world.QueueMovementDestination(request);

            std::cout
                << "Destination queued for Entity ID: "
                << playerID
                << " Destination: ("
                << clickedTileX
                << ", "
                << clickedTileY
                << ")"
                << std::endl;
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
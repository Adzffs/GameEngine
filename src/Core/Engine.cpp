#include "Engine.h"
#include <chrono>
#include <iostream>
#include <thread>
#include "Core/Logger.h"

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
                player->GetPosition().GetX(),
                player->GetPosition().GetY());
        }
    }
}

void Engine::Update()
{
    tick++;

    std::cout
        << "Server Tick: "
        << tick
        << std::endl;

    world.Update();

    // Remove later when the engine should run continuously.
    if (tick >= 10)
    {
        running = false;
    }
}
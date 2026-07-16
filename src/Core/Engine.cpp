#include "Engine.h"
#include <iostream>
#include <thread>
#include "Core/Logger.h"

void Engine::Run()
{
    Logger::Info("Server Started");

    while (running)
    {
        if (clock.ShouldTick())
        {
            Update();
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
    // Revmove later when i dont just need 10 ticks
    if (tick >= 10)
    {
        running = false;
    }
}
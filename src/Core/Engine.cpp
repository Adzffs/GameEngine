#include "Engine.h"
#include <iostream>
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

    if (tick >= 10)
    {
        running = false;
    }
}
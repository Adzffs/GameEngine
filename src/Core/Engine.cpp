#include "Engine.h"
#include <iostream>

void Engine::Run()
{
    std::cout << "Server Started\n";

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

    if (tick >= 10)
    {
        running = false;
    }
}
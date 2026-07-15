#include "Engine.h"
#include <iostream>
#include <thread>
#include <chrono>

void Engine::Run()
{
    for (int tick = 0; tick < 10; tick++)
    {
        std::cout
            << "Game Tick: "
            << tick
            << std::endl;

        std::this_thread::sleep_for(
            std::chrono::milliseconds(600));
    }

    std::cout << "Engine Finished" << std::endl;
}
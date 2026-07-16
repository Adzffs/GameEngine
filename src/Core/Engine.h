#pragma once

#include "../Time/Clock.h"
#include "../World/World.h"
#include "../Input/InputManager.h"

class Engine
{
public:
    Engine();
    void Run();

private:
    bool running = true;

    Clock clock;
    World world;
    InputManager inputManager;

    int tick = 0;
    int playerID = -1;

    void Update();
};
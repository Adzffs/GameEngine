#pragma once

#include "../Time/Clock.h"
#include "../World/World.h"
#include "../Input/InputManager.h"
#include "../Graphics/Graphics.h"

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
    Graphics graphics;

    int playerID = -1;

    void Update();
};
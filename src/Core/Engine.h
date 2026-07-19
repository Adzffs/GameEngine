#pragma once

#include "../Time/Clock.h"
#include "../Time/TickPerformance.h"
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
    TickPerformanceStats tickPerformanceStats;

    int playerID = -1;

    static constexpr int MaxCatchUpTicks = 3;

    void Update();
};
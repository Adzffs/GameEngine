#pragma once

#include "../Time/Clock.h"
#include "../World/World.h"

class Engine
{
public:
    void Run();

private:
    bool running = true;

    Clock clock;

    World world;

    int tick = 0;

    void Update();
};
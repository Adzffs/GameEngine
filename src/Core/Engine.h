#pragma once

#include "../Time/Clock.h"

class Engine
{
public:
    void Run();

private:
    bool running = true;

    Clock clock;

    int tick = 0;

    void Update();
};
#pragma once

#include <chrono>

class Clock
{
public:
    Clock();

    bool ShouldTick();

private:
    std::chrono::steady_clock::time_point lastTick;

    const int tickRate = 600;
};
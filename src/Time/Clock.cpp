#include "Clock.h"

Clock::Clock()
{
    lastTick = std::chrono::steady_clock::now();
}

bool Clock::ShouldTick()
{
    auto now = std::chrono::steady_clock::now();

    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastTick)
            .count();

    if (elapsed >= tickRate)
    {
        lastTick = now;
        return true;
    }

    return false;
}
#include "Action.h"
#include <iostream>

Action::Action(std::string name, int duration)
{
    this->name = name;
    this->duration = duration;
    currentTick = 0;
}

void Action::Update()
{
    if (currentTick < duration)
    {
        currentTick++;

        std::cout
            << "Action: "
            << name
            << " Tick "
            << currentTick
            << "/"
            << duration
            << std::endl;
    }
}

bool Action::IsComplete()
{
    return currentTick >= duration;
}

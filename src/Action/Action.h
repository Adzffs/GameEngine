#pragma once

#include <string>

class Action
{
public:
    Action(std::string name, int duration);

    void Update();

    bool IsComplete();

private:
    std::string name;

    int duration;

    int currentTick;
};

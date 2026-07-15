#pragma once

#include "Action.h"
#include <vector>

class ActionManager
{
public:
    void AddAction(Action action);

    void Update();

private:
    std::vector<Action> actions;
};
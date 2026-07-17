#pragma once

#include "Action.h"

#include <vector>

class ActionManager
{
public:
    void AddAction(const Action &action);

    bool HasActionForEntity(int entityID) const;

    void CancelActionsForEntity(int entityID);

    std::vector<Action> Update();

private:
    std::vector<Action> actions;
};
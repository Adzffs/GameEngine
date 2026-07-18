#pragma once

#include "Action.h"

#include <vector>

class ActionManager
{
public:
    void StartAction(const Action &action);
    void RestartAction(const Action &action);

    // Temporary compatibility function.
    // Existing World code still calls AddAction.
    void AddAction(const Action &action);

    bool HasActionForEntity(int entityID) const;

    const Action *GetActionForEntity(
        int entityID) const;

    void CancelActionsForEntity(
        int entityID,
        ActionCancelReason reason =
            ActionCancelReason::NONE);

    std::vector<Action> Update();

private:
    std::vector<Action> actions;

    int currentTick = 0;
};
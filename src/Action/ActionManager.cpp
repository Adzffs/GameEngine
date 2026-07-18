#include "ActionManager.h"

void ActionManager::StartAction(
    const Action &action)
{
    CancelActionsForEntity(
        action.GetEntityID(),
        ActionCancelReason::NEW_ACTION_STARTED);

    actions.push_back(action);
    actions.back().Start(currentTick);
}

void ActionManager::AddAction(
    const Action &action)
{
    StartAction(action);
}

bool ActionManager::HasActionForEntity(
    int entityID) const
{
    for (const Action &action : actions)
    {
        if (action.GetEntityID() == entityID)
        {
            return true;
        }
    }

    return false;
}

const Action *ActionManager::GetActionForEntity(
    int entityID) const
{
    for (const Action &action : actions)
    {
        if (action.GetEntityID() == entityID)
        {
            return &action;
        }
    }

    return nullptr;
}

std::vector<Action> ActionManager::Update()
{
    std::vector<Action> completedActions;

    currentTick++;

    for (auto actionIterator = actions.begin();
         actionIterator != actions.end();)
    {
        actionIterator->Update(currentTick);

        if (actionIterator->IsComplete())
        {
            completedActions.push_back(*actionIterator);
            actionIterator = actions.erase(actionIterator);
        }
        else if (actionIterator->IsCancelled())
        {
            actionIterator = actions.erase(actionIterator);
        }
        else
        {
            ++actionIterator;
        }
    }

    return completedActions;
}

void ActionManager::CancelActionsForEntity(
    int entityID,
    ActionCancelReason reason)
{
    auto actionIterator = actions.begin();

    while (actionIterator != actions.end())
    {
        if (actionIterator->GetEntityID() == entityID)
        {
            actionIterator->Cancel(reason);
            actionIterator = actions.erase(actionIterator);
        }
        else
        {
            ++actionIterator;
        }
    }
}

#include "ActionManager.h"

void ActionManager::AddAction(
    const Action &action)
{
    actions.push_back(action);
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

std::vector<Action> ActionManager::Update()
{
    std::vector<Action> completedActions;

    for (auto actionIterator = actions.begin();
         actionIterator != actions.end();)
    {
        actionIterator->Update();

        if (actionIterator->IsComplete())
        {
            completedActions.push_back(
                *actionIterator);

            actionIterator =
                actions.erase(actionIterator);
        }
        else
        {
            ++actionIterator;
        }
    }

    return completedActions;
}
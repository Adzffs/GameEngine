#include "ActionManager.h"

void ActionManager::AddAction(Action action)
{
    actions.push_back(action);
}

void ActionManager::Update()
{
    for (auto it = actions.begin(); it != actions.end();)
    {
        it->Update();

        if (it->IsComplete())
        {
            it = actions.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
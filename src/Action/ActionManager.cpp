#include "ActionManager.h"

namespace
{
    CancelledActionSnapshot BuildCancelledActionSnapshot(
        const Action &action,
        ActionCancelReason reason)
    {
        return CancelledActionSnapshot{
            action.GetOwnerID(),
            action.GetTargetID(),
            action.GetType(),
            reason};
    }

    ActionStartedSnapshot BuildStartedActionSnapshot(
        const Action &action)
    {
        return ActionStartedSnapshot{
            action.GetOwnerID(),
            action.GetTargetID(),
            action.GetType(),
            action.GetStartTick(),
            action.GetCompletionTick()};
    }
}

ActionStartTransition ActionManager::StartAction(
    const Action &action)
{
    ActionStartTransition transition;

    transition.cancelledAction = CancelActionsForEntity(
        action.GetOwnerID(),
        ActionCancelReason::NEW_ACTION_STARTED);

    actions.push_back(action);
    actions.back().Start(currentTick);

    if (actions.back().IsRunning())
    {
        transition.startedAction =
            BuildStartedActionSnapshot(actions.back());
    }

    return transition;
}

ActionStartTransition ActionManager::RestartAction(
    const Action &action)
{
    ActionStartTransition transition;

    if (!action.IsRepeating() ||
        !action.IsComplete())
    {
        return transition;
    }

    transition.cancelledAction = CancelActionsForEntity(
        action.GetOwnerID(),
        ActionCancelReason::NEW_ACTION_STARTED);

    Action restartedAction = action;
    restartedAction.Restart(currentTick);

    actions.push_back(restartedAction);

    if (actions.back().IsRunning())
    {
        transition.startedAction =
            BuildStartedActionSnapshot(actions.back());
    }

    return transition;
}

bool ActionManager::HasActionForEntity(
    int entityID) const
{
    for (const Action &action : actions)
    {
        if (action.GetOwnerID() == entityID)
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
        if (action.GetOwnerID() == entityID)
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

std::optional<CancelledActionSnapshot> ActionManager::CancelActionsForEntity(
    int entityID,
    ActionCancelReason reason)
{
    std::optional<CancelledActionSnapshot> cancelledAction;

    auto actionIterator = actions.begin();

    while (actionIterator != actions.end())
    {
        if (actionIterator->GetOwnerID() == entityID)
        {
            cancelledAction =
                BuildCancelledActionSnapshot(
                    *actionIterator,
                    reason);

            actionIterator->Cancel(reason);
            actionIterator = actions.erase(actionIterator);
        }
        else
        {
            ++actionIterator;
        }
    }

    return cancelledAction;
}

std::vector<CancelledActionSnapshot>
ActionManager::CancelMeleeActionsTargetingEntity(
    int targetEntityID,
    ActionCancelReason reason)
{
    std::vector<CancelledActionSnapshot> cancelledActions;

    auto actionIterator = actions.begin();

    while (actionIterator != actions.end())
    {
        if (actionIterator->GetType() == ActionType::MELEE_ATTACK &&
            actionIterator->GetTargetID() == targetEntityID)
        {
            cancelledActions.push_back(
                BuildCancelledActionSnapshot(
                    *actionIterator,
                    reason));

            actionIterator->Cancel(reason);
            actionIterator = actions.erase(actionIterator);
        }
        else
        {
            ++actionIterator;
        }
    }

    return cancelledActions;
}

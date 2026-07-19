#pragma once

#include "Action.h"

#include <optional>
#include <vector>

struct ActionStartedSnapshot
{
    int ownerEntityID;
    int targetID;
    ActionType actionType;
    int startTick;
    int completionTick;
};

struct CancelledActionSnapshot
{
    int ownerEntityID;
    int targetID;
    ActionType actionType;
    ActionCancelReason reason;
};

struct ActionStartTransition
{
    std::optional<CancelledActionSnapshot> cancelledAction;
    std::optional<ActionStartedSnapshot> startedAction;
};

class ActionManager
{
public:
    ActionStartTransition StartAction(
        const Action &action,
        int currentWorldTick);
    ActionStartTransition RestartAction(
        const Action &action,
        int currentWorldTick);

    bool HasActionForEntity(int entityID) const;

    const Action *GetActionForEntity(
        int entityID) const;

    std::optional<CancelledActionSnapshot> CancelActionsForEntity(
        int entityID,
        ActionCancelReason reason =
            ActionCancelReason::NONE);

    std::vector<CancelledActionSnapshot> CancelMeleeActionsTargetingEntity(
        int targetEntityID,
        ActionCancelReason reason =
            ActionCancelReason::ENTITY_DIED);

    std::vector<Action> Update(
        int currentWorldTick);

private:
    std::vector<Action> actions;
};
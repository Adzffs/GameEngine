#pragma once

#include "../../Action/ActionType.h"

struct ActionCompletedEvent
{
    const int ownerEntityID;
    const int targetID;
    const ActionType actionType;
    const int completionTick;

    ActionCompletedEvent(
        int ownerEntityID,
        int targetID,
        ActionType actionType,
        int completionTick)
        : ownerEntityID(ownerEntityID),
          targetID(targetID),
          actionType(actionType),
          completionTick(completionTick)
    {
    }
};

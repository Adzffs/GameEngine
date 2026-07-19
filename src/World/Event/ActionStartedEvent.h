#pragma once

#include "../../Action/ActionType.h"

struct ActionStartedEvent
{
    const int ownerEntityID;
    const int targetID;
    const ActionType actionType;
    const int startTick;
    const int completionTick;

    ActionStartedEvent(
        int ownerEntityID,
        int targetID,
        ActionType actionType,
        int startTick,
        int completionTick)
        : ownerEntityID(ownerEntityID),
          targetID(targetID),
          actionType(actionType),
          startTick(startTick),
          completionTick(completionTick)
    {
    }
};

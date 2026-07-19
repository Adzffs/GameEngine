#pragma once

#include "../../Action/ActionCancelReason.h"
#include "../../Action/ActionType.h"

struct ActionCancelledEvent
{
    const int ownerEntityID;
    const int targetID;
    const ActionType actionType;
    const ActionCancelReason reason;
    const int cancellationTick;

    ActionCancelledEvent(
        int ownerEntityID,
        int targetID,
        ActionType actionType,
        ActionCancelReason reason,
        int cancellationTick)
        : ownerEntityID(ownerEntityID),
          targetID(targetID),
          actionType(actionType),
          reason(reason),
          cancellationTick(cancellationTick)
    {
    }
};

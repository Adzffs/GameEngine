#pragma once

#include "ActionCancelledEvent.h"
#include "ActionCompletedEvent.h"
#include "ActionStartedEvent.h"

#include <variant>

using ActionLifecycleEventData = std::variant<
    ActionStartedEvent,
    ActionCompletedEvent,
    ActionCancelledEvent>;

struct ActionLifecycleEvent
{
    ActionLifecycleEventData data;
};

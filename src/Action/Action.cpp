#include "Action.h"

Action::Action(
    ActionType type,
    std::string name,
    int duration,
    int entityID,
    int targetID,
    bool repeats)
    : type(type),
      state(ActionState::PENDING),
      cancelReason(ActionCancelReason::NONE),
      name(name),
      duration(duration < 0 ? 0 : duration),
      currentTick(0),
      startTick(0),
      completionTick(0),
      ownerID(entityID),
      targetID(targetID),
      repeats(repeats)
{
}

void Action::Start(int currentWorldTick)
{
    startTick = currentWorldTick;
    completionTick = startTick + duration;
    currentTick = 0;

    cancelReason = ActionCancelReason::NONE;

    if (duration == 0)
    {
        state = ActionState::COMPLETED;
        return;
    }

    state = ActionState::RUNNING;
}

void Action::Update(int currentWorldTick)
{
    if (state != ActionState::RUNNING)
    {
        return;
    }

    if (currentWorldTick < startTick)
    {
        currentTick = 0;
        return;
    }

    currentTick = currentWorldTick - startTick;

    if (currentTick >= duration)
    {
        currentTick = duration;
        state = ActionState::COMPLETED;
    }
}

void Action::Restart(int currentWorldTick)
{
    Start(currentWorldTick);
}

void Action::Cancel(ActionCancelReason reason)
{
    if (state == ActionState::CANCELLED)
    {
        return;
    }

    state = ActionState::CANCELLED;
    cancelReason = reason;
}

bool Action::IsComplete() const
{
    return state == ActionState::COMPLETED;
}

bool Action::IsCancelled() const
{
    return state == ActionState::CANCELLED;
}

bool Action::IsRunning() const
{
    return state == ActionState::RUNNING;
}

bool Action::IsRepeating() const
{
    return repeats;
}

const std::string &Action::GetName() const
{
    return name;
}

int Action::GetDuration() const
{
    return duration;
}

int Action::GetCurrentTick() const
{
    return currentTick;
}

int Action::GetStartTick() const
{
    return startTick;
}

int Action::GetCompletionTick() const
{
    return completionTick;
}

float Action::GetProgress() const
{
    if (duration <= 0)
    {
        return 1.0f;
    }

    float progress =
        static_cast<float>(currentTick) /
        static_cast<float>(duration);

    if (progress < 0.0f)
    {
        return 0.0f;
    }

    if (progress > 1.0f)
    {
        return 1.0f;
    }

    return progress;
}

ActionType Action::GetType() const
{
    return type;
}

ActionState Action::GetState() const
{
    return state;
}

ActionCancelReason Action::GetCancelReason() const
{
    return cancelReason;
}

int Action::GetOwnerID() const
{
    return ownerID;
}

int Action::GetTargetID() const
{
    return targetID;
}
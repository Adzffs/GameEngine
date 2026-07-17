#include "Action.h"

Action::Action(
    ActionType type,
    std::string name,
    int duration,
    int entityID,
    int targetID)
    : type(type),
      name(name),
      duration(duration),
      currentTick(0),
      entityID(entityID),
      targetID(targetID)
{
}

void Action::Update()
{
    if (currentTick >= duration)
    {
        return;
    }

    currentTick++;
}

bool Action::IsComplete() const
{
    return currentTick >= duration;
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

int Action::GetEntityID() const
{
    return entityID;
}

int Action::GetTargetID() const
{
    return targetID;
}
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
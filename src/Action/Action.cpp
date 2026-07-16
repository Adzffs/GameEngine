#include "Action.h"

Action::Action(
    std::string name,
    int duration,
    int entityID,
    int targetID)
    : name(name),
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

int Action::GetEntityID() const
{
    return entityID;
}

int Action::GetTargetID() const
{
    return targetID;
}
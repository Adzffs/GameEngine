#include "Action.h"

#include <iostream>

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

    std::cout
        << "Entity "
        << entityID
        << " Action: "
        << name
        << " Target: "
        << targetID
        << " Tick "
        << currentTick
        << "/"
        << duration
        << std::endl;
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
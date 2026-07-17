#pragma once

#include <string>

#include "ActionType.h"

class Action
{
public:
    Action(
        ActionType type,
        std::string name,
        int duration,
        int entityID,
        int targetID);

    void Update();

    bool IsComplete() const;

    const std::string &GetName() const;

    int GetDuration() const;
    int GetCurrentTick() const;

    float GetProgress() const;

    ActionType GetType() const;

    int GetEntityID() const;
    int GetTargetID() const;

private:
    ActionType type;

    std::string name;

    int duration;
    int currentTick;

    int entityID;
    int targetID;
};
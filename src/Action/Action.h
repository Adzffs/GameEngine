#pragma once

#include <string>

class Action
{
public:
    Action(
        std::string name,
        int duration,
        int entityID,
        int targetID);

    void Update();

    bool IsComplete() const;

    int GetEntityID() const;
    int GetTargetID() const;

private:
    std::string name;

    int duration;
    int currentTick;

    int entityID;
    int targetID;
};
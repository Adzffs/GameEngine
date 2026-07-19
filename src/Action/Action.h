#pragma once

#include <string>

#include "ActionType.h"
#include "ActionState.h"
#include "ActionCancelReason.h"

class Action
{
public:
    Action(
        ActionType type,
        std::string name,
        int duration,
        int entityID,
        int targetID,
        bool repeats = false);

    void Start(int currentWorldTick);
    void Update(int currentWorldTick);
    void Restart(int currentWorldTick);

    void Cancel(ActionCancelReason reason);

    bool IsComplete() const;
    bool IsCancelled() const;
    bool IsRunning() const;
    bool IsRepeating() const;

    const std::string &GetName() const;

    int GetDuration() const;
    int GetCurrentTick() const;

    int GetStartTick() const;
    int GetCompletionTick() const;

    float GetProgress() const;

    ActionType GetType() const;
    ActionState GetState() const;
    ActionCancelReason GetCancelReason() const;

    int GetOwnerID() const;

    int GetTargetID() const;

private:
    ActionType type;
    ActionState state;
    ActionCancelReason cancelReason;

    std::string name;

    int duration;
    int currentTick;

    int startTick;
    int completionTick;

    int ownerID;
    int targetID;

    bool repeats;
};
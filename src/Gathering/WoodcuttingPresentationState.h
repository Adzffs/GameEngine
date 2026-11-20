#pragma once

#include "GatheringRewardEvent.h"
#include <optional>
#include <vector>

class Player;
class Action;
class ResourceNode;

struct WoodcuttingAnimation
{
    int resourceID;
    int facing;
    int axeTier;
    int actorX, actorY;
    float elapsedSeconds = 0.0f;
};

struct WoodcuttingRewardNotice
{
    GatheringRewardEvent event;
    float remainingSeconds = 1.4f;
};

// Presentation reads authoritative state and events; it never grants rewards.
class WoodcuttingPresentationState
{
public:
    static constexpr float CycleSeconds = 1.2f;
    static constexpr int PoseCount = 6;
    static int AxeTier(ItemType item);
    void SynchronizeRewards(int actorID, const std::vector<GatheringRewardEvent> &events);
    void Update(const Player &player, const Action *action,
        const std::vector<ResourceNode> &resources, float deltaSeconds, bool settled = true);
    const std::optional<WoodcuttingAnimation> &GetAnimation() const { return animation; }
    const std::vector<WoodcuttingRewardNotice> &GetRewards() const { return rewards; }
    int GetPose() const;
    float GetImpact() const;

private:
    int actorID = -1;
    int lastRewardTick = -1;
    std::optional<WoodcuttingAnimation> animation;
    std::vector<WoodcuttingRewardNotice> rewards;
};

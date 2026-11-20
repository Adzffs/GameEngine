#pragma once

#include "GatheringRewardEvent.h"
#include "../Skills/SkillType.h"
#include <optional>
#include <vector>

class Player;
class Action;
class ResourceNode;

struct GatheringAnimation
{
    int resourceID;
    int facing;
    int toolTier;
    int actorX, actorY;
    float elapsedSeconds = 0.0f;
};

struct GatheringRewardNotice
{
    GatheringRewardEvent event;
    float remainingSeconds = 1.4f;
};

// Shared read-only presentation for axe and pickaxe gathering. World actions
// and committed reward events remain the authority for progress and results.
class GatheringPresentationState
{
public:
    explicit GatheringPresentationState(SkillType skill) : skillType(skill) {}
    static constexpr float CycleSeconds = 1.2f;
    static constexpr int PoseCount = 6;
    static int ToolTier(ItemType item, SkillType skill);
    void SynchronizeRewards(int actorID, const std::vector<GatheringRewardEvent> &events);
    void Update(const Player &player, const Action *action,
        const std::vector<ResourceNode> &resources, float deltaSeconds, bool settled = true);
    SkillType GetSkillType() const { return skillType; }
    const std::optional<GatheringAnimation> &GetAnimation() const { return animation; }
    const std::vector<GatheringRewardNotice> &GetRewards() const { return rewards; }
    int GetPose() const;
    float GetImpact() const;
    bool ConsumeContact() { const bool result = contactPending; contactPending = false; return result; }

private:
    SkillType skillType;
    bool contactPending = false;
    int actorID = -1;
    int lastRewardTick = -1;
    std::optional<GatheringAnimation> animation;
    std::vector<GatheringRewardNotice> rewards;
};

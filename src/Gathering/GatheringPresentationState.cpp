#include "GatheringPresentationState.h"
#include "../Action/Action.h"
#include "../Player/Player.h"
#include "../Item/ItemDatabase.h"
#include "../World/Object/Resource/ResourceNode.h"
#include "../World/Object/Resource/ResourceDatabase.h"
#include <algorithm>
#include <cmath>

int GatheringPresentationState::ToolTier(ItemType item, SkillType skill)
{
    if (skill == SkillType::MINING)
    {
        switch (item)
        {
        case ItemType::BRONZE_PICKAXE: return 0;
        case ItemType::IRON_PICKAXE: return 1;
        case ItemType::STEEL_PICKAXE: return 2;
        default: return -1;
        }
    }
    if (skill != SkillType::WOODCUTTING) return -1;
    switch (item)
    {
    case ItemType::BRONZE_AXE: return 0;
    case ItemType::IRON_AXE: return 1;
    case ItemType::STEEL_AXE: return 2;
    default: return -1;
    }
}

void GatheringPresentationState::SynchronizeRewards(int localActorID,
    const std::vector<GatheringRewardEvent> &events)
{
    if (actorID != localActorID)
    {
        actorID = localActorID;
        lastRewardTick = -1;
        rewards.clear();
        animation.reset();
    }
    for (const auto &event : events)
    {
        if (event.actorEntityID != actorID || event.completionTick <= lastRewardTick) continue;
        lastRewardTick = event.completionTick;
        if (ResourceDatabase::Get(event.resourceType).GetRequiredSkill() != skillType) continue;
        if (event.amount <= 0 || event.xp <= 0) continue;
        rewards.push_back({event});
        if (rewards.size() > 4) rewards.erase(rewards.begin());
    }
}

void GatheringPresentationState::Update(const Player &player, const Action *action,
    const std::vector<ResourceNode> &resources, float deltaSeconds, bool settled)
{
    const float delta = std::isfinite(deltaSeconds) ? std::clamp(deltaSeconds, 0.0f, 0.1f) : 0.0f;
    for (auto &notice : rewards) notice.remainingSeconds -= delta;
    std::erase_if(rewards, [](const auto &notice) { return notice.remainingSeconds <= 0.0f; });

    const auto stop = [&] { animation.reset(); };
    if (!settled || !player.IsAlive() || !action || !action->IsRunning() ||
        action->GetOwnerID() != player.GetID() || action->GetType() != ActionType::GATHERING)
    {
        stop();
        return;
    }
    const auto resource = std::find_if(resources.begin(), resources.end(),
        [&](const auto &node) { return node.GetID() == action->GetTargetID(); });
    const auto tool = player.GetEquipment().GetEquippedItem(EquipmentSlotType::WEAPON);
    const int tier = ToolTier(tool, skillType);
    if (resource == resources.end() || !resource->IsActive() || tier < 0)
    {
        stop();
        return;
    }
    const auto &definition = ResourceDatabase::Get(resource->GetResourceType());
    const int level = player.GetSkills().GetSkill(skillType).GetLevel();
    const int x = player.GetPosition().GetX(), y = player.GetPosition().GetY();
    const int dx = resource->GetX() - x, dy = resource->GetY() - y;
    if (definition.GetRequiredSkill() != skillType ||
        level < definition.GetRequiredSkillLevel() || level < ItemDatabase::Get(tool).GetRequiredSkillLevel() ||
        std::abs(dx) > 1 || std::abs(dy) > 1 || (dx == 0 && dy == 0))
    {
        stop();
        return;
    }
    const int facing = dx == 0 ? (dy > 0 ? 0 : 3) : dy == 0 ? (dx < 0 ? 1 : 2) :
        dy > 0 ? (dx < 0 ? 4 : 5) : (dx < 0 ? 6 : 7);
    if (!animation || animation->resourceID != resource->GetID() || animation->toolTier != tier || animation->facing != facing ||
        animation->actorX != x || animation->actorY != y)
        animation = GatheringAnimation{resource->GetID(), facing, tier, x, y};
    else animation->elapsedSeconds = std::fmod(animation->elapsedSeconds + delta, CycleSeconds);
}

int GatheringPresentationState::GetPose() const
{
    return animation ? std::min(PoseCount - 1, static_cast<int>(animation->elapsedSeconds / CycleSeconds * PoseCount)) : 0;
}

float GatheringPresentationState::GetImpact() const
{
    if (!animation || GetPose() != 3) return 0.0f;
    return std::clamp((animation->elapsedSeconds / CycleSeconds * PoseCount - 3.0f), 0.0f, 1.0f);
}

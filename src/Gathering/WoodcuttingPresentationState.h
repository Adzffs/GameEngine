#pragma once

#include "GatheringPresentationState.h"

using WoodcuttingAnimation = GatheringAnimation;
using WoodcuttingRewardNotice = GatheringRewardNotice;

class WoodcuttingPresentationState : public GatheringPresentationState
{
public:
    WoodcuttingPresentationState() : GatheringPresentationState(SkillType::WOODCUTTING) {}
    static int AxeTier(ItemType item);
};

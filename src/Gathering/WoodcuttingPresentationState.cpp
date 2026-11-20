#include "WoodcuttingPresentationState.h"

int WoodcuttingPresentationState::AxeTier(ItemType item)
{
    return ToolTier(item, SkillType::WOODCUTTING);
}

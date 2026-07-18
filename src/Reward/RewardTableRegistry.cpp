#include "RewardTableRegistry.h"

#include "DevelopmentRewardTables.h"

namespace RewardTableRegistry
{
    const RewardTable *TryGetRewardTable(
        RewardTableType rewardTableType)
    {
        switch (rewardTableType)
        {
        case RewardTableType::DEVELOPMENT_MONSTER:
            return &DevelopmentRewardTables::GetDevelopmentMonsterRewardTable();

        case RewardTableType::NONE:
        default:
            return nullptr;
        }
    }
}

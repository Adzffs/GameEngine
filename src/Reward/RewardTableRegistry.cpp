#include "RewardTableRegistry.h"

#include "DevelopmentRewardTables.h"

namespace RewardTableRegistry
{
    const std::vector<RewardTableType> &
    GetAllRewardTableTypes()
    {
        static const std::vector<RewardTableType> rewardTableTypes{
            RewardTableType::DEVELOPMENT_MONSTER};

        return rewardTableTypes;
    }

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

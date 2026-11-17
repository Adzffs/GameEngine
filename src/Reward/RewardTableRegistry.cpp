#include "RewardTableRegistry.h"

#include "DevelopmentRewardTables.h"

namespace RewardTableRegistry
{
    const std::vector<RewardTableType> &
    GetAllRewardTableTypes()
    {
        static const std::vector<RewardTableType> rewardTableTypes{
            RewardTableType::DEVELOPMENT_MONSTER,
            RewardTableType::AGGRESSIVE_MONSTER,
            RewardTableType::DURABLE_MONSTER,
            RewardTableType::ELITE_MONSTER};

        return rewardTableTypes;
    }

    const RewardTable *TryGetRewardTable(
        RewardTableType rewardTableType)
    {
        switch (rewardTableType)
        {
        case RewardTableType::DEVELOPMENT_MONSTER:
            return &DevelopmentRewardTables::GetDevelopmentMonsterRewardTable();
        case RewardTableType::AGGRESSIVE_MONSTER:
            return &DevelopmentRewardTables::GetAggressiveMonsterRewardTable();
        case RewardTableType::DURABLE_MONSTER:
            return &DevelopmentRewardTables::GetDurableMonsterRewardTable();
        case RewardTableType::ELITE_MONSTER:
            return &DevelopmentRewardTables::GetEliteMonsterRewardTable();

        case RewardTableType::NONE:
        default:
            return nullptr;
        }
    }
}

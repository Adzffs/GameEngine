#include "DevelopmentRewardTables.h"

#include <stdexcept>

namespace DevelopmentRewardTables
{
    const RewardTable &
    GetDevelopmentMonsterRewardTable()
    {
        static const RewardTable table = []
        {
            RewardTable rewardTable;

            RewardTableValidationResult guaranteedResult =
                rewardTable.AddGuaranteed(
                    ItemType::COAL,
                    1,
                    1);

            if (!guaranteedResult.valid)
            {
                throw std::logic_error(
                    guaranteedResult.message);
            }

            RewardTableValidationResult copperResult =
                rewardTable.AddWeighted(
                    ItemType::COPPER_ORE,
                    1,
                    1,
                    60);

            if (!copperResult.valid)
            {
                throw std::logic_error(
                    copperResult.message);
            }

            RewardTableValidationResult tinResult =
                rewardTable.AddWeighted(
                    ItemType::TIN_ORE,
                    1,
                    1,
                    30);

            if (!tinResult.valid)
            {
                throw std::logic_error(
                    tinResult.message);
            }

            RewardTableValidationResult ironResult =
                rewardTable.AddWeighted(
                    ItemType::IRON_ORE,
                    1,
                    1,
                    10);

            if (!ironResult.valid)
            {
                throw std::logic_error(
                    ironResult.message);
            }

            return rewardTable;
        }();

        return table;
    }
}

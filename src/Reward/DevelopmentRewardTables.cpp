#include "DevelopmentRewardTables.h"

#include <stdexcept>

namespace DevelopmentRewardTables
{
    namespace
    {
        RewardTable MakeTable(
            ItemType guaranteedType, int guaranteedMinimum,
            int guaranteedMaximum, ItemType commonType,
            int commonMinimum, int commonMaximum, ItemType rareType,
            int rareMinimum, int rareMaximum)
        {
            RewardTable table;
            if (!table.AddGuaranteed(guaranteedType, guaranteedMinimum,
                    guaranteedMaximum).valid ||
                !table.AddWeighted(commonType, commonMinimum, commonMaximum,
                    80).valid ||
                !table.AddWeighted(rareType, rareMinimum, rareMaximum,
                    20).valid)
                throw std::logic_error("Invalid authored monster reward table");
            return table;
        }
    }

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

    const RewardTable &GetAggressiveMonsterRewardTable()
    {
        static const RewardTable table = MakeTable(
            ItemType::COOKED_MEAT, 1, 1,
            ItemType::COINS, 3, 7,
            ItemType::IRON_ORE, 1, 2);
        return table;
    }

    const RewardTable &GetDurableMonsterRewardTable()
    {
        static const RewardTable table = MakeTable(
            ItemType::IRON_ORE, 2, 3,
            ItemType::COAL, 2, 4,
            ItemType::STEEL_BAR, 1, 1);
        return table;
    }

    const RewardTable &GetEliteMonsterRewardTable()
    {
        static const RewardTable table = MakeTable(
            ItemType::STEEL_BAR, 2, 3,
            ItemType::COINS, 20, 35,
            ItemType::STEEL_AXE, 1, 1);
        return table;
    }
}

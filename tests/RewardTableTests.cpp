#include "TestSupport.h"

#include "../src/Core/RandomSource.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Reward/DevelopmentRewardTables.h"
#include "../src/Reward/ItemReward.h"
#include "../src/Reward/RewardTable.h"
#include "../src/Reward/RewardTableRoller.h"

#include <climits>
#include <utility>
#include <vector>

namespace
{
    class SequenceRandomSource : public RandomSource
    {
    public:
        explicit SequenceRandomSource(std::vector<int> values)
            : values(std::move(values))
        {
        }

        int NextIntInclusive(
            int minimum,
            int maximum) override
        {
            callCount++;

            if (minimum > maximum)
            {
                int temporary = minimum;
                minimum = maximum;
                maximum = temporary;
            }

            if (minimum == maximum)
            {
                return minimum;
            }

            if (nextIndex >= static_cast<int>(values.size()))
            {
                return minimum;
            }

            int result = values[nextIndex];
            nextIndex++;
            return result;
        }

        int GetCallCount() const
        {
            return callCount;
        }

    private:
        std::vector<int> values;
        int nextIndex = 0;
        int callCount = 0;
    };

    int FindQuantity(
        const std::vector<ItemReward> &rewards,
        ItemType itemType)
    {
        for (const ItemReward &reward : rewards)
        {
            if (reward.itemType == itemType)
            {
                return reward.quantity;
            }
        }

        return 0;
    }

    bool SameRewards(
        const std::vector<ItemReward> &left,
        const std::vector<ItemReward> &right)
    {
        if (left.size() != right.size())
        {
            return false;
        }

        for (int index = 0; index < static_cast<int>(left.size()); ++index)
        {
            if (left[index].itemType != right[index].itemType ||
                left[index].quantity != right[index].quantity)
            {
                return false;
            }
        }

        return true;
    }
}

int main()
{
    TestContext test;

    {
        RewardTable table;
        SequenceRandomSource random({});
        RewardTableRoller roller(random);

        std::vector<ItemReward> rewards =
            roller.Roll(table);

        test.Expect(
            rewards.empty(),
            "Empty table is valid and rolls to no rewards");
        test.Expect(
            table.GetGuaranteedEntries().empty(),
            "Empty table has no guaranteed entries");
        test.Expect(
            table.GetWeightedEntries().empty(),
            "Empty table has no weighted entries");
    }

    {
        RewardTable table;

        RewardTableValidationResult result =
            table.AddGuaranteed(
                ItemType::LOG,
                0,
                1);

        test.Expect(
            !result.valid,
            "Invalid minimum quantity is rejected");
        test.Expect(
            table.GetGuaranteedEntries().empty(),
            "Rejected guaranteed entry is not added");
    }

    {
        RewardTable table;

        RewardTableValidationResult result =
            table.AddGuaranteed(
                ItemType::LOG,
                3,
                2);

        test.Expect(
            !result.valid,
            "Maximum below minimum is rejected");
    }

    {
        RewardTable table;

        RewardTableValidationResult zeroWeight =
            table.AddWeighted(
                ItemType::COPPER_ORE,
                1,
                1,
                0);

        RewardTableValidationResult negativeWeight =
            table.AddWeighted(
                ItemType::COPPER_ORE,
                1,
                1,
                -2);

        test.Expect(
            !zeroWeight.valid,
            "Zero weight is rejected");
        test.Expect(
            !negativeWeight.valid,
            "Negative weight is rejected");
        test.Expect(
            table.GetWeightedEntries().empty(),
            "Rejected weighted entries are not added");
    }

    {
        RewardTable table;

        RewardTableValidationResult result =
            table.AddGuaranteed(
                static_cast<ItemType>(9999),
                1,
                1);

        test.Expect(
            !result.valid,
            "Invalid item identifier is rejected");
    }

    {
        RewardTable table;

        RewardTableValidationResult first =
            table.AddWeighted(
                ItemType::COPPER_ORE,
                1,
                1,
                INT_MAX);

        RewardTableValidationResult second =
            table.AddWeighted(
                ItemType::TIN_ORE,
                1,
                1,
                1);

        test.Expect(
            first.valid,
            "Initial large weight entry is accepted");
        test.Expect(
            !second.valid,
            "Total-weight overflow is rejected safely");
    }

    {
        RewardTable table;
        table.AddGuaranteed(
            ItemType::LOG,
            2,
            2);
        table.AddGuaranteed(
            ItemType::COPPER_ORE,
            1,
            1);

        SequenceRandomSource random({});
        RewardTableRoller roller(random);

        std::vector<ItemReward> rewards =
            roller.Roll(table);

        test.ExpectEqual(
            static_cast<int>(rewards.size()),
            2,
            "Every guaranteed entry is returned");
        test.ExpectEqual(
            rewards[0].quantity,
            2,
            "Fixed guaranteed quantity returns exact value");
        test.ExpectEqual(
            static_cast<int>(rewards[0].itemType),
            static_cast<int>(ItemType::LOG),
            "Guaranteed rewards preserve deterministic definition order (first)");
        test.ExpectEqual(
            static_cast<int>(rewards[1].itemType),
            static_cast<int>(ItemType::COPPER_ORE),
            "Guaranteed rewards preserve deterministic definition order (second)");
    }

    {
        RewardTable table;
        table.AddGuaranteed(
            ItemType::TIN_ORE,
            2,
            4);

        SequenceRandomSource random({2, 4});
        RewardTableRoller roller(random);

        std::vector<ItemReward> firstRoll =
            roller.Roll(table);
        std::vector<ItemReward> secondRoll =
            roller.Roll(table);

        test.ExpectEqual(
            firstRoll[0].quantity,
            2,
            "Guaranteed ranged quantity includes lower bound");
        test.ExpectEqual(
            secondRoll[0].quantity,
            4,
            "Guaranteed ranged quantity includes upper bound");
    }

    {
        RewardTable table;
        table.AddGuaranteed(
            ItemType::COAL,
            1,
            1);

        SequenceRandomSource random({});
        RewardTableRoller roller(random);

        std::vector<ItemReward> rewards =
            roller.Roll(table);

        test.ExpectEqual(
            static_cast<int>(rewards.size()),
            1,
            "Empty weighted group produces no weighted reward");
        test.ExpectEqual(
            static_cast<int>(rewards[0].itemType),
            static_cast<int>(ItemType::COAL),
            "Only guaranteed reward is produced when weighted group is empty");
    }

    {
        RewardTable table;
        table.AddWeighted(
            ItemType::COPPER_ORE,
            1,
            1,
            2);
        table.AddWeighted(
            ItemType::TIN_ORE,
            1,
            1,
            3);

        SequenceRandomSource random({1});
        RewardTableRoller roller(random);

        std::vector<ItemReward> rewards =
            roller.Roll(table);

        test.ExpectEqual(
            static_cast<int>(rewards.size()),
            1,
            "Exactly one weighted entry is selected");
    }

    {
        RewardTable table;
        table.AddWeighted(
            ItemType::COPPER_ORE,
            1,
            1,
            5);
        table.AddWeighted(
            ItemType::TIN_ORE,
            1,
            1,
            3);
        table.AddWeighted(
            ItemType::IRON_ORE,
            1,
            1,
            2);

        {
            SequenceRandomSource random({1});
            RewardTableRoller roller(random);
            std::vector<ItemReward> rewards =
                roller.Roll(table);

            test.ExpectEqual(
                static_cast<int>(rewards[0].itemType),
                static_cast<int>(ItemType::COPPER_ORE),
                "First weight boundary selects first entry");
        }

        {
            SequenceRandomSource random({5});
            RewardTableRoller roller(random);
            std::vector<ItemReward> rewards =
                roller.Roll(table);

            test.ExpectEqual(
                static_cast<int>(rewards[0].itemType),
                static_cast<int>(ItemType::COPPER_ORE),
                "Last value in first range still selects first entry");
        }

        {
            SequenceRandomSource random({6});
            RewardTableRoller roller(random);
            std::vector<ItemReward> rewards =
                roller.Roll(table);

            test.ExpectEqual(
                static_cast<int>(rewards[0].itemType),
                static_cast<int>(ItemType::TIN_ORE),
                "First value in second range selects second entry");
        }

        {
            SequenceRandomSource random({10});
            RewardTableRoller roller(random);
            std::vector<ItemReward> rewards =
                roller.Roll(table);

            test.ExpectEqual(
                static_cast<int>(rewards[0].itemType),
                static_cast<int>(ItemType::IRON_ORE),
                "Final total-weight value selects final entry");
        }
    }

    {
        RewardTable table;
        table.AddWeighted(
            ItemType::COPPER_ORE,
            1,
            1,
            5);
        table.AddWeighted(
            ItemType::TIN_ORE,
            3,
            5,
            5);

        SequenceRandomSource random({6, 4});
        RewardTableRoller roller(random);

        std::vector<ItemReward> rewards =
            roller.Roll(table);

        test.ExpectEqual(
            static_cast<int>(rewards[0].itemType),
            static_cast<int>(ItemType::TIN_ORE),
            "Weighted selection picks second entry before quantity roll");
        test.ExpectEqual(
            rewards[0].quantity,
            4,
            "Weighted quantity roll occurs after entry selection");
        test.ExpectEqual(
            random.GetCallCount(),
            2,
            "Weighted roll performs one selection and one quantity roll");
    }

    {
        RewardTable table;
        table.AddGuaranteed(
            ItemType::COAL,
            1,
            3);
        table.AddWeighted(
            ItemType::COPPER_ORE,
            1,
            1,
            50);
        table.AddWeighted(
            ItemType::TIN_ORE,
            1,
            1,
            50);

        SequenceRandomSource randomA({2, 1});
        SequenceRandomSource randomB({2, 1});

        RewardTableRoller rollerA(randomA);
        RewardTableRoller rollerB(randomB);

        std::vector<ItemReward> rewardsA =
            rollerA.Roll(table);
        std::vector<ItemReward> rewardsB =
            rollerB.Roll(table);

        test.Expect(
            SameRewards(rewardsA, rewardsB),
            "Identical deterministic sequences produce identical rewards");
    }

    {
        RewardTable table;
        table.AddWeighted(
            ItemType::COPPER_ORE,
            1,
            1,
            50);
        table.AddWeighted(
            ItemType::TIN_ORE,
            1,
            1,
            50);

        SequenceRandomSource randomA({1});
        SequenceRandomSource randomB({100});

        RewardTableRoller rollerA(randomA);
        RewardTableRoller rollerB(randomB);

        std::vector<ItemReward> rewardsA =
            rollerA.Roll(table);
        std::vector<ItemReward> rewardsB =
            rollerB.Roll(table);

        test.Expect(
            rewardsA[0].itemType != rewardsB[0].itemType,
            "Different deterministic sequences can select different weighted entries");
    }

    {
        RewardTable table;
        table.AddGuaranteed(
            ItemType::LOG,
            1,
            1);
        table.AddWeighted(
            ItemType::TIN_ORE,
            2,
            2,
            1);

        const int guaranteedCountBefore =
            static_cast<int>(table.GetGuaranteedEntries().size());
        const int weightedCountBefore =
            static_cast<int>(table.GetWeightedEntries().size());
        const int totalWeightBefore =
            table.GetTotalWeight();

        SequenceRandomSource random({1});
        RewardTableRoller roller(random);

        std::vector<ItemReward> rewards =
            roller.Roll(table);

        for (const ItemReward &reward : rewards)
        {
            test.Expect(
                reward.quantity > 0,
                "Every returned reward quantity is positive");
        }

        test.ExpectEqual(
            static_cast<int>(table.GetGuaranteedEntries().size()),
            guaranteedCountBefore,
            "Rolling does not mutate guaranteed entries");
        test.ExpectEqual(
            static_cast<int>(table.GetWeightedEntries().size()),
            weightedCountBefore,
            "Rolling does not mutate weighted entries");
        test.ExpectEqual(
            table.GetTotalWeight(),
            totalWeightBefore,
            "Rolling does not mutate total weighted value");
    }

    {
        RewardTable table;
        table.AddGuaranteed(
            ItemType::LOG,
            1,
            1);
        table.AddWeighted(
            ItemType::LOG,
            2,
            2,
            1);

        SequenceRandomSource random({1});
        RewardTableRoller roller(random);

        std::vector<ItemReward> rewards =
            roller.Roll(table);

        test.ExpectEqual(
            static_cast<int>(rewards.size()),
            1,
            "Duplicate rewards are merged into one ItemReward entry");
        test.ExpectEqual(
            FindQuantity(
                rewards,
                ItemType::LOG),
            3,
            "Merged duplicate reward quantity is additive");
    }

    {
        RewardTable table;
        table.AddGuaranteed(
            ItemType::LOG,
            INT_MAX,
            INT_MAX);
        table.AddGuaranteed(
            ItemType::LOG,
            1,
            1);

        SequenceRandomSource random({});
        RewardTableRoller roller(random);

        std::vector<ItemReward> rewards =
            roller.Roll(table);

        test.Expect(
            rewards.empty(),
            "Duplicate-reward quantity overflow is detected and roll fails safely");

        bool foundWrappedOrNegative = false;

        for (const ItemReward &reward : rewards)
        {
            if (reward.quantity <= 0)
            {
                foundWrappedOrNegative = true;
                break;
            }
        }

        test.Expect(
            !foundWrappedOrNegative,
            "Failed overflow roll returns no wrapped or negative quantities");
    }

    {
        RewardTable table;
        table.AddGuaranteed(
            ItemType::LOG,
            INT_MAX - 1,
            INT_MAX - 1);
        table.AddGuaranteed(
            ItemType::LOG,
            1,
            1);

        SequenceRandomSource random({});
        RewardTableRoller roller(random);

        std::vector<ItemReward> rewards =
            roller.Roll(table);

        test.ExpectEqual(
            static_cast<int>(rewards.size()),
            1,
            "Boundary duplicate merge returns a single reward entry");
        test.ExpectEqual(
            FindQuantity(
                rewards,
                ItemType::LOG),
            INT_MAX,
            "Boundary duplicate merge succeeds at exactly INT_MAX");
    }

    {
        const RewardTable &table =
            DevelopmentRewardTables::GetDevelopmentMonsterRewardTable();

        const auto &guaranteed =
            table.GetGuaranteedEntries();
        const auto &weighted =
            table.GetWeightedEntries();

        test.ExpectEqual(
            static_cast<int>(guaranteed.size()),
            1,
            "Development monster table has one guaranteed entry");
        test.ExpectEqual(
            static_cast<int>(guaranteed[0].itemType),
            static_cast<int>(ItemType::COAL),
            "Development monster table guaranteed item is Coal");
        test.ExpectEqual(
            guaranteed[0].minimumQuantity,
            1,
            "Development monster guaranteed minimum quantity is one");
        test.ExpectEqual(
            guaranteed[0].maximumQuantity,
            1,
            "Development monster guaranteed maximum quantity is one");

        test.ExpectEqual(
            static_cast<int>(weighted.size()),
            3,
            "Development monster table has three weighted entries");

        test.ExpectEqual(
            static_cast<int>(weighted[0].itemType),
            static_cast<int>(ItemType::COPPER_ORE),
            "Development monster weighted entry one is Copper ore");
        test.ExpectEqual(
            weighted[0].weight,
            60,
            "Development monster Copper ore weight is 60");

        test.ExpectEqual(
            static_cast<int>(weighted[1].itemType),
            static_cast<int>(ItemType::TIN_ORE),
            "Development monster weighted entry two is Tin ore");
        test.ExpectEqual(
            weighted[1].weight,
            30,
            "Development monster Tin ore weight is 30");

        test.ExpectEqual(
            static_cast<int>(weighted[2].itemType),
            static_cast<int>(ItemType::IRON_ORE),
            "Development monster weighted entry three is Iron ore");
        test.ExpectEqual(
            weighted[2].weight,
            10,
            "Development monster Iron ore weight is 10");
    }

    return test.Finish();
}

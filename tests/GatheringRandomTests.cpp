#include "TestSupport.h"

#include "../src/Action/ActionCancelReason.h"
#include "../src/Core/RandomSource.h"
#include "../src/Core/SeededRandom.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Equipment/EquipmentSlotType.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Item/ItemDatabase.h"
#include "../src/Player/Player.h"
#include "../src/Reward/RewardTableType.h"
#include "../src/Skills/SkillType.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/World/Object/Resource/ResourceDatabase.h"
#include "../src/World/World.h"

#include <memory>
#include <optional>
#include <stdexcept>
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
                throw std::runtime_error(
                    "SequenceRandomSource exhausted");
            }

            lastMinimum = minimum;
            lastMaximum = maximum;
            calls++;

            int value = values[nextIndex++];

            if (value < minimum)
            {
                return minimum;
            }

            if (value > maximum)
            {
                return maximum;
            }

            return value;
        }

        int GetCallCount() const
        {
            return calls;
        }

        int GetLastMinimum() const
        {
            return lastMinimum;
        }

        int GetLastMaximum() const
        {
            return lastMaximum;
        }

    private:
        std::vector<int> values;
        int nextIndex = 0;
        int calls = 0;
        int lastMinimum = 0;
        int lastMaximum = 0;
    };

    Player *GetPlayer(
        World &world,
        int entityID)
    {
        return dynamic_cast<Player *>(
            world.GetEntityByID(entityID));
    }

    Monster *GetMonster(
        World &world,
        int entityID)
    {
        return dynamic_cast<Monster *>(
            world.GetEntityByID(entityID));
    }

    int FindInventorySlot(
        const Inventory &inventory,
        ItemType itemType)
    {
        const auto &slots = inventory.GetSlots();

        for (int index = 0;
             index < static_cast<int>(slots.size());
             ++index)
        {
            if (!slots[index].IsEmpty() &&
                slots[index].GetItemType() == itemType)
            {
                return index;
            }
        }

        return -1;
    }

    bool EquipItem(
        World &world,
        int playerID,
        ItemType itemType)
    {
        Player *player =
            GetPlayer(world, playerID);

        if (player == nullptr)
        {
            return false;
        }

        int slotIndex =
            FindInventorySlot(
                player->GetInventory(),
                itemType);

        if (slotIndex < 0)
        {
            return false;
        }

        return world.TryEquipInventoryItem(
            playerID,
            slotIndex);
    }

    struct ResourceProbe
    {
        int x = 0;
        int y = 0;
        ResourceType type = ResourceType::NORMAL_TREE;
    };

    std::optional<ResourceProbe> FindResource(
        World &world,
        ResourceType resourceType)
    {
        for (const ResourceNode &resource :
             world.GetResources())
        {
            if (resource.GetResourceType() == resourceType)
            {
                return ResourceProbe{
                    resource.GetX(),
                    resource.GetY(),
                    resource.GetResourceType()};
            }
        }

        return std::nullopt;
    }

    std::optional<ResourceProbe> FindResourceForSkill(
        World &world,
        SkillType skill)
    {
        for (const ResourceNode &resource :
             world.GetResources())
        {
            const ResourceDefinition &definition =
                ResourceDatabase::Get(
                    resource.GetResourceType());

            if (definition.GetRequiredSkill() == skill)
            {
                return ResourceProbe{
                    resource.GetX(),
                    resource.GetY(),
                    resource.GetResourceType()};
            }
        }

        return std::nullopt;
    }

    void AdvanceTicks(
        World &world,
        int ticks)
    {
        for (int index = 0; index < ticks; ++index)
        {
            world.Update();
        }
    }

    int GetGatheringDurationTicks(
        ItemType tool)
    {
        return ItemDatabase::Get(tool)
            .GetActionDurationTicks();
    }

    bool StartGatheringAt(
        World &world,
        int playerID,
        int resourceX,
        int resourceY,
        ItemType tool)
    {
        if (!EquipItem(
                world,
                playerID,
                tool))
        {
            return false;
        }

        Player *player =
            GetPlayer(world, playerID);

        if (player == nullptr)
        {
            return false;
        }

        player->GetPosition().SetPosition(
            resourceX - 1,
            resourceY);

        ResourceNode *resource =
            world.GetResourceAt(
                resourceX,
                resourceY);

        if (resource == nullptr)
        {
            return false;
        }

        world.QueueResourceInteraction(
            playerID,
            resource->GetID());

        world.Update();

        const Action *action =
            world.GetActionForEntity(playerID);

        return action != nullptr;
    }

    void ResolveSingleGatherCompletion(
        World &world,
        ItemType tool)
    {
        AdvanceTicks(
            world,
            GetGatheringDurationTicks(tool));
    }

    struct RewardSnapshot
    {
        int coal = 0;
        int copper = 0;
        int tin = 0;
        int iron = 0;
    };

    RewardSnapshot GetRewardSnapshot(
        const Inventory &inventory)
    {
        return RewardSnapshot{
            inventory.GetItemAmount(ItemType::COAL),
            inventory.GetItemAmount(ItemType::COPPER_ORE),
            inventory.GetItemAmount(ItemType::TIN_ORE),
            inventory.GetItemAmount(ItemType::IRON_ORE)};
    }

    bool ConsumeCombatRollsWithoutRewards(
        World &world,
        int attackerID,
        int defenderID)
    {
        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        if (attacker == nullptr || defender == nullptr)
        {
            return false;
        }

        attacker->GetPosition().SetPosition(20, 20);
        defender->GetPosition().SetPosition(21, 20);

        if (!world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                1))
        {
            return false;
        }

        world.Update();
        return world.GetLastMeleeAttackResult().has_value();
    }

    CombatRatings MakeRatings(
        int attackAccuracy,
        int meleeStrength,
        int defence,
        int maximumHealth)
    {
        CombatRatings ratings;
        ratings.attackAccuracy = attackAccuracy;
        ratings.meleeStrength = meleeStrength;
        ratings.defence = defence;
        ratings.maximumHealth = maximumHealth;
        return ratings;
    }

    bool KillMonsterWithPlayer(
        World &world,
        int playerID,
        int monsterID)
    {
        Player *player = GetPlayer(world, playerID);
        Monster *monster = GetMonster(world, monsterID);

        if (player == nullptr ||
            monster == nullptr)
        {
            return false;
        }

        player->GetSkills().AddXP(
            SkillType::ATTACK,
            100000);

        player->GetPosition().SetPosition(
            monster->GetPosition().GetX() - 1,
            monster->GetPosition().GetY());

        if (!world.TryStartMeleeAttack(
                playerID,
                monsterID,
                1))
        {
            return false;
        }

        world.Update();

        return !monster->IsAlive();
    }
}

int main()
{
    TestContext test;

    {
        SequenceRandomSource random({});

        test.Expect(
            !random.RollPercentage(0),
            "Zero-percent roll always fails");
        test.Expect(
            !random.RollPercentage(-20),
            "Negative percentage fails safely");
        test.Expect(
            random.RollPercentage(100),
            "One-hundred-percent roll always succeeds");
        test.Expect(
            random.RollPercentage(250),
            "Percentages above one hundred succeed safely");
        test.ExpectEqual(
            random.GetCallCount(),
            0,
            "Boundary-safe percentage paths do not consume RNG draws");
    }

    {
        SequenceRandomSource random({1, 37, 38, 100});

        test.Expect(
            random.RollPercentage(1),
            "A roll of one succeeds at one percent");
        test.Expect(
            random.RollPercentage(37),
            "A roll equal to the percentage succeeds");
        test.Expect(
            !random.RollPercentage(37),
            "A roll one above the percentage fails");
        test.Expect(
            !random.RollPercentage(99),
            "A roll of one hundred fails at ninety-nine percent");
        test.ExpectEqual(
            random.GetCallCount(),
            4,
            "Each non-boundary percentage consumes exactly one draw");
        test.ExpectEqual(
            random.GetLastMinimum(),
            1,
            "Percentage rolls use an inclusive lower bound of one");
        test.ExpectEqual(
            random.GetLastMaximum(),
            100,
            "Percentage rolls use an inclusive upper bound of one hundred");
    }

    {
        SeededRandom left(1337U);
        SeededRandom right(1337U);

        for (int index = 0; index < 64; ++index)
        {
            test.ExpectEqual(
                left.RollPercentage(45),
                right.RollPercentage(45),
                "Fixed seed produces identical gathering roll sequence");
        }
    }

    {
        SeededRandom left(1337U);
        SeededRandom right(7331U);
        bool foundDifference = false;

        for (int index = 0; index < 64; ++index)
        {
            if (left.RollPercentage(45) !=
                right.RollPercentage(45))
            {
                foundDifference = true;
                break;
            }
        }

        test.Expect(
            foundDifference,
            "Different gathering seeds can produce different roll sequences");
    }

    {
        auto combat = std::make_unique<SequenceRandomSource>(
            std::vector<int>{});
        auto reward = std::make_unique<SequenceRandomSource>(
            std::vector<int>{});
        SequenceRandomSource *combatPtr = combat.get();
        SequenceRandomSource *rewardPtr = reward.get();

        World world(
            std::move(combat),
            std::move(reward));

        int playerID = world.CreatePlayer();
        std::optional<ResourceProbe> wood =
            FindResource(world, ResourceType::NORMAL_TREE);

        test.Expect(
            wood.has_value(),
            "Normal tree exists for two-source constructor compatibility");

        if (wood.has_value())
        {
            test.Expect(
                StartGatheringAt(
                    world,
                    playerID,
                    wood->x,
                    wood->y,
                    ItemType::BRONZE_AXE),
                "Two-source compatibility World can gather");

            ResolveSingleGatherCompletion(
                world,
                ItemType::BRONZE_AXE);

            test.ExpectEqual(
                combatPtr->GetCallCount(),
                0,
                "Two-source constructor does not reuse combat RNG for gathering");
            test.ExpectEqual(
                rewardPtr->GetCallCount(),
                0,
                "Two-source constructor does not reuse reward RNG for gathering");
        }
    }

    {
        auto gatherA = std::make_unique<SequenceRandomSource>(
            std::vector<int>{100, 1, 100, 1});
        auto gatherB = std::make_unique<SequenceRandomSource>(
            std::vector<int>{100, 1, 100, 1});
        SequenceRandomSource *gatherAPtr = gatherA.get();
        SequenceRandomSource *gatherBPtr = gatherB.get();

        World worldA(
            std::make_unique<SeededRandom>(1U),
            std::make_unique<SeededRandom>(2U),
            std::move(gatherA));

        World worldB(
            std::make_unique<SeededRandom>(99U),
            std::make_unique<SeededRandom>(77U),
            std::move(gatherB));

        int playerA = worldA.CreatePlayer();
        int playerB = worldB.CreatePlayer();

        std::optional<ResourceProbe> woodA =
            FindResourceForSkill(
                worldA,
                SkillType::WOODCUTTING);
        std::optional<ResourceProbe> woodB =
            FindResourceForSkill(
                worldB,
                SkillType::WOODCUTTING);

        test.Expect(
            woodA.has_value() && woodB.has_value(),
            "Woodcutting resources exist for sequence-independence checks");

        if (woodA.has_value() && woodB.has_value())
        {
            int logsBeforeA =
                GetPlayer(worldA, playerA)
                    ->GetInventory()
                    .GetItemAmount(ItemType::LOG);
            int logsBeforeB =
                GetPlayer(worldB, playerB)
                    ->GetInventory()
                    .GetItemAmount(ItemType::LOG);

            test.Expect(
                StartGatheringAt(
                    worldA,
                    playerA,
                    woodA->x,
                    woodA->y,
                    ItemType::BRONZE_AXE),
                "World A starts gathering");

            test.Expect(
                StartGatheringAt(
                    worldB,
                    playerB,
                    woodB->x,
                    woodB->y,
                    ItemType::BRONZE_AXE),
                "World B starts gathering");

            int extraAttacker = worldB.CreatePlayer();
            int extraDefender = worldB.CreatePlayer();

            test.Expect(
                ConsumeCombatRollsWithoutRewards(
                    worldB,
                    extraAttacker,
                    extraDefender),
                "World B consumes combat rolls without rolling rewards");

            ResolveSingleGatherCompletion(
                worldA,
                ItemType::BRONZE_AXE);
            ResolveSingleGatherCompletion(
                worldB,
                ItemType::BRONZE_AXE);

            int logsAfterA =
                GetPlayer(worldA, playerA)
                    ->GetInventory()
                    .GetItemAmount(ItemType::LOG);
            int logsAfterB =
                GetPlayer(worldB, playerB)
                    ->GetInventory()
                    .GetItemAmount(ItemType::LOG);

            test.ExpectEqual(
                logsAfterA - logsBeforeA,
                logsAfterB - logsBeforeB,
                "Combat RNG consumption does not change gathering outcomes");
            test.ExpectEqual(
                gatherAPtr->GetCallCount(),
                1,
                "Gather attempt consumes exactly one dedicated gathering roll in World A");
            test.ExpectEqual(
                gatherBPtr->GetCallCount(),
                1,
                "Gather attempt consumes exactly one dedicated gathering roll in World B");
        }
    }

    {
        auto combatA = std::make_unique<SequenceRandomSource>(
            std::vector<int>{1000000, 0, 1000000});
        auto combatB = std::make_unique<SequenceRandomSource>(
            std::vector<int>{1000000, 0, 1000000});
        auto rewardA = std::make_unique<SequenceRandomSource>(
            std::vector<int>{95});
        auto rewardB = std::make_unique<SequenceRandomSource>(
            std::vector<int>{95});
        auto gatherA = std::make_unique<SequenceRandomSource>(
            std::vector<int>{100});
        auto gatherB = std::make_unique<SequenceRandomSource>(
            std::vector<int>{100});

        SequenceRandomSource *combatAPtr = combatA.get();
        SequenceRandomSource *combatBPtr = combatB.get();
        SequenceRandomSource *rewardAPtr = rewardA.get();
        SequenceRandomSource *rewardBPtr = rewardB.get();
        SequenceRandomSource *gatherAPtr = gatherA.get();
        SequenceRandomSource *gatherBPtr = gatherB.get();

        World worldA(
            std::move(combatA),
            std::move(rewardA),
            std::move(gatherA));
        World worldB(
            std::move(combatB),
            std::move(rewardB),
            std::move(gatherB));

        int playerA = worldA.CreatePlayer();
        int playerB = worldB.CreatePlayer();
        int monsterA = worldA.CreateMonster(
            22,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::NONE);
        int monsterB = worldB.CreateMonster(
            22,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);

        test.Expect(
            KillMonsterWithPlayer(worldA, playerA, monsterA),
            "Control monster kill completes without a reward table");
        test.Expect(
            KillMonsterWithPlayer(worldB, playerB, monsterB),
            "Comparison monster kill completes with a reward table");
        test.ExpectEqual(
            combatAPtr->GetCallCount(),
            combatBPtr->GetCallCount(),
            "Reward isolation uses identical combat consumption");
        test.ExpectEqual(
            rewardAPtr->GetCallCount(),
            0,
            "No-table monster consumes no reward roll");
        test.Expect(
            rewardBPtr->GetCallCount() > 0,
            "Development reward table consumes reward RNG");

        std::optional<ResourceProbe> woodA =
            FindResource(worldA, ResourceType::NORMAL_TREE);
        std::optional<ResourceProbe> woodB =
            FindResource(worldB, ResourceType::NORMAL_TREE);

        test.Expect(
            woodA.has_value() && woodB.has_value(),
            "Woodcutting resources exist for reward-stream isolation");

        if (woodA.has_value() && woodB.has_value())
        {
            int logsBeforeA = GetPlayer(worldA, playerA)
                                  ->GetInventory()
                                  .GetItemAmount(ItemType::LOG);
            int logsBeforeB = GetPlayer(worldB, playerB)
                                  ->GetInventory()
                                  .GetItemAmount(ItemType::LOG);

            test.Expect(
                StartGatheringAt(
                    worldA,
                    playerA,
                    woodA->x,
                    woodA->y,
                    ItemType::BRONZE_AXE),
                "Control gathering starts after no reward roll");
            test.Expect(
                StartGatheringAt(
                    worldB,
                    playerB,
                    woodB->x,
                    woodB->y,
                    ItemType::BRONZE_AXE),
                "Comparison gathering starts after reward consumption");

            ResolveSingleGatherCompletion(
                worldA,
                ItemType::BRONZE_AXE);
            ResolveSingleGatherCompletion(
                worldB,
                ItemType::BRONZE_AXE);

            test.ExpectEqual(
                GetPlayer(worldA, playerA)
                        ->GetInventory()
                        .GetItemAmount(ItemType::LOG) -
                    logsBeforeA,
                GetPlayer(worldB, playerB)
                        ->GetInventory()
                        .GetItemAmount(ItemType::LOG) -
                    logsBeforeB,
                "Reward RNG consumption does not change gathering outcomes");
            test.ExpectEqual(
                gatherAPtr->GetCallCount(),
                1,
                "Control gather consumes one gathering roll");
            test.ExpectEqual(
                gatherBPtr->GetCallCount(),
                1,
                "Gather after reward consumption uses its own one roll");
        }
    }

    {
        auto consumedGather =
            std::make_unique<SequenceRandomSource>(
                std::vector<int>{1});
        SequenceRandomSource *consumedGatherPtr =
            consumedGather.get();

        World baseline(
            std::make_unique<SeededRandom>(500U),
            std::make_unique<SeededRandom>(600U),
            std::make_unique<SeededRandom>(700U));

        World withGatherConsumption(
            std::make_unique<SeededRandom>(500U),
            std::make_unique<SeededRandom>(600U),
            std::move(consumedGather));

        int baseAttacker = baseline.CreatePlayer();
        int baseDefender = baseline.CreatePlayer();

        int gatherAttacker = withGatherConsumption.CreatePlayer();
        int gatherDefender = withGatherConsumption.CreatePlayer();

        GetPlayer(baseline, baseAttacker)->GetPosition().SetPosition(10, 10);
        GetPlayer(baseline, baseDefender)->GetPosition().SetPosition(11, 10);

        std::optional<ResourceProbe> wood =
            FindResourceForSkill(
                withGatherConsumption,
                SkillType::WOODCUTTING);

        int gatherPlayer = withGatherConsumption.CreatePlayer();

        test.Expect(
            wood.has_value(),
            "Woodcutting resource exists for combat independence check");

        if (wood.has_value())
        {
            test.Expect(
                StartGatheringAt(
                    withGatherConsumption,
                    gatherPlayer,
                    wood->x,
                    wood->y,
                    ItemType::BRONZE_AXE),
                "Gathering can be started before combat independence check");

            ResolveSingleGatherCompletion(
                withGatherConsumption,
                ItemType::BRONZE_AXE);

            test.ExpectEqual(
                consumedGatherPtr->GetCallCount(),
                1,
                "Combat independence setup consumes one gathering roll");
        }

        GetPlayer(withGatherConsumption, gatherAttacker)->GetPosition().SetPosition(10, 10);
        GetPlayer(withGatherConsumption, gatherDefender)->GetPosition().SetPosition(11, 10);

        test.Expect(
            baseline.TryStartMeleeAttack(
                baseAttacker,
                baseDefender,
                1),
            "Baseline melee attack starts");

        test.Expect(
            withGatherConsumption.TryStartMeleeAttack(
                gatherAttacker,
                gatherDefender,
                1),
            "Melee attack starts after gathering RNG consumption");

        baseline.Update();
        withGatherConsumption.Update();

        const std::optional<MeleeAttackResult> &baseResult =
            baseline.GetLastMeleeAttackResult();
        const std::optional<MeleeAttackResult> &gatherResult =
            withGatherConsumption.GetLastMeleeAttackResult();

        test.Expect(
            baseResult.has_value() && gatherResult.has_value(),
            "Both worlds produced a combat result");

        if (baseResult.has_value() && gatherResult.has_value())
        {
            test.ExpectEqual(
                baseResult->attackerRolledResult,
                gatherResult->attackerRolledResult,
                "Gathering RNG consumption does not change the combat attack roll");
            test.ExpectEqual(
                baseResult->defenderRolledResult,
                gatherResult->defenderRolledResult,
                "Gathering RNG consumption does not change the combat defence roll");
            test.ExpectEqual(
                static_cast<int>(baseResult->didHit),
                static_cast<int>(gatherResult->didHit),
                "Gathering RNG consumption does not change combat hit resolution");
            test.ExpectEqual(
                baseResult->rolledDamage,
                gatherResult->rolledDamage,
                "Gathering RNG consumption does not change combat damage roll");
            test.ExpectEqual(
                baseResult->actualDamageApplied,
                gatherResult->actualDamageApplied,
                "Gathering RNG consumption does not change applied combat damage");
        }
    }

    {
        auto baseCombat = std::make_unique<SequenceRandomSource>(
            std::vector<int>{1000000, 0, 1000000});
        auto gatherCombat = std::make_unique<SequenceRandomSource>(
            std::vector<int>{1000000, 0, 1000000});
        auto baseReward = std::make_unique<SequenceRandomSource>(
            std::vector<int>{95});
        auto gatherReward = std::make_unique<SequenceRandomSource>(
            std::vector<int>{95});
        auto consumedGather = std::make_unique<SequenceRandomSource>(
            std::vector<int>{1});

        SequenceRandomSource *baseRewardPtr = baseReward.get();
        SequenceRandomSource *gatherRewardPtr = gatherReward.get();
        SequenceRandomSource *consumedGatherPtr = consumedGather.get();

        World baseline(
            std::move(baseCombat),
            std::move(baseReward),
            std::make_unique<SeededRandom>(902U));

        World withGatherConsumption(
            std::move(gatherCombat),
            std::move(gatherReward),
            std::move(consumedGather));

        int basePlayer = baseline.CreatePlayer();
        int gatherPlayer = withGatherConsumption.CreatePlayer();

        std::optional<ResourceProbe> wood =
            FindResourceForSkill(
                withGatherConsumption,
                SkillType::WOODCUTTING);

        test.Expect(
            wood.has_value(),
            "Woodcutting resource exists for loot independence check");

        if (wood.has_value())
        {
            test.Expect(
                StartGatheringAt(
                    withGatherConsumption,
                    gatherPlayer,
                    wood->x,
                    wood->y,
                    ItemType::BRONZE_AXE),
                "Gathering starts before loot independence check");

            ResolveSingleGatherCompletion(
                withGatherConsumption,
                ItemType::BRONZE_AXE);

            test.ExpectEqual(
                consumedGatherPtr->GetCallCount(),
                1,
                "Loot independence setup consumes one gathering roll");
        }

        int baseMonster = baseline.CreateMonster(
            15,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);

        int gatherMonster = withGatherConsumption.CreateMonster(
            15,
            10,
            MakeRatings(1, 1, 1, 1),
            RewardTableType::DEVELOPMENT_MONSTER);

        test.Expect(
            KillMonsterWithPlayer(
                baseline,
                basePlayer,
                baseMonster),
            "Baseline monster kill succeeds for loot comparison");

        test.Expect(
            KillMonsterWithPlayer(
                withGatherConsumption,
                gatherPlayer,
                gatherMonster),
            "Monster kill succeeds after gathering RNG consumption");

        RewardSnapshot baseLoot =
            GetRewardSnapshot(
                GetPlayer(baseline, basePlayer)
                    ->GetInventory());

        RewardSnapshot gatheredLoot =
            GetRewardSnapshot(
                GetPlayer(withGatherConsumption, gatherPlayer)
                    ->GetInventory());

        test.ExpectEqual(
            baseLoot.coal,
            gatheredLoot.coal,
            "Gathering RNG consumption does not change coal rewards");
        test.ExpectEqual(
            baseLoot.copper,
            gatheredLoot.copper,
            "Gathering RNG consumption does not change copper rewards");
        test.ExpectEqual(
            baseLoot.tin,
            gatheredLoot.tin,
            "Gathering RNG consumption does not change tin rewards");
        test.ExpectEqual(
            baseLoot.iron,
            gatheredLoot.iron,
            "Gathering RNG consumption does not change iron rewards");
        test.ExpectEqual(
            baseRewardPtr->GetCallCount(),
            gatherRewardPtr->GetCallCount(),
            "Gathering RNG consumption does not change reward draw count");
    }

    {
        auto scripted = std::make_unique<SequenceRandomSource>(
            std::vector<int>{1});
        SequenceRandomSource *scriptedPtr = scripted.get();

        World world(
            std::make_unique<SeededRandom>(123U),
            std::make_unique<SeededRandom>(456U),
            std::move(scripted));

        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);
        std::optional<ResourceProbe> wood =
            FindResource(world, ResourceType::NORMAL_TREE);

        test.Expect(
            wood.has_value(),
            "Normal tree exists for deterministic woodcutting success");

        if (wood.has_value())
        {
            ResourceNode *resource =
                world.GetResourceAt(wood->x, wood->y);
            const ResourceDefinition &definition =
                ResourceDatabase::Get(resource->GetResourceType());
            int logsBefore = player->GetInventory().GetItemAmount(
                definition.GetItemReward());
            int xpBefore = player->GetSkills()
                               .GetSkill(SkillType::WOODCUTTING)
                               .GetXP();
            int usesBefore = resource->GetRemainingUses();

            test.Expect(
                StartGatheringAt(
                    world,
                    playerID,
                    wood->x,
                    wood->y,
                    ItemType::BRONZE_AXE),
                "Scripted woodcutting success starts");
            test.ExpectEqual(
                scriptedPtr->GetCallCount(),
                0,
                "Starting gathering consumes no random roll");

            AdvanceTicks(
                world,
                GetGatheringDurationTicks(ItemType::BRONZE_AXE) - 1);
            test.ExpectEqual(
                scriptedPtr->GetCallCount(),
                0,
                "Gathering progress ticks consume no random roll");

            AdvanceTicks(world, 1);

            test.ExpectEqual(
                scriptedPtr->GetCallCount(),
                1,
                "A completed semantic attempt consumes exactly one roll");
            test.ExpectEqual(
                player->GetInventory().GetItemAmount(
                    definition.GetItemReward()) -
                    logsBefore,
                definition.GetItemAmount(),
                "Scripted success grants exact woodcutting reward amount");
            test.ExpectEqual(
                player->GetSkills()
                        .GetSkill(SkillType::WOODCUTTING)
                        .GetXP() -
                    xpBefore,
                definition.GetXPReward(),
                "Scripted success grants exact woodcutting XP");
            test.ExpectEqual(
                usesBefore - resource->GetRemainingUses(),
                1,
                "Successful gather consumes one resource use");
            test.Expect(
                !resource->IsActive(),
                "Single-use tree depletes after a successful gather");
            test.Expect(
                world.GetActionForEntity(playerID) == nullptr,
                "Gathering stops when the resource depletes");
        }
    }

    {
        auto scripted = std::make_unique<SequenceRandomSource>(
            std::vector<int>{100});
        SequenceRandomSource *scriptedPtr = scripted.get();

        World world(
            std::make_unique<SeededRandom>(124U),
            std::make_unique<SeededRandom>(457U),
            std::move(scripted));

        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);
        std::optional<ResourceProbe> wood =
            FindResource(world, ResourceType::NORMAL_TREE);

        test.Expect(
            wood.has_value(),
            "Normal tree exists for deterministic woodcutting failure");

        if (wood.has_value())
        {
            ResourceNode *resource =
                world.GetResourceAt(wood->x, wood->y);
            const ResourceDefinition &definition =
                ResourceDatabase::Get(resource->GetResourceType());
            int logsBefore = player->GetInventory().GetItemAmount(
                definition.GetItemReward());
            int xpBefore = player->GetSkills()
                               .GetSkill(SkillType::WOODCUTTING)
                               .GetXP();
            int usesBefore = resource->GetRemainingUses();

            test.Expect(
                StartGatheringAt(
                    world,
                    playerID,
                    wood->x,
                    wood->y,
                    ItemType::BRONZE_AXE),
                "Scripted woodcutting failure starts");

            ResolveSingleGatherCompletion(
                world,
                ItemType::BRONZE_AXE);

            test.ExpectEqual(
                scriptedPtr->GetCallCount(),
                1,
                "Failed completed attempt consumes exactly one roll");
            test.ExpectEqual(
                player->GetInventory().GetItemAmount(
                    definition.GetItemReward()),
                logsBefore,
                "Scripted failure grants no woodcutting reward");
            test.ExpectEqual(
                player->GetSkills()
                    .GetSkill(SkillType::WOODCUTTING)
                    .GetXP(),
                xpBefore,
                "Scripted failure grants no woodcutting XP");
            test.ExpectEqual(
                resource->GetRemainingUses(),
                usesBefore,
                "Scripted failure consumes no resource use");
            test.Expect(
                resource->IsActive(),
                "Resource remains active after a failed attempt");
            test.Expect(
                world.GetActionForEntity(playerID) != nullptr,
                "Gathering repeats after a failed attempt");
        }
    }

    {
        auto scripted = std::make_unique<SequenceRandomSource>(
            std::vector<int>{100, 100, 100});
        SequenceRandomSource *scriptedPtr = scripted.get();

        World world(
            std::make_unique<SeededRandom>(125U),
            std::make_unique<SeededRandom>(458U),
            std::move(scripted));

        int playerID = world.CreatePlayer();
        std::optional<ResourceProbe> wood =
            FindResource(world, ResourceType::NORMAL_TREE);

        test.Expect(
            wood.has_value(),
            "Normal tree exists for repeat and cancellation checks");

        if (wood.has_value())
        {
            test.Expect(
                StartGatheringAt(
                    world,
                    playerID,
                    wood->x,
                    wood->y,
                    ItemType::BRONZE_AXE),
                "Repeating gathering starts");

            for (int attempt = 1; attempt <= 3; ++attempt)
            {
                ResolveSingleGatherCompletion(
                    world,
                    ItemType::BRONZE_AXE);
                test.ExpectEqual(
                    scriptedPtr->GetCallCount(),
                    attempt,
                    "Repeating gathering consumes one roll per completed attempt");
            }

            test.Expect(
                world.GetActionForEntity(playerID) != nullptr,
                "Failed gathering attempts continue repeating");

            world.CancelActionsForEntity(
                playerID,
                ActionCancelReason::INTERFACE_CLOSED);
            world.Update();
            ResolveSingleGatherCompletion(
                world,
                ItemType::BRONZE_AXE);

            test.ExpectEqual(
                scriptedPtr->GetCallCount(),
                3,
                "Cancelled gathering consumes no completion roll");
            test.Expect(
                world.GetActionForEntity(playerID) == nullptr,
                "Cancelled gathering remains stopped");
        }
    }

    {
        auto scripted = std::make_unique<SequenceRandomSource>(
            std::vector<int>{});
        SequenceRandomSource *scriptedPtr = scripted.get();

        World world(
            std::make_unique<SeededRandom>(126U),
            std::make_unique<SeededRandom>(459U),
            std::move(scripted));

        int playerID = world.CreatePlayer();
        std::optional<ResourceProbe> wood =
            FindResource(world, ResourceType::NORMAL_TREE);

        test.Expect(
            wood.has_value(),
            "Normal tree exists for removed-tool validation");

        if (wood.has_value())
        {
            test.Expect(
                StartGatheringAt(
                    world,
                    playerID,
                    wood->x,
                    wood->y,
                    ItemType::BRONZE_AXE),
                "Gathering starts before its tool is removed");
            test.Expect(
                world.TryUnequipItem(
                    playerID,
                    EquipmentSlotType::WEAPON),
                "Gathering tool can be removed before completion");

            ResolveSingleGatherCompletion(
                world,
                ItemType::BRONZE_AXE);

            test.ExpectEqual(
                scriptedPtr->GetCallCount(),
                0,
                "Removing the gathering tool consumes no completion roll");
        }
    }

    {
        auto scripted = std::make_unique<SequenceRandomSource>(
            std::vector<int>{});
        SequenceRandomSource *scriptedPtr = scripted.get();

        World world(
            std::make_unique<SeededRandom>(127U),
            std::make_unique<SeededRandom>(460U),
            std::move(scripted));

        int playerID = world.CreatePlayer();
        std::optional<ResourceProbe> wood =
            FindResource(world, ResourceType::NORMAL_TREE);

        test.Expect(
            wood.has_value(),
            "Normal tree exists for depleted-target validation");

        if (wood.has_value())
        {
            test.Expect(
                StartGatheringAt(
                    world,
                    playerID,
                    wood->x,
                    wood->y,
                    ItemType::BRONZE_AXE),
                "Gathering starts before the target depletes");

            ResourceNode *resource =
                world.GetResourceAt(wood->x, wood->y);
            resource->ConsumeUse();

            ResolveSingleGatherCompletion(
                world,
                ItemType::BRONZE_AXE);

            test.ExpectEqual(
                scriptedPtr->GetCallCount(),
                0,
                "A depleted target consumes no gathering roll");
        }
    }

    {
        auto scripted = std::make_unique<SequenceRandomSource>(
            std::vector<int>{});
        SequenceRandomSource *scriptedPtr = scripted.get();

        World world(
            std::make_unique<SeededRandom>(128U),
            std::make_unique<SeededRandom>(461U),
            std::move(scripted));

        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);
        std::optional<ResourceProbe> wood =
            FindResource(world, ResourceType::NORMAL_TREE);

        test.Expect(
            wood.has_value(),
            "Normal tree exists for dead-player validation");

        if (wood.has_value())
        {
            test.Expect(
                StartGatheringAt(
                    world,
                    playerID,
                    wood->x,
                    wood->y,
                    ItemType::BRONZE_AXE),
                "Gathering starts before the player dies");

            player->ApplyDamage(player->GetCurrentHealth());
            ResolveSingleGatherCompletion(
                world,
                ItemType::BRONZE_AXE);

            test.ExpectEqual(
                scriptedPtr->GetCallCount(),
                0,
                "A dead player consumes no gathering roll");
        }
    }

    {
        auto combat = std::make_unique<SequenceRandomSource>(
            std::vector<int>{1000000, 0, 1000000});
        auto gathering = std::make_unique<SequenceRandomSource>(
            std::vector<int>{});
        SequenceRandomSource *combatPtr = combat.get();
        SequenceRandomSource *gatheringPtr = gathering.get();

        World world(
            std::move(combat),
            std::make_unique<SeededRandom>(462U),
            std::move(gathering));

        int attackerID = world.CreatePlayer();
        int gathererID = world.CreatePlayer();
        Player *attacker = GetPlayer(world, attackerID);
        Player *gatherer = GetPlayer(world, gathererID);
        std::optional<ResourceProbe> wood =
            FindResource(world, ResourceType::NORMAL_TREE);

        test.Expect(
            wood.has_value(),
            "Normal tree exists for same-tick death validation");

        if (wood.has_value())
        {
            ResourceNode *resource =
                world.GetResourceAt(wood->x, wood->y);
            const ResourceDefinition &definition =
                ResourceDatabase::Get(resource->GetResourceType());
            int logsBefore = gatherer->GetInventory().GetItemAmount(
                definition.GetItemReward());
            int xpBefore = gatherer->GetSkills()
                               .GetSkill(SkillType::WOODCUTTING)
                               .GetXP();
            int usesBefore = resource->GetRemainingUses();

            attacker->GetPosition().SetPosition(
                wood->x - 2,
                wood->y);
            gatherer->GetPosition().SetPosition(
                wood->x - 1,
                wood->y);
            gatherer->ApplyDamage(
                gatherer->GetCurrentHealth() - 1);

            test.Expect(
                world.TryStartMeleeAttack(
                    attackerID,
                    gathererID,
                    GetGatheringDurationTicks(
                        ItemType::BRONZE_AXE) +
                        1),
                "Attack starts before gathering with a shared completion tick");
            test.Expect(
                StartGatheringAt(
                    world,
                    gathererID,
                    wood->x,
                    wood->y,
                    ItemType::BRONZE_AXE),
                "Gathering starts one tick after the lethal attack");

            ResolveSingleGatherCompletion(
                world,
                ItemType::BRONZE_AXE);

            test.Expect(
                !gatherer->IsAlive(),
                "Earlier completed attack kills the gatherer");
            test.ExpectEqual(
                combatPtr->GetCallCount(),
                3,
                "Lethal attack consumes its scripted combat rolls");
            test.ExpectEqual(
                gatheringPtr->GetCallCount(),
                0,
                "Completion validation rejects a dead gatherer before rolling");
            test.ExpectEqual(
                gatherer->GetInventory().GetItemAmount(
                    definition.GetItemReward()),
                logsBefore,
                "Dead gatherer receives no item reward");
            test.ExpectEqual(
                gatherer->GetSkills()
                    .GetSkill(SkillType::WOODCUTTING)
                    .GetXP(),
                xpBefore,
                "Dead gatherer receives no XP");
            test.ExpectEqual(
                resource->GetRemainingUses(),
                usesBefore,
                "Dead gatherer consumes no resource use");
        }
    }

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{});
        SequenceRandomSource *sequencePtr = sequence.get();

        World world(
            std::make_unique<SeededRandom>(222U),
            std::make_unique<SeededRandom>(333U),
            std::move(sequence));

        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        std::optional<ResourceProbe> wood =
            FindResourceForSkill(
                world,
                SkillType::WOODCUTTING);

        test.Expect(
            wood.has_value(),
            "Woodcutting resource exists for completion-validation tests");

        if (wood.has_value())
        {
            test.Expect(
                StartGatheringAt(
                    world,
                    playerID,
                    wood->x,
                    wood->y,
                    ItemType::BRONZE_AXE),
                "Gathering starts before validation-failure test");

            player->GetPosition().SetPosition(50, 50);

            ResolveSingleGatherCompletion(
                world,
                ItemType::BRONZE_AXE);

            test.ExpectEqual(
                sequencePtr->GetCallCount(),
                0,
                "Failed completion validation consumes no gathering roll");
        }
    }

    {
        auto scripted = std::make_unique<SequenceRandomSource>(
            std::vector<int>{});
        SequenceRandomSource *scriptedPtr = scripted.get();

        World world(
            std::make_unique<SeededRandom>(444U),
            std::make_unique<SeededRandom>(555U),
            std::move(scripted));

        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        std::optional<ResourceProbe> wood =
            FindResourceForSkill(
                world,
                SkillType::WOODCUTTING);

        test.Expect(
            wood.has_value(),
            "Woodcutting resource exists for inventory-full behavior check");

        if (wood.has_value())
        {
            ResourceNode *resource =
                world.GetResourceAt(
                    wood->x,
                    wood->y);
            int usesBefore =
                resource->GetRemainingUses();
            int xpBefore =
                player->GetSkills()
                    .GetSkill(SkillType::WOODCUTTING)
                    .GetXP();

            test.Expect(
                StartGatheringAt(
                    world,
                    playerID,
                    wood->x,
                    wood->y,
                    ItemType::BRONZE_AXE),
                "Gathering starts before the inventory is filled");

            while (player->GetInventory().AddItem(
                ItemType::LOG,
                1))
            {
            }

            int logsBefore =
                player->GetInventory().GetItemAmount(ItemType::LOG);

            ResolveSingleGatherCompletion(
                world,
                ItemType::BRONZE_AXE);

            test.ExpectEqual(
                player->GetInventory().GetItemAmount(ItemType::LOG),
                logsBefore,
                "Inventory-full completion does not incorrectly award items");
            test.ExpectEqual(
                resource->GetRemainingUses(),
                usesBefore,
                "Inventory-full completion does not consume resource use");
            test.ExpectEqual(
                player->GetSkills()
                    .GetSkill(SkillType::WOODCUTTING)
                    .GetXP(),
                xpBefore,
                "Inventory-full completion does not award XP");
            test.ExpectEqual(
                scriptedPtr->GetCallCount(),
                0,
                "Inventory-full completion validation consumes no roll");
        }
    }

    {
        auto scripted = std::make_unique<SequenceRandomSource>(
            std::vector<int>{1});
        SequenceRandomSource *scriptedPtr = scripted.get();

        World world(
            std::make_unique<SeededRandom>(111U),
            std::make_unique<SeededRandom>(222U),
            std::move(scripted));

        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        std::optional<ResourceProbe> mining =
            FindResource(world, ResourceType::COPPER_ROCK);

        test.Expect(
            mining.has_value(),
            "Mining resource exists for deterministic mining reward assertions");

        if (mining.has_value())
        {
            ResourceNode *resource =
                world.GetResourceAt(
                    mining->x,
                    mining->y);

            const ResourceDefinition &definition =
                ResourceDatabase::Get(
                    resource->GetResourceType());

            int oreBefore =
                player->GetInventory().GetItemAmount(
                    definition.GetItemReward());
            int xpBefore =
                player->GetSkills()
                    .GetSkill(SkillType::MINING)
                    .GetXP();

            test.Expect(
                StartGatheringAt(
                    world,
                    playerID,
                    mining->x,
                    mining->y,
                    ItemType::BRONZE_PICKAXE),
                "Scripted mining starts with bronze pickaxe");

            ResolveSingleGatherCompletion(
                world,
                ItemType::BRONZE_PICKAXE);

            test.ExpectEqual(
                scriptedPtr->GetCallCount(),
                1,
                "Completed mining attempt consumes exactly one roll");

            test.ExpectEqual(
                player->GetInventory().GetItemAmount(
                    definition.GetItemReward()) -
                    oreBefore,
                definition.GetItemAmount(),
                "Scripted mining success awards deterministic ore quantity");
            test.ExpectEqual(
                player->GetSkills()
                        .GetSkill(SkillType::MINING)
                        .GetXP() -
                    xpBefore,
                definition.GetXPReward(),
                "Scripted mining success awards deterministic mining XP");
        }
    }

    return test.Finish();
}

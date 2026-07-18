#include "TestSupport.h"

#include "../src/Combat/CombatFormulas.h"
#include "../src/Combat/MeleeAttackResolver.h"
#include "../src/Core/RandomSource.h"
#include "../src/Core/SeededRandom.h"
#include "../src/Equipment/EquipmentSlotType.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Player/Player.h"
#include "../src/Skills/SkillType.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/Stats/HealthPool.h"
#include "../src/Stats/StatType.h"

#include <utility>
#include <vector>

namespace
{
    class HealthPoolCombatant : public Combatant
    {
    public:
        explicit HealthPoolCombatant(int maximumHealth)
            : healthPool(maximumHealth)
        {
        }

        CombatRatings GetCombatRatings() const override
        {
            return CombatRatings{};
        }

        int GetCurrentHealth() const override
        {
            return healthPool.GetCurrentHealth();
        }

        int GetMaximumHealth() const override
        {
            return healthPool.GetMaximumHealth();
        }

        bool IsAlive() const override
        {
            return healthPool.IsAlive();
        }

        int ApplyDamage(int amount) override
        {
            return healthPool.ApplyDamage(amount);
        }

    private:
        HealthPool healthPool;
    };

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
                int temp = minimum;
                minimum = maximum;
                maximum = temp;
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

    void AdvanceSkillToLevel(
        Player &player,
        SkillType skillType,
        int targetLevel)
    {
        while (player.GetSkills().GetSkill(skillType).GetLevel() < targetLevel)
        {
            const Skill &skill = player.GetSkills().GetSkill(skillType);
            int xpNeeded = skill.GetXPForNextLevel() - skill.GetXP();
            player.GetSkills().AddXP(
                skillType,
                xpNeeded);
        }
    }
}

int main()
{
    TestContext test;

    {
        SeededRandom left(12345);
        SeededRandom right(12345);

        for (int index = 0; index < 32; ++index)
        {
            test.ExpectEqual(
                left.NextIntInclusive(-5, 11),
                right.NextIntInclusive(-5, 11),
                "Same seed and same calls produce the same integer sequence");
        }
    }

    {
        SeededRandom left(12345);
        SeededRandom right(54321);
        bool foundDifference = false;

        for (int index = 0; index < 32; ++index)
        {
            if (left.NextIntInclusive(0, 1000) != right.NextIntInclusive(0, 1000))
            {
                foundDifference = true;
                break;
            }
        }

        test.Expect(
            foundDifference,
            "Different seeds normally produce different sequences");
    }

    {
        SequenceRandomSource sequence({0, 5});

        test.ExpectEqual(
            sequence.NextIntInclusive(0, 5),
            0,
            "Controllable test RNG can produce the lower inclusive bound");
        test.ExpectEqual(
            sequence.NextIntInclusive(0, 5),
            5,
            "Controllable test RNG can produce the upper inclusive bound");
    }

    {
        SeededRandom random(99);

        test.ExpectEqual(
            random.NextIntInclusive(7, 7),
            7,
            "Equal bounds always return that bound");
    }

    {
        SeededRandom random(77);

        for (int index = 0; index < 64; ++index)
        {
            int rolled = random.NextIntInclusive(9, 2);

            test.Expect(
                rolled >= 2 && rolled <= 9,
                "Reversed bounds are handled by swapping into an inclusive range");
        }
    }

    {
        MeleeAttackResolver resolver;
        CombatRatings attacker = MakeRatings(10, 10, 5, 100);
        CombatRatings defender = MakeRatings(5, 5, 10, 100);
        HealthPoolCombatant defenderHealth(20);
        SequenceRandomSource sequence({3, 4, 7});

        MeleeAttackResult result = resolver.Resolve(
            attacker,
            defender,
            defenderHealth,
            sequence);

        test.Expect(
            !result.didHit,
            "Lower attacker roll result than defender roll result misses");
        test.ExpectEqual(
            result.rolledDamage,
            0,
            "Miss sets rolled damage to zero");
        test.ExpectEqual(
            result.actualDamageApplied,
            0,
            "Miss applies zero damage");
        test.ExpectEqual(
            defenderHealth.GetCurrentHealth(),
            20,
            "Miss does not reduce health");
        test.ExpectEqual(
            sequence.GetCallCount(),
            2,
            "Miss consumes only attack and defence rolls");
    }

    {
        MeleeAttackResolver resolver;
        CombatRatings attacker = MakeRatings(8, 10, 5, 100);
        CombatRatings defender = MakeRatings(5, 5, 8, 100);
        HealthPoolCombatant defenderHealth(20);
        SequenceRandomSource sequence({5, 5, 9});

        MeleeAttackResult result = resolver.Resolve(
            attacker,
            defender,
            defenderHealth,
            sequence);

        test.Expect(
            !result.didHit,
            "Equal attacker and defender rolls miss");
        test.ExpectEqual(
            sequence.GetCallCount(),
            2,
            "Equal-roll miss does not consume a damage roll");
    }

    {
        MeleeAttackResolver resolver;
        CombatRatings attacker = MakeRatings(12, 20, 5, 100);
        CombatRatings defender = MakeRatings(5, 5, 6, 100);
        HealthPoolCombatant defenderHealth(20);
        SequenceRandomSource sequence({6, 2, 2});

        MeleeAttackResult result = resolver.Resolve(
            attacker,
            defender,
            defenderHealth,
            sequence);

        test.Expect(
            result.didHit,
            "Higher attacker roll result than defender roll result hits");
        test.ExpectEqual(
            sequence.GetCallCount(),
            3,
            "Hit consumes attack roll, defence roll, and damage roll");
        test.ExpectEqual(
            result.actualDamageApplied,
            2,
            "Actual damage reflects health removed when enough health is available");
        test.ExpectEqual(
            defenderHealth.GetCurrentHealth(),
            18,
            "Hit reduces health by actual applied damage");
    }

    {
        MeleeAttackResolver resolver;
        CombatRatings attacker = MakeRatings(12, 20, 5, 100);
        CombatRatings defender = MakeRatings(5, 5, 6, 100);
        HealthPoolCombatant defenderHealth(20);
        SequenceRandomSource sequence({6, 1, 0});

        MeleeAttackResult result = resolver.Resolve(
            attacker,
            defender,
            defenderHealth,
            sequence);

        test.Expect(
            result.didHit,
            "A successful hit can still roll zero damage");
        test.ExpectEqual(
            result.rolledDamage,
            0,
            "Zero damage roll is preserved");
        test.ExpectEqual(
            result.actualDamageApplied,
            0,
            "Zero rolled damage applies zero actual damage");
    }

    {
        MeleeAttackResolver resolver;
        CombatRatings attacker = MakeRatings(12, 20, 5, 100);
        CombatRatings defender = MakeRatings(5, 5, 6, 100);
        int maximumHit = CombatFormulas::CalculateMaximumHit(attacker);
        HealthPoolCombatant defenderHealth(20);
        SequenceRandomSource sequence({6, 1, maximumHit});

        MeleeAttackResult result = resolver.Resolve(
            attacker,
            defender,
            defenderHealth,
            sequence);

        test.ExpectEqual(
            result.rolledDamage,
            maximumHit,
            "Maximum possible damage can be rolled");
        test.Expect(
            result.rolledDamage <= result.maximumHit,
            "Resolved rolled damage never exceeds maximum hit");
    }

    {
        MeleeAttackResolver resolver;
        CombatRatings attacker = MakeRatings(12, 20, 5, 100);
        CombatRatings defender = MakeRatings(5, 5, 6, 100);
        HealthPoolCombatant defenderHealth(2);
        SequenceRandomSource sequence({6, 1, 5});

        MeleeAttackResult result = resolver.Resolve(
            attacker,
            defender,
            defenderHealth,
            sequence);

        test.ExpectEqual(
            result.actualDamageApplied,
            2,
            "Overkill reports only remaining defender health as actual damage");
        test.ExpectEqual(
            defenderHealth.GetCurrentHealth(),
            0,
            "Overkill still clamps defender health at zero");
    }

    {
        MeleeAttackResolver resolver;
        CombatRatings attacker = MakeRatings(12, 20, 5, 100);
        CombatRatings defender = MakeRatings(5, 5, 6, 100);
        HealthPoolCombatant defenderHealth(1);
        defenderHealth.ApplyDamage(1);

        SequenceRandomSource sequence({6, 1, 3});

        MeleeAttackResult result = resolver.Resolve(
            attacker,
            defender,
            defenderHealth,
            sequence);

        test.Expect(
            result.didHit,
            "Dead targets are still resolvable in this formula checkpoint");
        test.ExpectEqual(
            result.actualDamageApplied,
            0,
            "Dead target takes zero actual damage");
        test.ExpectEqual(
            defenderHealth.GetCurrentHealth(),
            0,
            "Dead target remains at zero health");
    }

    {
        HealthPool health(10);

        test.ExpectEqual(
            health.ApplyDamage(4),
            4,
            "HealthPool reports applied damage when enough health is available");
        test.ExpectEqual(
            health.ApplyDamage(50),
            6,
            "HealthPool reports only remaining health on overkill");
        test.ExpectEqual(
            health.ApplyDamage(3),
            0,
            "HealthPool reports zero damage when already dead");
        test.ExpectEqual(
            health.ApplyDamage(0),
            0,
            "HealthPool reports zero for zero damage");
        test.ExpectEqual(
            health.ApplyDamage(-3),
            0,
            "HealthPool reports zero for negative damage");
    }

    {
        Player attacker(3001);
        Player defender(3002);
        MeleeAttackResolver resolver;

        CombatRatings attackerRatings = attacker.GetCombatRatings();
        CombatRatings defenderRatings = defender.GetCombatRatings();
        HealthPoolCombatant defenderHealth(defenderRatings.maximumHealth);

        SequenceRandomSource sequence({0, 0, 0});

        MeleeAttackResult baseline = resolver.Resolve(
            attackerRatings,
            defenderRatings,
            defenderHealth,
            sequence);

        test.ExpectEqual(
            baseline.attackerMaximumRoll,
            CombatFormulas::CalculateAttackRoll(attackerRatings),
            "Resolver exposes calculated attacker maximum roll");
        test.ExpectEqual(
            baseline.defenderMaximumRoll,
            CombatFormulas::CalculateDefenceRoll(defenderRatings),
            "Resolver exposes calculated defender maximum roll");
        test.ExpectEqual(
            baseline.maximumHit,
            CombatFormulas::CalculateMaximumHit(attackerRatings),
            "Resolver exposes calculated maximum hit");
    }

    {
        Player attacker(3003);
        Player defender(3004);
        MeleeAttackResolver resolver;

        AdvanceSkillToLevel(
            attacker,
            SkillType::ATTACK,
            6);

        CombatRatings beforeSwordRatings = attacker.GetCombatRatings();

        attacker.GetEquipment().Equip(
            EquipmentSlotType::WEAPON,
            ItemType::BRONZE_SWORD);

        CombatRatings swordRatings = attacker.GetCombatRatings();

        HealthPoolCombatant beforeSwordHealth(defender.GetCombatRatings().maximumHealth);
        SequenceRandomSource beforeSequence({0, 0, 0});
        MeleeAttackResult beforeSwordResult = resolver.Resolve(
            beforeSwordRatings,
            defender.GetCombatRatings(),
            beforeSwordHealth,
            beforeSequence);

        HealthPoolCombatant swordHealth(defender.GetCombatRatings().maximumHealth);
        SequenceRandomSource swordSequence({0, 0, 0});
        MeleeAttackResult swordResult = resolver.Resolve(
            swordRatings,
            defender.GetCombatRatings(),
            swordHealth,
            swordSequence);

        test.Expect(
            swordResult.attackerMaximumRoll > beforeSwordResult.attackerMaximumRoll,
            "Bronze sword increases attacker maximum roll through existing ratings");
        test.Expect(
            swordResult.maximumHit >= beforeSwordResult.maximumHit,
            "Bronze sword can increase maximum hit at strength thresholds");
    }

    {
        Player attacker(3005);
        Player defender(3006);
        MeleeAttackResolver resolver;

        CombatRatings defenderBeforeShield = defender.GetCombatRatings();

        defender.GetEquipment().Equip(
            EquipmentSlotType::SHIELD,
            ItemType::WOODEN_SHIELD);
        defender.RefreshDerivedState();

        CombatRatings defenderWithShield = defender.GetCombatRatings();

        HealthPoolCombatant baseHealth(defenderBeforeShield.maximumHealth);
        SequenceRandomSource baseSequence({0, 0, 0});
        MeleeAttackResult baseResult = resolver.Resolve(
            attacker.GetCombatRatings(),
            defenderBeforeShield,
            baseHealth,
            baseSequence);

        HealthPoolCombatant shieldHealth(defenderWithShield.maximumHealth);
        SequenceRandomSource shieldSequence({0, 0, 0});
        MeleeAttackResult shieldResult = resolver.Resolve(
            attacker.GetCombatRatings(),
            defenderWithShield,
            shieldHealth,
            shieldSequence);

        test.Expect(
            shieldResult.defenderMaximumRoll > baseResult.defenderMaximumRoll,
            "Wooden shield increases defender maximum roll through existing ratings");
    }

    {
        Player attacker(3007);
        Player defender(3008);
        MeleeAttackResolver resolver;

        CombatRatings beforeAttackXp = attacker.GetCombatRatings();

        attacker.GetSkills().AddXP(
            SkillType::ATTACK,
            5000);

        CombatRatings afterAttackXp = attacker.GetCombatRatings();

        HealthPoolCombatant beforeHealth(defender.GetCombatRatings().maximumHealth);
        SequenceRandomSource beforeSequence({0, 0, 0});
        MeleeAttackResult beforeResult = resolver.Resolve(
            beforeAttackXp,
            defender.GetCombatRatings(),
            beforeHealth,
            beforeSequence);

        HealthPoolCombatant afterHealth(defender.GetCombatRatings().maximumHealth);
        SequenceRandomSource afterSequence({0, 0, 0});
        MeleeAttackResult afterResult = resolver.Resolve(
            afterAttackXp,
            defender.GetCombatRatings(),
            afterHealth,
            afterSequence);

        test.Expect(
            afterAttackXp.attackAccuracy > beforeAttackXp.attackAccuracy,
            "Higher Attack skill increases attack accuracy rating");
        test.Expect(
            afterAttackXp.meleeStrength > beforeAttackXp.meleeStrength,
            "Higher Attack skill increases melee strength rating");
        test.Expect(
            afterResult.attackerMaximumRoll > beforeResult.attackerMaximumRoll,
            "Higher Attack skill increases attacker maximum roll in resolver");
    }

    {
        Player attacker(3009);
        Player defender(3010);
        MeleeAttackResolver resolver;

        CombatRatings beforeDefenceXp = defender.GetCombatRatings();

        defender.GetSkills().AddXP(
            SkillType::DEFENCE,
            5000);

        CombatRatings afterDefenceXp = defender.GetCombatRatings();

        HealthPoolCombatant beforeHealth(afterDefenceXp.maximumHealth);
        SequenceRandomSource beforeSequence({0, 0, 0});
        MeleeAttackResult beforeResult = resolver.Resolve(
            attacker.GetCombatRatings(),
            beforeDefenceXp,
            beforeHealth,
            beforeSequence);

        HealthPoolCombatant afterHealth(afterDefenceXp.maximumHealth);
        SequenceRandomSource afterSequence({0, 0, 0});
        MeleeAttackResult afterResult = resolver.Resolve(
            attacker.GetCombatRatings(),
            afterDefenceXp,
            afterHealth,
            afterSequence);

        test.Expect(
            afterDefenceXp.defence > beforeDefenceXp.defence,
            "Higher Defence skill increases defence rating");
        test.Expect(
            afterResult.defenderMaximumRoll > beforeResult.defenderMaximumRoll,
            "Higher Defence skill increases defender maximum roll in resolver");
    }

    return test.Finish();
}

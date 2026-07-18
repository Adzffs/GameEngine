#include "TestSupport.h"

#include "../src/Combat/CombatFormulas.h"
#include "../src/Combat/CombatService.h"
#include "../src/Core/RandomSource.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/Stats/HealthPool.h"

#include <memory>
#include <stdexcept>
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

    bool AreEqual(
        const CombatRatings &left,
        const CombatRatings &right)
    {
        return left.attackAccuracy == right.attackAccuracy &&
               left.meleeStrength == right.meleeStrength &&
               left.defence == right.defence &&
               left.maximumHealth == right.maximumHealth;
    }

    bool AreEqual(
        const MeleeAttackResult &left,
        const MeleeAttackResult &right)
    {
        return left.attackerMaximumRoll == right.attackerMaximumRoll &&
               left.defenderMaximumRoll == right.defenderMaximumRoll &&
               left.attackerRolledResult == right.attackerRolledResult &&
               left.defenderRolledResult == right.defenderRolledResult &&
               left.maximumHit == right.maximumHit &&
               left.didHit == right.didHit &&
               left.rolledDamage == right.rolledDamage &&
               left.actualDamageApplied == right.actualDamageApplied;
    }
}

int main()
{
    TestContext test;

    {
        CombatService service(101);
        CombatRatings attacker = MakeRatings(12, 20, 5, 100);
        CombatRatings defender = MakeRatings(8, 10, 7, 100);
        HealthPoolCombatant health(40);

        MeleeAttackResult result = service.ResolveMeleeAttack(
            attacker,
            defender,
            health);

        test.ExpectEqual(
            result.attackerMaximumRoll,
            CombatFormulas::CalculateAttackRoll(attacker),
            "Explicit-seed construction resolves attacks through formulas");
    }

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{6, 2, 3});

        CombatService service(std::move(sequence));

        CombatRatings attacker = MakeRatings(10, 20, 5, 100);
        CombatRatings defender = MakeRatings(8, 10, 5, 100);
        HealthPoolCombatant health(20);

        MeleeAttackResult result = service.ResolveMeleeAttack(
            attacker,
            defender,
            health);

        test.Expect(
            result.didHit,
            "Injected fake RNG construction supports deterministic forced outcomes");
    }

    {
        bool threw = false;

        try
        {
            std::unique_ptr<RandomSource> nullRandomSource;
            CombatService service(std::move(nullRandomSource));
            (void)service;
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }

        test.Expect(
            threw,
            "Null RandomSource injection throws std::invalid_argument");
    }

    {
        CombatService first(2024);
        CombatService second(2024);

        CombatRatings attacker = MakeRatings(14, 27, 4, 100);
        CombatRatings defender = MakeRatings(11, 5, 13, 100);

        for (int index = 0; index < 12; ++index)
        {
            HealthPoolCombatant firstHealth(80);
            HealthPoolCombatant secondHealth(80);

            MeleeAttackResult left = first.ResolveMeleeAttack(
                attacker,
                defender,
                firstHealth);

            MeleeAttackResult right = second.ResolveMeleeAttack(
                attacker,
                defender,
                secondHealth);

            test.Expect(
                AreEqual(left, right),
                "Same seeds produce identical result sequences across attacks");
        }
    }

    {
        CombatService first(222);
        CombatService second(333);

        CombatRatings attacker = MakeRatings(15, 29, 6, 100);
        CombatRatings defender = MakeRatings(10, 7, 12, 100);

        bool diverged = false;

        for (int index = 0; index < 20; ++index)
        {
            HealthPoolCombatant firstHealth(80);
            HealthPoolCombatant secondHealth(80);

            MeleeAttackResult left = first.ResolveMeleeAttack(
                attacker,
                defender,
                firstHealth);

            MeleeAttackResult right = second.ResolveMeleeAttack(
                attacker,
                defender,
                secondHealth);

            if (!AreEqual(left, right))
            {
                diverged = true;
                break;
            }
        }

        test.Expect(
            diverged,
            "Different seeds normally diverge across attack sequences");
    }

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{5, 1, 1, 7, 0, 4});
        CombatService service(std::move(sequence));

        CombatRatings attacker = MakeRatings(10, 30, 4, 100);
        CombatRatings defender = MakeRatings(8, 5, 6, 100);

        HealthPoolCombatant firstHealth(100);
        HealthPoolCombatant secondHealth(100);

        MeleeAttackResult first = service.ResolveMeleeAttack(
            attacker,
            defender,
            firstHealth);
        MeleeAttackResult second = service.ResolveMeleeAttack(
            attacker,
            defender,
            secondHealth);

        test.Expect(
            first.rolledDamage != second.rolledDamage,
            "One service advances RNG state instead of repeating the first outcome");
    }

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{1, 2, 9});
        SequenceRandomSource *sequencePtr = sequence.get();

        CombatService service(std::move(sequence));

        CombatRatings attacker = MakeRatings(10, 20, 3, 100);
        CombatRatings defender = MakeRatings(8, 4, 10, 100);
        HealthPoolCombatant health(40);

        MeleeAttackResult result = service.ResolveMeleeAttack(
            attacker,
            defender,
            health);

        test.Expect(
            !result.didHit,
            "Forced miss via injected values resolves as miss");
        test.ExpectEqual(
            sequencePtr->GetCallCount(),
            2,
            "Forced miss consumes exactly two random values");
    }

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{6, 2, 4});
        SequenceRandomSource *sequencePtr = sequence.get();

        CombatService service(std::move(sequence));

        CombatRatings attacker = MakeRatings(12, 20, 5, 100);
        CombatRatings defender = MakeRatings(8, 7, 6, 100);
        HealthPoolCombatant health(20);

        MeleeAttackResult result = service.ResolveMeleeAttack(
            attacker,
            defender,
            health);

        int expectedDamage = CombatFormulas::ClampDamageRoll(
            4,
            CombatFormulas::CalculateMaximumHit(attacker));

        test.Expect(
            result.didHit,
            "Forced hit via injected values resolves as hit");
        test.ExpectEqual(
            sequencePtr->GetCallCount(),
            3,
            "Forced hit consumes exactly three random values");
        test.ExpectEqual(
            result.actualDamageApplied,
            expectedDamage,
            "Forced hit applies expected clamped damage");
    }

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{6, 2, 10});

        CombatService service(std::move(sequence));

        CombatRatings attacker = MakeRatings(12, 20, 5, 100);
        CombatRatings defender = MakeRatings(8, 7, 6, 100);
        HealthPoolCombatant health(3);

        MeleeAttackResult result = service.ResolveMeleeAttack(
            attacker,
            defender,
            health);

        test.ExpectEqual(
            result.actualDamageApplied,
            3,
            "Overkill reports only actual health removed");
    }

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{6, 2, 5});

        CombatService service(std::move(sequence));

        CombatRatings attacker = MakeRatings(12, 20, 5, 100);
        CombatRatings defender = MakeRatings(8, 7, 6, 100);
        HealthPoolCombatant health(1);
        health.ApplyDamage(1);

        MeleeAttackResult result = service.ResolveMeleeAttack(
            attacker,
            defender,
            health);

        test.ExpectEqual(
            result.actualDamageApplied,
            0,
            "Dead defender receives zero actual damage");
        test.ExpectEqual(
            health.GetCurrentHealth(),
            0,
            "Dead defender remains at zero health");
    }

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{6, 1, 2});

        CombatService service(std::move(sequence));

        CombatRatings attacker = MakeRatings(13, 20, 5, 100);
        CombatRatings defender = MakeRatings(9, 9, 7, 100);

        CombatRatings attackerBefore = attacker;
        CombatRatings defenderBefore = defender;

        HealthPoolCombatant defenderHealth(20);
        int defenderHealthBefore = defenderHealth.GetCurrentHealth();

        MeleeAttackResult result = service.ResolveMeleeAttack(
            attacker,
            defender,
            defenderHealth);

        test.Expect(
            AreEqual(attacker, attackerBefore),
            "Attacker ratings remain unchanged after resolution");
        test.Expect(
            AreEqual(defender, defenderBefore),
            "Defender ratings remain unchanged after resolution");
        test.ExpectEqual(
            defenderHealthBefore - defenderHealth.GetCurrentHealth(),
            result.actualDamageApplied,
            "Only defender health changes by actual applied damage");
        test.ExpectEqual(
            result.attackerMaximumRoll,
            CombatFormulas::CalculateAttackRoll(attacker),
            "Result exposes attacker maximum roll from formulas");
        test.ExpectEqual(
            result.defenderMaximumRoll,
            CombatFormulas::CalculateDefenceRoll(defender),
            "Result exposes defender maximum roll from formulas");
        test.ExpectEqual(
            result.maximumHit,
            CombatFormulas::CalculateMaximumHit(attacker),
            "Result exposes maximum hit from formulas");
    }

    return test.Finish();
}

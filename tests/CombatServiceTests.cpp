#include "TestSupport.h"

#include "../src/Action/Action.h"
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

    MeleeAttackResult EvaluateAndApply(
        CombatService &service,
        const CombatRatings &attacker,
        const CombatRatings &defender,
        Combatant &defenderCombatant)
    {
        MeleeAttackResult result =
            service.EvaluateMeleeAttack(
                attacker,
                defender);

        result.actualDamageApplied =
            service.ApplyMeleeDamage(
                result.rolledDamage,
                defenderCombatant);

        return result;
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

        MeleeAttackResult result = EvaluateAndApply(
            service,
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

        MeleeAttackResult result = EvaluateAndApply(
            service,
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

            MeleeAttackResult left = EvaluateAndApply(
                first,
                attacker,
                defender,
                firstHealth);

            MeleeAttackResult right = EvaluateAndApply(
                second,
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

            MeleeAttackResult left = EvaluateAndApply(
                first,
                attacker,
                defender,
                firstHealth);

            MeleeAttackResult right = EvaluateAndApply(
                second,
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

        MeleeAttackResult first = EvaluateAndApply(
            service,
            attacker,
            defender,
            firstHealth);
        MeleeAttackResult second = EvaluateAndApply(
            service,
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

        MeleeAttackResult result = EvaluateAndApply(
            service,
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

        MeleeAttackResult result = EvaluateAndApply(
            service,
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

        MeleeAttackResult result = EvaluateAndApply(
            service,
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

        MeleeAttackResult result = EvaluateAndApply(
            service,
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

        MeleeAttackResult result = EvaluateAndApply(
            service,
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

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{6, 1, 0});
        SequenceRandomSource *sequencePtr = sequence.get();

        CombatService service(std::move(sequence));
        CombatRatings attacker = MakeRatings(12, 20, 5, 100);
        CombatRatings defender = MakeRatings(8, 7, 6, 100);

        MeleeAttackResult result =
            service.EvaluateMeleeAttack(
                attacker,
                defender);

        test.Expect(
            result.didHit,
            "Zero-damage hit still resolves as a hit");
        test.ExpectEqual(
            result.rolledDamage,
            0,
            "Zero-damage hit preserves rolled damage");
        test.ExpectEqual(
            sequencePtr->GetCallCount(),
            3,
            "Zero-damage hit consumes exactly three random values");
    }

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{9, 1, 1});
        SequenceRandomSource *sequencePtr = sequence.get();

        CombatService service(std::move(sequence));

        Action action(
            ActionType::RECIPE,
            "Recipe",
            1,
            100,
            200,
            false);

        bool validatorCalled = false;
        bool providerCalled = false;

        MeleeCompletionOutcome outcome =
            service.EvaluateCompletedMeleeAction(
                action,
                [&](int, int)
                {
                    validatorCalled = true;
                    return ActionValidationResult{
                        true,
                        ActionCancelReason::NONE,
                        ""};
                },
                [&](int, int)
                    -> std::optional<MeleeCombatRatingsSnapshot>
                {
                    providerCalled = true;
                    return std::nullopt;
                });

        test.ExpectEqual(
            static_cast<int>(outcome.type),
            static_cast<int>(MeleeCompletionOutcomeType::IGNORE),
            "Non-melee completed actions return IGNORE");
        test.Expect(
            !validatorCalled && !providerCalled,
            "IGNORE outcome does not invoke completion validation or ratings lookup");
        test.ExpectEqual(
            sequencePtr->GetCallCount(),
            0,
            "IGNORE outcome consumes zero combat RNG calls");
    }

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{9, 1, 1});
        SequenceRandomSource *sequencePtr = sequence.get();

        CombatService service(std::move(sequence));

        Action action(
            ActionType::MELEE_ATTACK,
            "Melee attack",
            1,
            401,
            402,
            false);

        MeleeCompletionOutcome outcome =
            service.EvaluateCompletedMeleeAction(
                action,
                [](int, int)
                {
                    return ActionValidationResult{
                        true,
                        ActionCancelReason::NONE,
                        ""};
                },
                [](int, int)
                    -> std::optional<MeleeCombatRatingsSnapshot>
                {
                    return std::nullopt;
                });

        test.ExpectEqual(
            static_cast<int>(outcome.type),
            static_cast<int>(MeleeCompletionOutcomeType::CLEAR_STALE),
            "Missing completion participants return CLEAR_STALE");
        test.ExpectEqual(
            sequencePtr->GetCallCount(),
            0,
            "CLEAR_STALE completion consumes zero combat RNG calls");
    }

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{9, 1, 1});
        SequenceRandomSource *sequencePtr = sequence.get();

        CombatService service(std::move(sequence));

        Action action(
            ActionType::MELEE_ATTACK,
            "Melee attack",
            1,
            411,
            412,
            false);

        MeleeCompletionOutcome outcome =
            service.EvaluateCompletedMeleeAction(
                action,
                [](int, int)
                {
                    return ActionValidationResult{
                        true,
                        ActionCancelReason::NONE,
                        ""};
                },
                [](int attackerEntityID, int)
                    -> std::optional<MeleeCombatRatingsSnapshot>
                {
                    if (attackerEntityID == 411)
                    {
                        return std::nullopt;
                    }

                    return MeleeCombatRatingsSnapshot{
                        MakeRatings(10, 10, 10, 100),
                        MakeRatings(10, 10, 10, 100)};
                });

        test.ExpectEqual(
            static_cast<int>(outcome.type),
            static_cast<int>(MeleeCompletionOutcomeType::CLEAR_STALE),
            "Missing attacker completion returns CLEAR_STALE");
        test.ExpectEqual(
            sequencePtr->GetCallCount(),
            0,
            "Missing attacker completion consumes zero combat RNG calls");
    }

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{9, 1, 1});
        SequenceRandomSource *sequencePtr = sequence.get();

        CombatService service(std::move(sequence));

        Action action(
            ActionType::MELEE_ATTACK,
            "Melee attack",
            1,
            421,
            422,
            false);

        MeleeCompletionOutcome outcome =
            service.EvaluateCompletedMeleeAction(
                action,
                [](int, int)
                {
                    return ActionValidationResult{
                        true,
                        ActionCancelReason::NONE,
                        ""};
                },
                [](int, int defenderEntityID)
                    -> std::optional<MeleeCombatRatingsSnapshot>
                {
                    if (defenderEntityID == 422)
                    {
                        return std::nullopt;
                    }

                    return MeleeCombatRatingsSnapshot{
                        MakeRatings(10, 10, 10, 100),
                        MakeRatings(10, 10, 10, 100)};
                });

        test.ExpectEqual(
            static_cast<int>(outcome.type),
            static_cast<int>(MeleeCompletionOutcomeType::CLEAR_STALE),
            "Missing target completion returns CLEAR_STALE");
        test.ExpectEqual(
            sequencePtr->GetCallCount(),
            0,
            "Missing target completion consumes zero combat RNG calls");
    }

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{9, 1, 1});
        SequenceRandomSource *sequencePtr = sequence.get();

        CombatService service(std::move(sequence));

        Action action(
            ActionType::MELEE_ATTACK,
            "Melee attack",
            1,
            501,
            502,
            false);

        bool providerCalled = false;

        MeleeCompletionOutcome outcome =
            service.EvaluateCompletedMeleeAction(
                action,
                [](int, int)
                {
                    return ActionValidationResult{
                        false,
                        ActionCancelReason::ENTITY_DIED,
                        "Attacker is not alive"};
                },
                [&](int, int)
                    -> std::optional<MeleeCombatRatingsSnapshot>
                {
                    providerCalled = true;
                    return std::nullopt;
                });

        test.ExpectEqual(
            static_cast<int>(outcome.type),
            static_cast<int>(MeleeCompletionOutcomeType::CANCEL),
            "Dead attacker completion returns CANCEL");
        test.ExpectEqual(
            static_cast<int>(outcome.cancelReason),
            static_cast<int>(ActionCancelReason::ENTITY_DIED),
            "Dead attacker completion preserves cancellation reason");
        test.Expect(
            !providerCalled,
            "CANCEL completion skips stale ratings lookup");
        test.ExpectEqual(
            sequencePtr->GetCallCount(),
            0,
            "Dead attacker completion consumes zero combat RNG calls");
    }

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{9, 1, 1});
        SequenceRandomSource *sequencePtr = sequence.get();

        CombatService service(std::move(sequence));

        Action action(
            ActionType::MELEE_ATTACK,
            "Melee attack",
            1,
            601,
            602,
            false);

        MeleeCompletionOutcome outcome =
            service.EvaluateCompletedMeleeAction(
                action,
                [](int, int)
                {
                    return ActionValidationResult{
                        false,
                        ActionCancelReason::ENTITY_DIED,
                        "Defender is not alive"};
                },
                [](int, int)
                    -> std::optional<MeleeCombatRatingsSnapshot>
                {
                    return std::nullopt;
                });

        test.ExpectEqual(
            static_cast<int>(outcome.type),
            static_cast<int>(MeleeCompletionOutcomeType::CANCEL),
            "Dead target completion returns CANCEL");
        test.ExpectEqual(
            sequencePtr->GetCallCount(),
            0,
            "Dead target completion consumes zero combat RNG calls");
    }

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{9, 1, 1});
        SequenceRandomSource *sequencePtr = sequence.get();

        CombatService service(std::move(sequence));

        Action action(
            ActionType::MELEE_ATTACK,
            "Melee attack",
            1,
            701,
            702,
            false);

        MeleeCompletionOutcome outcome =
            service.EvaluateCompletedMeleeAction(
                action,
                [](int, int)
                {
                    return ActionValidationResult{
                        false,
                        ActionCancelReason::OUT_OF_RANGE,
                        "You are too far away from the defender"};
                },
                [](int, int)
                    -> std::optional<MeleeCombatRatingsSnapshot>
                {
                    return std::nullopt;
                });

        test.ExpectEqual(
            static_cast<int>(outcome.type),
            static_cast<int>(MeleeCompletionOutcomeType::CANCEL),
            "Out-of-range completion returns CANCEL");
        test.ExpectEqual(
            sequencePtr->GetCallCount(),
            0,
            "Out-of-range completion consumes zero combat RNG calls");
    }

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{9, 1, 1});
        SequenceRandomSource *sequencePtr = sequence.get();

        CombatService service(std::move(sequence));

        Action action(
            ActionType::MELEE_ATTACK,
            "Melee attack",
            1,
            711,
            712,
            false);

        MeleeCompletionOutcome outcome =
            service.EvaluateCompletedMeleeAction(
                action,
                [](int, int)
                {
                    return ActionValidationResult{
                        false,
                        ActionCancelReason::OUT_OF_RANGE,
                        "The target moved out of range"};
                },
                [](int, int)
                    -> std::optional<MeleeCombatRatingsSnapshot>
                {
                    return std::nullopt;
                });

        test.ExpectEqual(
            static_cast<int>(outcome.type),
            static_cast<int>(MeleeCompletionOutcomeType::CANCEL),
            "Out-of-range target completion returns CANCEL");
        test.ExpectEqual(
            sequencePtr->GetCallCount(),
            0,
            "Out-of-range target completion consumes zero combat RNG calls");
    }

    {
        auto sequence = std::make_unique<SequenceRandomSource>(
            std::vector<int>{6, 1, 2});

        CombatService service(std::move(sequence));

        Action action(
            ActionType::MELEE_ATTACK,
            "Melee attack",
            1,
            801,
            802,
            false);

        HealthPoolCombatant defenderHealth(20);
        const int healthBefore =
            defenderHealth.GetCurrentHealth();

        MeleeCompletionOutcome outcome =
            service.EvaluateCompletedMeleeAction(
                action,
                [](int, int)
                {
                    return ActionValidationResult{
                        true,
                        ActionCancelReason::NONE,
                        ""};
                },
                [&](int, int)
                    -> std::optional<MeleeCombatRatingsSnapshot>
                {
                    return MeleeCombatRatingsSnapshot{
                        MakeRatings(12, 20, 5, 100),
                        MakeRatings(8, 7, 6, 100)};
                });

        test.ExpectEqual(
            static_cast<int>(outcome.type),
            static_cast<int>(MeleeCompletionOutcomeType::RESOLVE),
            "Valid completion returns RESOLVE");
        test.ExpectEqual(
            outcome.attackerEntityID,
            801,
            "RESOLVE completion preserves attacker ID value data");
        test.ExpectEqual(
            outcome.defenderEntityID,
            802,
            "RESOLVE completion preserves defender ID value data");
        test.ExpectEqual(
            outcome.ratingsSnapshot.has_value(),
            true,
            "RESOLVE completion carries value-only ratings snapshot");
        test.ExpectEqual(
            defenderHealth.GetCurrentHealth(),
            healthBefore,
            "Completion evaluation does not alter defender health");
    }

    return test.Finish();
}

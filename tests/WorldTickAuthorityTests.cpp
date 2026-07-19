#include "TestSupport.h"

#include "../src/Action/Action.h"
#include "../src/Action/ActionCancelReason.h"
#include "../src/Action/ActionType.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Player/Player.h"
#include "../src/Reward/RewardTableType.h"
#include "../src/Stats/CombatRatings.h"
#include "../src/StatusEffect/StatusEffectDefinition.h"
#include "../src/StatusEffect/StatusEffectType.h"
#include "../src/World/Event/ActionCancelledEvent.h"
#include "../src/World/Event/ActionCompletedEvent.h"
#include "../src/World/Event/ActionStartedEvent.h"
#include "../src/World/World.h"

#include <optional>
#include <variant>

namespace
{
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

    template <typename EventType>
    const EventType *FindFirstEvent(
        const std::vector<ActionLifecycleEvent> &events)
    {
        for (const ActionLifecycleEvent &event : events)
        {
            const EventType *typedEvent =
                std::get_if<EventType>(
                    &event.data);

            if (typedEvent != nullptr)
            {
                return typedEvent;
            }
        }

        return nullptr;
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

    StatusEffectDefinition MakeStatusEffect(
        StatusEffectType type,
        int durationTicks,
        int accuracyBonus)
    {
        return StatusEffectDefinition{
            type,
            durationTicks,
            StatusEffectModifiers{
                accuracyBonus,
                0,
                0,
                0}};
    }
}

int main()
{
    TestContext test;

    {
        World world;

        test.ExpectEqual(
            world.GetCurrentTick(),
            0,
            "World begins at tick zero");

        world.Update();

        test.ExpectEqual(
            world.GetCurrentTick(),
            1,
            "One update advances the authoritative tick exactly once");
    }

    {
        World world;

        AdvanceTicks(world, 10);

        test.ExpectEqual(
            world.GetCurrentTick(),
            10,
            "Ten updates advance the authoritative tick by ten");
    }

    {
        World world;

        world.Update();

        test.ExpectEqual(
            world.GetCurrentTick(),
            1,
            "An empty update does not advance more than once");
    }

    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        test.Expect(
            world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                3),
            "Pre-update melee start succeeds");
        test.ExpectEqual(
            world.GetCurrentTick(),
            0,
            "Starting an action does not advance the world tick");

        const Action *startedAction =
            world.GetActionForEntity(attackerID);

        test.Expect(
            startedAction != nullptr,
            "Started action is available for authoritative tick checks");

        if (startedAction != nullptr)
        {
            test.ExpectEqual(
                startedAction->GetStartTick(),
                world.GetCurrentTick(),
                "Action start tick matches the authoritative world tick");
            test.ExpectEqual(
                startedAction->GetCompletionTick(),
                world.GetCurrentTick() + 3,
                "Action completion tick is derived from the authoritative world tick and duration");
        }
    }

    {
        World world;

        test.Expect(
            !world.TryStartMeleeAttack(
                999999,
                1,
                3),
            "Invalid action start fails cleanly");
        test.ExpectEqual(
            world.GetCurrentTick(),
            0,
            "Failed action start does not advance the world tick");
    }

    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeAttack(
            attackerID,
            defenderID,
            2);

        const Action *pendingAction =
            world.GetActionForEntity(attackerID);

        test.Expect(
            pendingAction != nullptr,
            "Action exists before completion timing validation");

        int expectedCompletionTick = -1;
        if (pendingAction != nullptr)
        {
            expectedCompletionTick =
                pendingAction->GetCompletionTick();
        }

        world.Update();
        world.Update();

        const ActionCompletedEvent *completed =
            FindFirstEvent<ActionCompletedEvent>(
                world.GetActionLifecycleEvents());

        test.Expect(
            completed != nullptr,
            "Completed event is published on the authoritative completion tick");

        if (completed != nullptr)
        {
            test.ExpectEqual(
                completed->completionTick,
                expectedCompletionTick,
                "Completion event tick matches the action snapshot");
        }
    }

    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeAttack(
            attackerID,
            defenderID,
            2);

        const Action *startedAction =
            world.GetActionForEntity(attackerID);

        test.Expect(
            startedAction != nullptr,
            "Action exists before lifecycle start publication");

        int expectedStartTick = -1;
        int expectedCompletionTick = -1;
        if (startedAction != nullptr)
        {
            expectedStartTick = startedAction->GetStartTick();
            expectedCompletionTick = startedAction->GetCompletionTick();
        }

        world.Update();

        const ActionStartedEvent *started =
            FindFirstEvent<ActionStartedEvent>(
                world.GetActionLifecycleEvents());

        test.Expect(
            started != nullptr,
            "Started lifecycle event is published");

        if (started != nullptr)
        {
            test.ExpectEqual(
                started->startTick,
                expectedStartTick,
                "Started event start tick matches the action snapshot");
            test.ExpectEqual(
                started->completionTick,
                expectedCompletionTick,
                "Started event completion tick matches the action snapshot");
        }
    }

    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeAttack(
            attackerID,
            defenderID,
            3);
        world.Update();

        int cancellationTick = world.GetCurrentTick();

        world.CancelActionsForEntity(
            attackerID,
            ActionCancelReason::INTERFACE_CLOSED);
        world.Update();

        const ActionCancelledEvent *cancelled =
            FindFirstEvent<ActionCancelledEvent>(
                world.GetActionLifecycleEvents());

        test.Expect(
            cancelled != nullptr,
            "Cancellation lifecycle event is published");

        if (cancelled != nullptr)
        {
            test.ExpectEqual(
                cancelled->cancellationTick,
                cancellationTick,
                "Cancellation event tick matches the authoritative world tick at cancel time");
        }
    }

    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeEngagement(
            attackerID,
            defenderID,
            1);
        world.Update();

        const Action *restartedAction =
            world.GetActionForEntity(attackerID);

        test.Expect(
            restartedAction != nullptr,
            "Repeating action remains active after authoritative restart");

        if (restartedAction != nullptr)
        {
            test.ExpectEqual(
                restartedAction->GetStartTick(),
                world.GetCurrentTick(),
                "Repeating restart uses the current authoritative world tick");
            test.ExpectEqual(
                restartedAction->GetCompletionTick(),
                world.GetCurrentTick() + 1,
                "Repeating restart computes completion from the authoritative world tick");
        }
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            world.TryApplyStatusEffect(
                playerID,
                MakeStatusEffect(
                    StatusEffectType::COMBAT_BOOST,
                    2,
                    4)),
            "Status effect applies for authoritative expiry timing test");

        const ActiveStatusEffect *applied =
            player->GetStatusEffectManager().FindEffect(
                StatusEffectType::COMBAT_BOOST);

        test.Expect(
            applied != nullptr,
            "Applied status effect is present before updates");

        world.Update();

        const ActiveStatusEffect *afterFirstTick =
            player->GetStatusEffectManager().FindEffect(
                StatusEffectType::COMBAT_BOOST);

        test.Expect(
            afterFirstTick != nullptr,
            "Status effect remains active after one authoritative tick");

        if (afterFirstTick != nullptr)
        {
            test.ExpectEqual(
                afterFirstTick->remainingTicks,
                1,
                "Status effect decrements exactly once per authoritative world tick");
        }

        world.Update();

        test.Expect(
            !player->GetStatusEffectManager().HasEffect(
                StatusEffectType::COMBAT_BOOST),
            "Status effect expires on the same authoritative tick as before");
    }

    {
        World world;
        int monsterID = world.CreateMonster(
            20,
            20,
            MakeRatings(1, 1, 1, 2),
            RewardTableType::NONE,
            MonsterRespawnDefinition{3});

        Monster *monster = GetMonster(world, monsterID);

        monster->ApplyDamage(monster->GetCurrentHealth());
        world.Update();

        test.Expect(
            world.HasScheduledMonsterRespawn(monsterID),
            "Monster death schedules respawn using authoritative world tick");

        int respawnTick =
            world.GetScheduledMonsterRespawnTick(monsterID).value_or(-1);

        test.ExpectEqual(
            respawnTick,
            world.GetCurrentTick() + 3,
            "Monster respawn delay remains derived from the authoritative world tick plus delay");

        world.Update();
        world.Update();

        test.Expect(
            !monster->IsAlive(),
            "Monster does not respawn early under authoritative world tick ownership");

        world.Update();

        test.Expect(
            monster->IsAlive(),
            "Monster respawns on the same authoritative due tick as before");
    }

    return test.Finish();
}

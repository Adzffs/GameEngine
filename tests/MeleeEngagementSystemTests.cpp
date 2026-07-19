#include "TestSupport.h"

#include "../src/Combat/MeleeEngagementSystem.h"
#include "../src/Entity/Manager/EntityManager.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Player/Player.h"

struct EntityManagerTestAccess
{
    static bool RemoveEntity(EntityManager &manager, int entityID)
    {
        return manager.RemoveEntity(entityID);
    }
};

namespace
{
    void TestState(TestContext &test)
    {
        EntityManager entities;
        MeleeEngagementSystem system;
        int firstPlayerID = entities.CreatePlayer();
        int secondPlayerID = entities.CreatePlayer();
        int firstMonsterID = entities.CreateMonster(
            5, 5, CombatRatings{8, 8, 8, 40});
        int secondMonsterID = entities.CreateMonster(
            7, 5, CombatRatings{8, 8, 8, 40});
        int npcID = entities.CreateNPC(3, 3);

        test.ExpectEqual(system.GetEngagementCount(), std::size_t{0},
                         "System starts empty");
        test.Expect(system.RequestEngagement(
                        firstPlayerID, firstMonsterID, 4, entities),
                    "Valid engagement is accepted");
        test.Expect(system.HasEngagement(firstPlayerID),
                    "Accepted engagement is observable");
        test.Expect(!system.RequestEngagement(999, firstMonsterID, 4, entities),
                    "Unknown attacker is rejected");
        test.Expect(!system.RequestEngagement(firstPlayerID, 999, 4, entities),
                    "Unknown target is rejected");
        test.Expect(!system.RequestEngagement(
                        firstPlayerID, firstPlayerID, 4, entities),
                    "Self-targeting is rejected");
        test.Expect(!system.RequestEngagement(npcID, firstMonsterID, 4, entities) &&
                        !system.RequestEngagement(firstPlayerID, npcID, 4, entities),
                    "Unsupported participant types are rejected");

        test.Expect(system.RequestEngagement(
                        firstPlayerID, firstMonsterID, 4, entities),
                    "Same target request succeeds without duplication");
        test.ExpectEqual(system.GetEngagementCount(), std::size_t{1},
                         "Same target does not duplicate state");
        test.Expect(system.RequestEngagement(
                        firstPlayerID, secondMonsterID, 6, entities),
                    "A new target replaces the previous target");
        auto target = system.GetTargetEntityID(firstPlayerID);
        test.Expect(target.has_value() && target.value() == secondMonsterID,
                    "Read-only query returns the replacement target ID");
        test.ExpectEqual(system.GetEngagementCount(), std::size_t{1},
                         "Replacement retains one attacker state");

        test.Expect(system.ClearEngagement(firstPlayerID),
                    "Known engagement clears successfully");
        test.Expect(!system.ClearEngagement(firstPlayerID),
                    "Unknown engagement clears safely");

        system.RequestEngagement(firstPlayerID, firstMonsterID, 4, entities);
        system.RequestEngagement(secondPlayerID, firstMonsterID, 4, entities);
        system.RequestEngagement(secondMonsterID, secondPlayerID, 4, entities);
        test.ExpectEqual(
            system.ClearEngagementsInvolving(firstMonsterID),
            std::size_t{2},
            "Clearing a target removes every engagement involving it");
        test.ExpectEqual(system.GetEngagementCount(), std::size_t{1},
                         "Unrelated engagement remains after involved cleanup");

        EntityManager deadEntities;
        int deadAttackerID = deadEntities.CreatePlayer();
        int liveMonsterID = deadEntities.CreateMonster(
            1, 0, CombatRatings{8, 8, 8, 40});
        deadEntities.GetEntityByID(deadAttackerID);
        dynamic_cast<Player *>(deadEntities.GetEntityByID(deadAttackerID))
            ->ApplyDamage(10000);
        test.Expect(!system.RequestEngagement(
                        deadAttackerID, liveMonsterID, 4, deadEntities),
                    "Dead attacker is rejected");
        dynamic_cast<Monster *>(deadEntities.GetEntityByID(liveMonsterID))
            ->ApplyDamage(10000);
        int livePlayerID = deadEntities.CreatePlayer();
        test.Expect(!system.RequestEngagement(
                        livePlayerID, liveMonsterID, 4, deadEntities),
                    "Dead target is rejected");
    }

    void TestDecisions(TestContext &test)
    {
        EntityManager entities;
        MeleeEngagementSystem system;
        int firstID = entities.CreatePlayer();
        int secondID = entities.CreatePlayer();
        int thirdID = entities.CreateMonster(
            10, 10, CombatRatings{8, 8, 8, 40});

        entities.GetEntityByID(firstID)->GetPosition().SetPosition(2, 2);
        entities.GetEntityByID(secondID)->GetPosition().SetPosition(4, 4);
        system.RequestEngagement(secondID, thirdID, 4, entities);
        system.RequestEngagement(firstID, thirdID, 4, entities);

        auto distant = system.Evaluate(entities);
        test.Expect(distant.size() == 2 &&
                        distant[0].attackerEntityID == firstID &&
                        distant[1].attackerEntityID == secondID,
                    "Multiple attackers evaluate in creation order, not hash order");
        test.Expect(distant[0].type ==
                        MeleeEngagementIntentType::APPROACH_TARGET &&
                        distant[1].type ==
                            MeleeEngagementIntentType::APPROACH_TARGET,
                    "Distant targets produce approach intentions");

        entities.GetEntityByID(firstID)->GetPosition().SetPosition(9, 9);
        auto diagonal = system.Evaluate(entities);
        test.Expect(diagonal.size() == 2 &&
                        diagonal[0].type ==
                            MeleeEngagementIntentType::ATTACK_TARGET,
                    "Diagonal adjacency preserves the existing melee range");

        entities.GetEntityByID(firstID)->GetPosition().SetPosition(7, 7);
        auto movedOut = system.Evaluate(entities);
        test.Expect(movedOut[0].type ==
                        MeleeEngagementIntentType::APPROACH_TARGET,
                    "Moving out of range changes attack to approach");
        entities.GetEntityByID(secondID)->GetPosition().SetPosition(9, 10);
        auto movedIn = system.Evaluate(entities);
        test.Expect(movedIn[1].type ==
                        MeleeEngagementIntentType::ATTACK_TARGET,
                    "Moving into range changes approach to attack");

        dynamic_cast<Monster *>(entities.GetEntityByID(thirdID))
            ->ApplyDamage(10000);
        auto deadTarget = system.Evaluate(entities);
        test.Expect(deadTarget.size() == 2 &&
                        deadTarget[0].type ==
                            MeleeEngagementIntentType::CLEAR_ENGAGEMENT &&
                        deadTarget[0].clearReason ==
                            MeleeEngagementClearReason::TARGET_DEAD,
                    "Dead target deterministically clears each engagement");
        test.ExpectEqual(system.GetEngagementCount(), std::size_t{0},
                         "Dead-target evaluation removes stale state");
    }

    void TestMissingParticipants(TestContext &test)
    {
        EntityManager entities;
        MeleeEngagementSystem system;
        int attackerID = entities.CreatePlayer();
        int targetID = entities.CreateMonster(
            1, 0, CombatRatings{8, 8, 8, 40});
        system.RequestEngagement(attackerID, targetID, 4, entities);
        EntityManagerTestAccess::RemoveEntity(entities, targetID);

        auto missingTarget = system.Evaluate(entities);
        test.Expect(missingTarget.size() == 1 &&
                        missingTarget[0].type ==
                            MeleeEngagementIntentType::CLEAR_ENGAGEMENT &&
                        missingTarget[0].clearReason ==
                            MeleeEngagementClearReason::INVALID_TARGET,
                    "Missing target emits deterministic clear intent");

        EntityManager missingAttackerEntities;
        MeleeEngagementSystem missingAttackerSystem;
        int missingAttackerID = missingAttackerEntities.CreatePlayer();
        int validTargetID = missingAttackerEntities.CreateMonster(
            1, 0, CombatRatings{8, 8, 8, 40});
        missingAttackerSystem.RequestEngagement(
            missingAttackerID, validTargetID, 4, missingAttackerEntities);
        EntityManagerTestAccess::RemoveEntity(
            missingAttackerEntities, missingAttackerID);
        auto missingAttacker =
            missingAttackerSystem.Evaluate(missingAttackerEntities);
        test.Expect(missingAttacker.empty() &&
                        missingAttackerSystem.GetEngagementCount() == 0,
                    "Missing attacker state is removed safely without unordered output");
    }
}

int main()
{
    TestContext test;
    TestState(test);
    TestDecisions(test);
    TestMissingParticipants(test);
    return test.Finish();
}

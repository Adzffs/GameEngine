#include "TestSupport.h"

#include "../src/Interaction/InteractionSystem.h"
#include "../src/Entity/Manager/EntityManager.h"
#include "../src/Player/Player.h"
#include "../src/World/Object/Manager/ObjectManager.h"

struct InteractionSystemTestAccess
{
    static void Insert(InteractionSystem &system, PendingInteraction pending)
    {
        system.pendingInteractionsByActorID.insert_or_assign(
            pending.actorEntityID, pending);
    }
};

namespace
{
    struct Fixture
    {
        EntityManager entities;
        ObjectManager objects;
        InteractionSystem system;
        int firstPlayerID = entities.CreatePlayer();
        int secondPlayerID = entities.CreatePlayer();
        int npcID = entities.CreateNPC(4, 4);

        Fixture()
        {
            objects.CreateResource(ResourceType::NORMAL_TREE, 5, 5);
            objects.CreateStation(StationType::FURNACE, 9, 9);
        }

        int ResourceID() const { return objects.GetResources().front().GetID(); }
        int StationID() const { return objects.GetStations().front().GetID(); }
    };

    void TestState(TestContext &test)
    {
        Fixture f;
        test.ExpectEqual(f.system.GetInteractionCount(), std::size_t{0},
                         "System starts empty");
        test.Expect(f.system.RequestInteraction(
                        f.firstPlayerID, f.ResourceID(),
                        InteractionTargetType::RESOURCE, f.entities, f.objects),
                    "Valid resource request succeeds");
        test.Expect(f.system.HasInteraction(f.firstPlayerID),
                    "Valid request is observable");
        test.Expect(!f.system.RequestInteraction(
                        999, f.ResourceID(), InteractionTargetType::RESOURCE,
                        f.entities, f.objects),
                    "Unknown actor is rejected");
        test.Expect(!f.system.RequestInteraction(
                        f.npcID, f.ResourceID(), InteractionTargetType::RESOURCE,
                        f.entities, f.objects),
                    "Non-player actor is rejected");
        test.Expect(!f.system.RequestInteraction(
                        f.firstPlayerID, 999, InteractionTargetType::RESOURCE,
                        f.entities, f.objects),
                    "Unknown target is rejected");
        test.Expect(!f.system.RequestInteraction(
                        f.firstPlayerID, f.ResourceID(),
                        InteractionTargetType::STATION, f.entities, f.objects),
                    "Resource declared as station is rejected");

        test.Expect(f.system.RequestInteraction(
                        f.firstPlayerID, f.ResourceID(),
                        InteractionTargetType::RESOURCE, f.entities, f.objects),
                    "Same interaction can be requested again");
        test.ExpectEqual(f.system.GetInteractionCount(), std::size_t{1},
                         "Same interaction does not duplicate state");
        test.Expect(f.system.RequestInteraction(
                        f.firstPlayerID, f.StationID(),
                        InteractionTargetType::STATION, f.entities, f.objects),
                    "Resource-to-station replacement succeeds");
        const auto pending = f.system.GetInteraction(f.firstPlayerID);
        test.Expect(pending.has_value() &&
                        pending->targetObjectID == f.StationID() &&
                        pending->targetType == InteractionTargetType::STATION,
                    "Read-only query returns exact replacement state");
        test.ExpectEqual(f.system.GetInteractionCount(), std::size_t{1},
                         "Replacement retains one state per actor");
        test.Expect(f.system.ClearInteraction(f.firstPlayerID),
                    "Known interaction clears");
        test.Expect(!f.system.ClearInteraction(f.firstPlayerID),
                    "Unknown interaction clears safely");

        f.system.RequestInteraction(f.firstPlayerID, f.ResourceID(),
                                    InteractionTargetType::RESOURCE,
                                    f.entities, f.objects);
        f.system.RequestInteraction(f.secondPlayerID, f.ResourceID(),
                                    InteractionTargetType::RESOURCE,
                                    f.entities, f.objects);
        test.ExpectEqual(f.system.ClearInteractionsTargeting(f.ResourceID()),
                         std::size_t{2},
                         "Target cleanup clears every matching actor");

        Fixture dead;
        dynamic_cast<Player *>(dead.entities.GetEntityByID(dead.firstPlayerID))
            ->ApplyDamage(10000);
        test.Expect(!dead.system.RequestInteraction(
                        dead.firstPlayerID, dead.ResourceID(),
                        InteractionTargetType::RESOURCE,
                        dead.entities, dead.objects),
                    "Dead actor is rejected");
    }

    void TestEvaluation(TestContext &test)
    {
        Fixture f;
        f.entities.GetEntityByID(f.firstPlayerID)->GetPosition().SetPosition(0, 0);
        f.system.RequestInteraction(f.firstPlayerID, f.ResourceID(),
                                    InteractionTargetType::RESOURCE,
                                    f.entities, f.objects);
        auto waiting = f.system.Evaluate(
            f.entities, f.objects, {f.firstPlayerID});
        test.Expect(waiting.empty() && f.system.HasInteraction(f.firstPlayerID),
                    "Out-of-range moving actor remains pending");
        auto failed = f.system.Evaluate(f.entities, f.objects, {});
        test.Expect(failed.size() == 1 &&
                        failed[0].type ==
                            InteractionIntentType::CLEAR_INTERACTION &&
                        failed[0].clearReason ==
                            InteractionClearReason::APPROACH_FAILED,
                    "Ended approach emits approach-failed clear");

        f.entities.GetEntityByID(f.firstPlayerID)->GetPosition().SetPosition(4, 4);
        f.system.RequestInteraction(f.firstPlayerID, f.ResourceID(),
                                    InteractionTargetType::RESOURCE,
                                    f.entities, f.objects);
        auto resourceReady = f.system.Evaluate(f.entities, f.objects, {});
        test.Expect(resourceReady.size() == 1 &&
                        resourceReady[0].type == InteractionIntentType::READY &&
                        resourceReady[0].targetType ==
                            InteractionTargetType::RESOURCE,
                    "Diagonal resource adjacency is ready");

        f.entities.GetEntityByID(f.firstPlayerID)->GetPosition().SetPosition(8, 9);
        f.system.RequestInteraction(f.firstPlayerID, f.StationID(),
                                    InteractionTargetType::STATION,
                                    f.entities, f.objects);
        auto stationReady = f.system.Evaluate(f.entities, f.objects, {});
        test.Expect(stationReady.size() == 1 &&
                        stationReady[0].type == InteractionIntentType::READY,
                    "Cardinal station adjacency is ready");

        Fixture ordered;
        ordered.entities.GetEntityByID(ordered.firstPlayerID)
            ->GetPosition().SetPosition(4, 5);
        ordered.entities.GetEntityByID(ordered.secondPlayerID)
            ->GetPosition().SetPosition(5, 4);
        ordered.system.RequestInteraction(
            ordered.secondPlayerID, ordered.ResourceID(),
            InteractionTargetType::RESOURCE, ordered.entities, ordered.objects);
        ordered.system.RequestInteraction(
            ordered.firstPlayerID, ordered.ResourceID(),
            InteractionTargetType::RESOURCE, ordered.entities, ordered.objects);
        auto intents = ordered.system.Evaluate(ordered.entities, ordered.objects, {});
        test.Expect(intents.size() == 2 &&
                        intents[0].actorEntityID == ordered.firstPlayerID &&
                        intents[1].actorEntityID == ordered.secondPlayerID,
                    "Actor creation order controls intent order");

        Fixture invalid;
        InteractionSystemTestAccess::Insert(
            invalid.system,
            {invalid.npcID, invalid.ResourceID(),
             InteractionTargetType::RESOURCE});
        auto invalidActor = invalid.system.Evaluate(
            invalid.entities, invalid.objects, {});
        test.Expect(invalidActor.size() == 1 &&
                        invalidActor[0].clearReason ==
                            InteractionClearReason::INVALID_ACTOR,
                    "Invalid stored actor emits clear intent");

        InteractionSystemTestAccess::Insert(
            invalid.system,
            {invalid.firstPlayerID, invalid.ResourceID(),
             InteractionTargetType::STATION});
        auto mismatch = invalid.system.Evaluate(invalid.entities, invalid.objects, {});
        test.Expect(mismatch.size() == 1 &&
                        mismatch[0].clearReason ==
                            InteractionClearReason::TARGET_TYPE_MISMATCH,
                    "Stored target-type mismatch emits clear intent");
    }
}

int main()
{
    TestContext test;
    TestState(test);
    TestEvaluation(test);
    return test.Finish();
}

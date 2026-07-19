#include "TestSupport.h"

#include "../src/Entity/Manager/EntityManager.h"
#include "../src/Movement/MovementSystem.h"
#include "../src/World/Map.h"
#include "../src/World/Tile/TileType.h"

#include <type_traits>

struct EntityManagerTestAccess
{
    static bool RemoveEntity(EntityManager &manager, int entityID)
    {
        return manager.RemoveEntity(entityID);
    }
};

struct MovementSystemTestAccess
{
    static std::size_t RemainingSteps(
        const MovementSystem &system,
        int entityID)
    {
        auto iterator = system.activeMovementPaths.find(entityID);
        return iterator == system.activeMovementPaths.end()
                   ? 0
                   : iterator->second.remainingSteps.size();
    }

    static bool StoresEntityPointers()
    {
        using State = decltype(MovementSystem::activeMovementPaths)::mapped_type;
        using Step = typename decltype(State::remainingSteps)::value_type;
        return std::is_pointer_v<decltype(State::requestedDestination)> ||
               std::is_pointer_v<Step>;
    }
};

namespace
{
    void SetPosition(EntityManager &manager, int entityID, int x, int y)
    {
        manager.GetEntityByID(entityID)->GetPosition().SetPosition(x, y);
    }

    void TestOwnershipAndQueue(TestContext &test)
    {
        EntityManager entities;
        Map map(20, 20);
        MovementSystem system;
        int entityID = entities.CreateNPC(5, 5);

        test.ExpectEqual(system.GetActiveMovementCount(), std::size_t{0},
                         "System starts without active movement");
        test.Expect(!system.HasMovement(entityID),
                    "System starts without queued movement");
        test.Expect(system.QueueDestination(entityID, Position(8, 5), entities, map),
                    "Valid destination is accepted");
        test.Expect(system.HasMovement(entityID),
                    "Queued destination is observable");
        test.Expect(!system.QueueDestination(999, Position(8, 5), entities, map),
                    "Unknown entity is rejected");
        test.Expect(!system.QueueDestination(entityID, Position(20, 5), entities, map),
                    "Out-of-bounds destination is rejected");

        map.SetTileType(9, 5, TileType::WALL);
        test.Expect(system.QueueDestination(entityID, Position(9, 5), entities, map),
                    "Blocked in-bounds destination is accepted");
        test.Expect(system.QueueDestination(entityID, Position(5, 8), entities, map),
                    "Newer destination is accepted");
        auto destination = system.GetDestination(entityID);
        test.Expect(destination.has_value() &&
                        destination->GetX() == 5 && destination->GetY() == 8,
                    "Newest queued destination is retained");

        system.Process(entities, map);
        test.ExpectEqual(system.GetActiveMovementCount(), std::size_t{1},
                         "Replacement leaves exactly one active state");
        test.Expect(!MovementSystemTestAccess::StoresEntityPointers(),
                    "Movement state stores values rather than entity pointers");
        test.Expect(system.CancelMovement(entityID),
                    "Known movement can be cancelled");
        test.Expect(!system.HasMovement(entityID),
                    "Cancellation removes movement state");
        test.Expect(!system.CancelMovement(entityID),
                    "Cancelling missing movement fails safely");
    }

    void TestStepProgression(TestContext &test)
    {
        const Position destinations[] = {
            Position(5, 4), Position(5, 6), Position(6, 5), Position(4, 5)};

        for (const Position &destination : destinations)
        {
            EntityManager entities;
            Map map(12, 12);
            MovementSystem system;
            int entityID = entities.CreateNPC(5, 5);
            system.QueueDestination(entityID, destination, entities, map);
            auto outcomes = system.Process(entities, map);

            Entity *entity = entities.GetEntityByID(entityID);
            const int distance =
                std::abs(entity->GetPosition().GetX() - 5) +
                std::abs(entity->GetPosition().GetY() - 5);
            test.ExpectEqual(distance, 1,
                             "Each cardinal direction moves exactly one tile");
            test.ExpectEqual(outcomes.size(), std::size_t{1},
                             "One entity produces one primary outcome");
            test.Expect(outcomes[0].type == MovementOutcomeType::ARRIVED,
                        "A final successful step emits ARRIVED");
            test.Expect(outcomes[0].previousPosition.GetX() == 5 &&
                            outcomes[0].previousPosition.GetY() == 5 &&
                            outcomes[0].currentPosition.GetX() == destination.GetX() &&
                            outcomes[0].currentPosition.GetY() == destination.GetY(),
                        "Successful outcome contains prior and current values");
            test.Expect(!system.HasMovement(entityID),
                        "Arrival removes path state");
        }

        EntityManager entities;
        Map map(20, 20);
        MovementSystem system;
        int entityID = entities.CreateNPC(2, 2);
        system.QueueDestination(entityID, Position(6, 4), entities, map);

        auto first = system.Process(entities, map);
        test.Expect(first.size() == 1 &&
                        first[0].type == MovementOutcomeType::STEP_SUCCEEDED,
                    "Intermediate success emits STEP_SUCCEEDED");
        test.ExpectEqual(
            MovementSystemTestAccess::RemainingSteps(system, entityID),
            std::size_t{5},
            "Successful movement removes exactly one stored step");
        test.ExpectEqual(
            std::abs(entities.GetEntityByID(entityID)->GetPosition().GetX() - 2) +
                std::abs(entities.GetEntityByID(entityID)->GetPosition().GetY() - 2),
            1,
            "A process call never performs diagonal or multi-tile movement");
    }

    void TestRecalculation(TestContext &test)
    {
        EntityManager entities;
        Map map(20, 20);
        MovementSystem system;
        int entityID = entities.CreateNPC(2, 2);
        system.QueueDestination(entityID, Position(6, 2), entities, map);
        system.Process(entities, map);

        map.SetTileType(4, 2, TileType::WALL);
        Position before = entities.GetEntityByID(entityID)->GetPosition();
        auto recalculated = system.Process(entities, map);
        Position after = entities.GetEntityByID(entityID)->GetPosition();

        test.Expect(recalculated.size() == 1 &&
                        recalculated[0].type == MovementOutcomeType::PATH_RECALCULATED,
                    "Dynamic obstruction emits one recalculation outcome");
        test.Expect(before.GetX() == after.GetX() && before.GetY() == after.GetY(),
                    "Recalculation performs no same-call movement");
        auto continued = system.Process(entities, map);
        test.Expect(continued.size() == 1 &&
                        continued[0].type == MovementOutcomeType::STEP_SUCCEEDED,
                    "Replacement path continues on the next call");

        EntityManager trappedEntities;
        Map trappedMap(8, 8);
        MovementSystem trappedSystem;
        int trappedID = trappedEntities.CreateNPC(3, 3);
        trappedSystem.QueueDestination(trappedID, Position(6, 3), trappedEntities, trappedMap);
        trappedSystem.Process(trappedEntities, trappedMap);
        trappedMap.SetTileType(5, 3, TileType::WALL);
        trappedMap.SetTileType(4, 2, TileType::WALL);
        trappedMap.SetTileType(4, 4, TileType::WALL);
        trappedMap.SetTileType(3, 3, TileType::WALL);

        Position trappedBefore = trappedEntities.GetEntityByID(trappedID)->GetPosition();
        auto cancelled = trappedSystem.Process(trappedEntities, trappedMap);
        Position trappedAfter = trappedEntities.GetEntityByID(trappedID)->GetPosition();
        test.Expect(cancelled.size() == 1 &&
                        cancelled[0].type == MovementOutcomeType::PATH_CANCELLED,
                    "Failed recalculation emits PATH_CANCELLED");
        test.Expect(trappedBefore.GetX() == trappedAfter.GetX() &&
                        trappedBefore.GetY() == trappedAfter.GetY(),
                    "Failed step is not skipped or converted into a teleport");
        test.Expect(!trappedSystem.HasMovement(trappedID),
                    "Failed recalculation removes path state");

        trappedMap.SetTileType(3, 3, TileType::GRASS);
        trappedMap.SetTileType(3, 2, TileType::GRASS);
        test.Expect(trappedSystem.QueueDestination(
                        trappedID, Position(3, 2), trappedEntities, trappedMap),
                    "New movement can be queued after cancellation");
        test.Expect(!trappedSystem.Process(trappedEntities, trappedMap).empty(),
                    "New movement works after cancellation");
    }

    void TestDeterminismAndStaleState(TestContext &test)
    {
        EntityManager entities;
        Map map(20, 20);
        MovementSystem system;
        int firstID = entities.CreateNPC(2, 2);
        int secondID = entities.CreateNPC(2, 4);
        int thirdID = entities.CreateNPC(2, 6);

        system.QueueDestination(thirdID, Position(8, 6), entities, map);
        system.QueueDestination(firstID, Position(8, 2), entities, map);
        system.QueueDestination(secondID, Position(8, 4), entities, map);
        auto outcomes = system.Process(entities, map);

        test.Expect(outcomes.size() == 3 &&
                        outcomes[0].entityID == firstID &&
                        outcomes[1].entityID == secondID &&
                        outcomes[2].entityID == thirdID,
                    "Outcome order follows entity creation rather than hash insertion");

        test.Expect(EntityManagerTestAccess::RemoveEntity(entities, secondID),
                    "Physical removal setup succeeds");
        system.Process(entities, map);
        test.Expect(!system.HasMovement(secondID) &&
                        system.GetActiveMovementCount() == 2,
                    "Missing entity state is cleaned without blocking other movement");

        for (int index = 0; index < 100; ++index)
        {
            entities.CreateNPC(10, 10);
        }
        auto afterGrowth = system.Process(entities, map);
        test.Expect(afterGrowth.size() == 2,
                    "Entity vector growth does not invalidate movement state");
    }
}

int main()
{
    TestContext test;
    TestOwnershipAndQueue(test);
    TestStepProgression(test);
    TestRecalculation(test);
    TestDeterminismAndStaleState(test);
    return test.Finish();
}

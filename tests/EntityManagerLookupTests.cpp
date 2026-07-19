#include "TestSupport.h"

#include "../src/Entity/Manager/EntityManager.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/NPC/NPC.h"
#include "../src/Player/Player.h"

#include <memory>
#include <type_traits>

struct EntityManagerTestAccess
{
    static Entity *RegisterEntity(
        EntityManager &manager,
        std::unique_ptr<Entity> entity)
    {
        return manager.RegisterEntity(std::move(entity));
    }

    static bool RemoveEntity(EntityManager &manager, int id)
    {
        return manager.RemoveEntity(id);
    }

    static std::size_t GetLookupCount(
        const EntityManager &manager)
    {
        return manager.entityLookup.size();
    }
};

namespace
{
    void TestLookupBasics(TestContext &test)
    {
        EntityManager manager;

        test.Expect(manager.GetEntityByID(1) == nullptr,
                    "Empty manager returns null");
        test.Expect(manager.GetEntityByID(-1) == nullptr,
                    "Negative ID returns null");
        test.Expect(manager.GetEntityByID(1000000) == nullptr,
                    "Unknown large ID returns null");

        int playerID = manager.CreatePlayer();
        int npcID = manager.CreateNPC(3, 4);
        int monsterID = manager.CreateMonster(
            5, 6, CombatRatings{1, 2, 3, 10});

        Entity *player = manager.GetEntityByID(playerID);
        Entity *npc = manager.GetEntityByID(npcID);
        Entity *monster = manager.GetEntityByID(monsterID);

        test.Expect(dynamic_cast<Player *>(player) != nullptr,
                    "Created Player is retrievable by exact ID");
        test.Expect(dynamic_cast<NPC *>(npc) != nullptr,
                    "Created NPC is retrievable by exact ID");
        test.Expect(dynamic_cast<Monster *>(monster) != nullptr,
                    "Created Monster is retrievable by exact ID");
        test.Expect(player == manager.GetEntities()[0].get(),
                    "Lookup returns the exact owned object");

        const EntityManager &constManager = manager;
        static_assert(std::is_same_v<
                      decltype(constManager.GetEntityByID(playerID)),
                      const Entity *>);
        test.Expect(constManager.GetEntityByID(playerID) == player,
                    "Const lookup returns the exact const-visible object");

        std::size_t entityCount = manager.GetEntities().size();
        std::size_t lookupCount =
            EntityManagerTestAccess::GetLookupCount(manager);
        test.Expect(manager.GetEntityByID(999999) == nullptr,
                    "Absent lookup remains absent");
        test.ExpectEqual(manager.GetEntities().size(), entityCount,
                         "Absent lookup does not change entity count");
        test.ExpectEqual(
            EntityManagerTestAccess::GetLookupCount(manager),
            lookupCount,
            "Absent lookup does not insert into index");
    }

    void TestDeterministicIterationAndPointerStability(
        TestContext &test)
    {
        EntityManager manager;
        int playerID = manager.CreatePlayer();
        int npcID = manager.CreateNPC(1, 2);
        int monsterID = manager.CreateMonster(
            3, 4, CombatRatings{1, 1, 1, 5});

        const auto &initialEntities = manager.GetEntities();
        test.Expect(initialEntities[0]->GetID() == playerID &&
                        initialEntities[1]->GetID() == npcID &&
                        initialEntities[2]->GetID() == monsterID,
                    "Iteration order remains creation order");

        Entity *firstPointer = manager.GetEntityByID(playerID);
        for (int index = 0; index < 512; ++index)
        {
            manager.CreatePlayer();
        }

        test.Expect(manager.GetEntityByID(playerID) == firstPointer,
                    "Vector growth preserves indexed entity pointers");
        test.Expect(manager.GetEntities().front().get() == firstPointer,
                    "Owning collection still contains the original object");
    }

    void TestRegistrationIntegrity(TestContext &test)
    {
        EntityManager manager;
        manager.CreatePlayer();
        manager.CreateNPC(1, 1);
        manager.CreateMonster(2, 2, CombatRatings{1, 1, 1, 5});

        test.ExpectEqual(
            manager.GetEntities().size(),
            EntityManagerTestAccess::GetLookupCount(manager),
            "Every creation path registers exactly once");

        std::size_t entityCount = manager.GetEntities().size();
        std::size_t lookupCount =
            EntityManagerTestAccess::GetLookupCount(manager);
        int duplicateID = manager.GetEntities().front()->GetID();

        test.Expect(
            EntityManagerTestAccess::RegisterEntity(
                manager, std::make_unique<Player>(duplicateID)) == nullptr,
            "Duplicate ID registration fails safely");
        test.Expect(
            EntityManagerTestAccess::RegisterEntity(manager, nullptr) == nullptr,
            "Null registration fails safely");
        test.Expect(
            EntityManagerTestAccess::RegisterEntity(
                manager, std::make_unique<Player>(0)) == nullptr,
            "Invalid ID registration fails safely");
        test.Expect(
            manager.GetEntities().size() == entityCount &&
                EntityManagerTestAccess::GetLookupCount(manager) == lookupCount,
            "Failed registration leaves both structures unchanged");
    }

    void TestRemovalSynchronization(TestContext &test)
    {
        EntityManager manager;
        int firstID = manager.CreatePlayer();
        int removedID = manager.CreateNPC(1, 1);
        int lastID = manager.CreateMonster(
            2, 2, CombatRatings{1, 1, 1, 5});

        test.Expect(EntityManagerTestAccess::RemoveEntity(manager, removedID),
                    "Removing a known entity succeeds");
        test.Expect(manager.GetEntityByID(removedID) == nullptr,
                    "Removed ID returns null without a stale pointer");
        test.Expect(manager.GetEntityByID(firstID) != nullptr &&
                        manager.GetEntityByID(lastID) != nullptr,
                    "Other entities remain retrievable");
        test.Expect(manager.GetEntities().size() == 2 &&
                        manager.GetEntities()[0]->GetID() == firstID &&
                        manager.GetEntities()[1]->GetID() == lastID,
                    "Remaining iteration order is preserved");

        std::size_t entityCount = manager.GetEntities().size();
        std::size_t lookupCount =
            EntityManagerTestAccess::GetLookupCount(manager);
        test.Expect(!EntityManagerTestAccess::RemoveEntity(manager, 999999),
                    "Removing an unknown ID fails safely");
        test.Expect(!EntityManagerTestAccess::RemoveEntity(manager, removedID),
                    "Removing the same ID twice fails safely");
        test.Expect(manager.GetEntities().size() == entityCount &&
                        EntityManagerTestAccess::GetLookupCount(manager) == lookupCount,
                    "Failed removal does not mutate either structure");
    }
}

int main()
{
    TestContext test;
    TestLookupBasics(test);
    TestDeterministicIterationAndPointerStability(test);
    TestRegistrationIntegrity(test);
    TestRemovalSynchronization(test);
    return test.Finish();
}

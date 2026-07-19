#include "TestSupport.h"

#include "../src/NPC/NpcSpawnDatabase.h"
#include "../src/NPC/NpcSpawnManager.h"

int main()
{
    TestContext test;
    const auto &spawns = NpcSpawnDatabase::GetStarterMonsterSpawns();
    NpcSpawnManager manager;

    test.Expect(manager.AddSpawn(spawns[0]), "Valid passive spawn is added");
    test.Expect(manager.AddSpawn(spawns[1]), "Valid aggressive spawn is added");
    test.Expect(!manager.AddSpawn(spawns[0]), "Duplicate spawn ID is rejected");
    test.Expect(!manager.HasSpawn(NpcSpawnId::NONE), "NONE spawn is unknown");
    test.Expect(!manager.RegisterEntity(NpcSpawnId::NONE, 20, 0), "Unknown spawn registration fails");
    NpcSpawnDefinition unknownIdSpawn = spawns[0];
    unknownIdSpawn.spawnId = static_cast<NpcSpawnId>(999);
    test.Expect(!manager.AddSpawn(unknownIdSpawn), "Unknown spawn definition ID is rejected");

    test.Expect(manager.RegisterEntity(spawns[0].spawnId, 2, 0), "Entity registers to passive spawn");
    test.ExpectEqual(manager.GetActiveCount(spawns[0].spawnId), 1, "Registered ownership counts as active");
    test.Expect(manager.GetSpawnId(2).value_or(NpcSpawnId::NONE) == spawns[0].spawnId,
                "Entity resolves to its stable spawn ID");
    test.Expect(!manager.RegisterEntity(spawns[0].spawnId, 2, 0), "Duplicate entity registration fails");
    test.Expect(!manager.RegisterEntity(spawns[1].spawnId, 2, 0),
                "One entity cannot belong to two spawns");
    test.ExpectEqual(manager.GetActiveCount(spawns[1].spawnId), 0,
                     "Cross-spawn duplicate does not consume capacity");
    test.Expect(!manager.HasCapacity(spawns[0].spawnId), "Maximum active count is enforced");
    test.Expect(!manager.RegisterEntity(spawns[0].spawnId, 4, 0), "Capacity rejects another entity");

    test.Expect(!manager.IsWanderDue(2, 4), "Wander is not due before its interval");
    test.Expect(manager.IsWanderDue(2, 5), "Wander is due on its interval");
    test.ExpectEqual(manager.BeginWanderAttempt(2, 5).value_or(-1), 0,
                     "First deterministic direction index is zero");
    test.Expect(!manager.IsWanderDue(2, 9), "Attempt advances the deterministic schedule");
    test.ExpectEqual(manager.BeginWanderAttempt(2, 10).value_or(-1), 1,
                     "Direction index rotates independently per entity");
    test.Expect(manager.ResetWanderState(2, 20), "Wander state resets for respawn");
    test.ExpectEqual(manager.GetActiveCount(spawns[0].spawnId), 1,
                     "Wander reset does not change spawn membership");
    test.ExpectEqual(manager.BeginWanderAttempt(2, 25).value_or(-1), 0,
                     "Respawn reset restores the initial candidate index");

    test.Expect(manager.UnregisterEntity(2), "Permanent removal unregisters ownership");
    test.ExpectEqual(manager.GetActiveCount(spawns[0].spawnId), 0, "Unregister releases capacity");
    test.Expect(!manager.GetSpawnId(2).has_value(), "Unregister clears reverse lookup");
    test.Expect(!manager.UnregisterEntity(2), "Repeated unregister fails safely");
    test.Expect(!manager.UnregisterEntity(999), "Unknown removal fails safely");
    test.Expect(!manager.IsWanderDue(2, 100), "Unregister removes wander state");
    test.Expect(manager.HasCapacity(spawns[0].spawnId), "Unregister restores capacity");

    return test.Finish();
}

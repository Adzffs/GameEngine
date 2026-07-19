#include "TestSupport.h"

#include "../src/Action/Action.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/NPC/NpcSpawnDatabase.h"
#include "../src/NPC/NpcSpawnId.h"
#include "../src/World/Tile/TileType.h"
#include "../src/World/World.h"

struct WorldTestAccess
{
    static void StartRetaliation(World &world, int monsterId, int playerId)
    {
        world.TryStartMonsterRetaliation(monsterId, playerId);
    }

    static bool RemoveEntity(World &world, int entityId)
    {
        return world.RemoveEntity(entityId);
    }

    static void EndCombat(World &world, int monsterId)
    {
        world.CancelActionsForEntity(monsterId);
        world.meleeEngagementSystem.ClearEngagement(monsterId);
    }
};

namespace
{
    Monster *Find(World &world, NpcSpawnId spawnId)
    {
        for (const auto &entity : world.GetEntities())
        {
            Monster *monster = dynamic_cast<Monster *>(entity.get());
            if (monster != nullptr && monster->GetNpcSpawnId() == spawnId)
                return monster;
        }
        return nullptr;
    }
}

int main()
{
    TestContext test;
    World world;
    Monster *passive = Find(world, NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN);
    Monster *aggressive = Find(world, NpcSpawnId::AGGRESSIVE_DEVELOPMENT_SPAWN);
    test.Expect(passive != nullptr && aggressive != nullptr, "Both authored monsters expose stable spawn identity");
    if (passive == nullptr || aggressive == nullptr)
        return test.Finish();

    for (int tick = 0; tick < 4; ++tick)
        world.Update();
    test.ExpectEqual(world.GetCurrentTick(), 4, "Pre-wander assertions occur at exact tick four");
    test.ExpectEqual(passive->GetPosition().GetX(), 6, "Passive does not wander before tick five");
    test.Expect(!world.HasActiveMovementPath(passive->GetID()), "No early wander path exists");

    world.Update();
    test.ExpectEqual(world.GetCurrentTick(), 5, "First wander evaluation occurs at exact tick five");
    test.Expect(world.HasActiveMovementPath(passive->GetID()), "Tick five creates one deterministic wander request");
    test.ExpectEqual(world.GetMovementDestination(passive->GetID())->GetX(), 7, "First destination uses east-first order");
    test.ExpectEqual(world.GetMovementDestination(passive->GetID())->GetY(), 1, "First destination remains adjacent");
    test.Expect(!world.HasActiveMovementPath(aggressive->GetID()), "Aggressive spawn never receives wandering movement");

    world.Update();
    test.ExpectEqual(world.GetCurrentTick(), 6, "First wander movement executes at exact tick six");
    test.ExpectEqual(passive->GetPosition().GetX(), 7, "Wander uses movement system on the following movement phase");
    test.ExpectEqual(passive->GetPosition().GetY(), 1, "Wander performs one adjacent step");

    for (int tick = 0; tick < 5; ++tick)
        world.Update();
    test.ExpectEqual(world.GetCurrentTick(), 11, "Second scheduled movement executes at exact tick eleven");
    test.ExpectEqual(passive->GetPosition().GetX(), 7, "Second deterministic destination preserves X");
    test.ExpectEqual(passive->GetPosition().GetY(), 2, "Second deterministic attempt rotates south");

    World repeated;
    Monster *repeatedPassive = Find(repeated, NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN);
    for (int tick = 0; tick < 11; ++tick)
        repeated.Update();
    test.ExpectEqual(repeatedPassive->GetPosition().GetX(), passive->GetPosition().GetX(),
                     "Repeated simulation produces identical wander X");
    test.ExpectEqual(repeatedPassive->GetPosition().GetY(), passive->GetPosition().GetY(),
                     "Repeated simulation produces identical wander Y");

    World blocked;
    blocked.GetMap().SetTileType(7, 1, TileType::TREE);
    Monster *blockedPassive = Find(blocked, NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN);
    for (int tick = 0; tick < 6; ++tick)
        blocked.Update();
    test.ExpectEqual(blockedPassive->GetPosition().GetX(), 6, "Blocked first candidate is skipped");
    test.ExpectEqual(blockedPassive->GetPosition().GetY(), 2, "Next valid candidate is selected deterministically");

    World fullyBlocked;
    fullyBlocked.GetMap().SetTileType(7, 1, TileType::TREE);
    fullyBlocked.GetMap().SetTileType(6, 2, TileType::TREE);
    fullyBlocked.GetMap().SetTileType(5, 1, TileType::TREE);
    fullyBlocked.GetMap().SetTileType(6, 0, TileType::TREE);
    Monster *fullyBlockedPassive = Find(fullyBlocked, NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN);
    for (int tick = 0; tick < 5; ++tick)
        fullyBlocked.Update();
    test.ExpectEqual(fullyBlocked.GetCurrentTick(), 5,
                     "Fully blocked attempt occurs on exact eligible tick");
    test.Expect(!fullyBlocked.HasActiveMovementPath(fullyBlockedPassive->GetID()),
                "Fully blocked attempt creates no invalid path");
    for (int tick = 0; tick < 5; ++tick)
        fullyBlocked.Update();
    test.ExpectEqual(fullyBlocked.GetCurrentTick(), 10,
                     "Failed attempt retains the deterministic five-tick schedule");
    test.Expect(!fullyBlocked.HasActiveMovementPath(fullyBlockedPassive->GetID()),
                "Repeated fully blocked attempt remains path-free");

    World retaliation;
    Monster *retaliatingPassive = Find(retaliation, NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN);
    const int playerId = retaliation.CreatePlayer();
    retaliation.GetEntityByID(playerId)->GetPosition().SetPosition(7, 1);
    for (int tick = 0; tick < 5; ++tick)
        retaliation.Update();
    test.Expect(retaliation.HasActiveMovementPath(retaliatingPassive->GetID()),
                "Passive has a wander path before retaliation");
    WorldTestAccess::StartRetaliation(retaliation, retaliatingPassive->GetID(), playerId);
    test.Expect(!retaliation.HasActiveMovementPath(retaliatingPassive->GetID()),
                "Retaliation immediately preempts wandering movement");
    test.Expect(retaliation.HasPendingMeleeEngagement(retaliatingPassive->GetID()) ||
                    retaliation.GetActionForEntity(retaliatingPassive->GetID()) != nullptr,
                "Passive retaliation keeps existing combat timing flow");
    retaliation.Update();
    const Action *retaliationAction =
        retaliation.GetActionForEntity(retaliatingPassive->GetID());
    test.Expect(retaliationAction != nullptr, "Retaliation starts its melee action immediately");
    if (retaliationAction != nullptr)
    {
        test.ExpectEqual(retaliationAction->GetStartTick(), 5,
                         "Retaliation action starts on exact attack tick five");
        test.ExpectEqual(retaliationAction->GetCompletionTick(), 10,
                         "Passive attack timing remains exactly five ticks");
        test.ExpectEqual(retaliationAction->GetTargetID(), playerId,
                         "Retaliation establishes the attacking player as target");
    }
    test.ExpectEqual(retaliatingPassive->GetPosition().GetX(), 6,
                     "Preempted wander step never executes");
    test.ExpectEqual(retaliatingPassive->GetPosition().GetY(), 1,
                     "Retaliation leaves passive on its pre-wander tile");
    WorldTestAccess::EndCombat(retaliation, retaliatingPassive->GetID());
    for (int tick = 0; tick < 3; ++tick)
        retaliation.Update();
    test.ExpectEqual(retaliation.GetCurrentTick(), 9,
                     "Combat cleanup does not restore old path through tick nine");
    test.Expect(!retaliation.HasActiveMovementPath(retaliatingPassive->GetID()),
                "Old wander path does not resume after combat");
    retaliation.Update();
    test.ExpectEqual(retaliation.GetCurrentTick(), 10,
                     "New wandering waits for the next scheduled attempt");
    test.ExpectEqual(retaliation.GetMovementDestination(retaliatingPassive->GetID())->GetY(), 2,
                     "Post-combat attempt uses the next deterministic direction");

    World deathBeforeMovement;
    Monster *doomedWanderer = Find(deathBeforeMovement, NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN);
    for (int tick = 0; tick < 5; ++tick)
        deathBeforeMovement.Update();
    test.Expect(deathBeforeMovement.HasActiveMovementPath(doomedWanderer->GetID()),
                "Death-preemption fixture has a queued wander path");
    doomedWanderer->ApplyDamage(doomedWanderer->GetCurrentHealth());
    deathBeforeMovement.Update();
    test.Expect(!deathBeforeMovement.HasActiveMovementPath(doomedWanderer->GetID()),
                "Death cleanup clears wandering before movement executes");
    test.ExpectEqual(doomedWanderer->GetPosition().GetX(), 6,
                     "Dead monster does not execute queued wander X");
    test.ExpectEqual(doomedWanderer->GetPosition().GetY(), 1,
                     "Dead monster does not execute queued wander Y");
    test.ExpectEqual(deathBeforeMovement.GetNpcSpawnManager().GetActiveCount(
                         NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN), 1,
                     "Dead wanderer remains registered but inactive");

    World respawn;
    Monster *respawningPassive = Find(respawn, NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN);
    const int respawningId = respawningPassive->GetID();
    respawningPassive->ApplyDamage(respawningPassive->GetCurrentHealth());
    respawn.Update();
    test.ExpectEqual(respawn.GetNpcSpawnManager().GetActiveCount(
                         NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN), 1,
                     "Dead waiting-to-respawn ownership still occupies capacity");
    test.Expect(respawn.HasScheduledMonsterRespawn(respawningId),
                "Existing scheduler remains authoritative for respawn");
    for (int tick = 0; tick < 8; ++tick)
        respawn.Update();
    test.ExpectEqual(respawn.GetCurrentTick(), 9, "Respawn occurs on exact due tick nine");
    test.Expect(respawningPassive->IsAlive(), "Passive respawns after the existing eight-tick delay");
    test.ExpectEqual(respawningPassive->GetID(), respawningId, "Respawn preserves entity ID");
    test.ExpectEqual(respawningPassive->GetPosition().GetX(), 6, "Respawn restores authored home X");
    test.ExpectEqual(respawningPassive->GetPosition().GetY(), 1, "Respawn restores authored home Y");
    test.ExpectEqual(respawningPassive->GetCurrentHealth(), respawningPassive->GetMaximumHealth(),
                     "Respawn restores full health");
    test.Expect(!respawn.HasActiveMovementPath(respawningId),
                "Respawn resets wandering without an immediate path");
    for (int tick = 0; tick < 4; ++tick)
        respawn.Update();
    test.ExpectEqual(respawn.GetCurrentTick(), 13, "Post-respawn wander remains suppressed through tick thirteen");
    test.Expect(!respawn.HasActiveMovementPath(respawningId),
                "No post-respawn wander occurs before the reset interval");
    respawn.Update();
    test.ExpectEqual(respawn.GetCurrentTick(), 14, "First post-respawn wander is evaluated at tick fourteen");
    test.ExpectEqual(respawn.GetMovementDestination(respawningId)->GetX(), 7,
                     "Respawn resets direction index to east");

    World removal;
    Monster *removedPassive = Find(removal, NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN);
    test.Expect(WorldTestAccess::RemoveEntity(removal, removedPassive->GetID()),
                "Permanent removal succeeds");
    test.ExpectEqual(removal.GetNpcSpawnManager().GetActiveCount(
                         NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN), 0,
                     "Permanent removal releases spawn capacity");
    test.Expect(!removal.GetNpcSpawnManager().GetSpawnId(removedPassive->GetID()).has_value(),
                "Permanent removal clears reverse spawn lookup");
    test.Expect(removal.GetNpcSpawnManager().HasCapacity(
                    NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN),
                "Permanent removal makes replacement capacity available");
    const NpcSpawnDefinition &passiveSpawn =
        NpcSpawnDatabase::GetStarterMonsterSpawns()[0];
    const int replacementId = removal.CreateMonster(passiveSpawn.npcType, passiveSpawn);
    test.Expect(replacementId > 0, "A replacement can be created after permanent removal");
    test.Expect(removal.GetNpcSpawnManager().GetSpawnId(replacementId).value_or(NpcSpawnId::NONE) ==
                    NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN,
                "Replacement receives fresh reverse membership");
    for (int tick = 0; tick < 5; ++tick)
        removal.Update();
    test.ExpectEqual(removal.GetMovementDestination(replacementId)->GetX(), 7,
                     "Replacement has fresh wander state with no old direction index");

    return test.Finish();
}

#include "TestSupport.h"
#include "../src/Command/ServerCommand.h"
#include "../src/Core/RandomSource.h"
#include "../src/Interaction/InteractionSystem.h"
#include "../src/NPC/NPC.h"
#include "../src/NPC/NpcSpawnDatabase.h"
#include "../src/Player/Player.h"
#include "../src/World/Tile/TileType.h"
#include "../src/World/World.h"

#include <memory>
#include <type_traits>

static_assert(std::is_same_v<decltype(NpcInteractionCommand::actorEntityID), int>);
static_assert(std::is_same_v<decltype(NpcInteractionCommand::targetNpcEntityID), int>);
static_assert(std::is_same_v<decltype(NpcInteractionCommand::interactionType), NpcInteractionType>);
static_assert(!std::is_pointer_v<decltype(PendingInteraction::actorEntityID)>);
static_assert(!std::is_pointer_v<decltype(PendingInteraction::targetObjectID)>);
static_assert(!std::is_pointer_v<decltype(NpcTalkEvent::text)>);

struct WorldTestAccess
{
    static std::optional<PendingInteraction> Pending(World &world, int actorId)
    {
        return world.interactionSystem.GetInteraction(actorId);
    }

    static bool Remove(World &world, int entityId)
    {
        return world.RemoveEntity(entityId);
    }
};

namespace
{
    class CountingRandomSource final : public RandomSource
    {
    public:
        int NextIntInclusive(int minimum, int) override
        {
            ++calls;
            return minimum;
        }
        int calls = 0;
    };

    Player *CreatePlayerAt(World &world, int x, int y)
    {
        Player *player = dynamic_cast<Player *>(world.GetEntityByID(world.CreatePlayer()));
        player->GetPosition().SetPosition(x, y);
        return player;
    }

    void QueueTalk(World &world, int actorId)
    {
        world.EnqueueCommand(NpcInteractionCommand{actorId, 1, NpcInteractionType::TALK});
    }

    void ExpectPendingType(TestContext &test, World &world, int actorId,
                           InteractionTargetType type, const std::string &message)
    {
        const auto pending = WorldTestAccess::Pending(world, actorId);
        test.Expect(pending.has_value() && pending->targetType == type, message);
    }
}

int main()
{
    TestContext test;

    World nearby;
    Player *nearbyPlayer = CreatePlayerAt(nearby, 2, 3);
    QueueTalk(nearby, nearbyPlayer->GetID());
    nearby.Update();
    test.ExpectEqual(nearby.GetCurrentTick(), 1, "Adjacent TALK completes on tick one");
    test.ExpectEqual(nearby.GetNpcTalkEvents().size(), std::size_t{1},
                     "Adjacent TALK emits once on tick one");
    const NpcTalkEvent event = nearby.GetNpcTalkEvents().front();
    test.ExpectEqual(event.actorEntityID, nearbyPlayer->GetID(), "Event actor is authoritative");
    test.ExpectEqual(event.npcEntityID, 1, "Event target is authoritative");
    test.Expect(event.npcType == NpcType::DEVELOPMENT_GUIDE, "Event type is stable");
    test.Expect(event.text == "Welcome to the development world.", "Event copies server-owned text");
    test.Expect(!WorldTestAccess::Pending(nearby, nearbyPlayer->GetID()).has_value(),
                "Completion clears pending TALK");
    nearby.Update();
    test.Expect(nearby.GetNpcTalkEvents().empty(), "Published TALK lasts one update only");

    World distant;
    Player *distantPlayer = CreatePlayerAt(distant, 0, 0);
    QueueTalk(distant, distantPlayer->GetID());
    distant.Update();
    test.ExpectEqual(distant.GetCurrentTick(), 1, "Distant command processes on tick one");
    test.ExpectEqual(distantPlayer->GetPosition().GetX(), 1, "Tick one path step X");
    test.ExpectEqual(distantPlayer->GetPosition().GetY(), 0, "Tick one path step Y");
    test.Expect(distant.GetNpcTalkEvents().empty(), "No tick-one event while out of range");
    distant.Update();
    test.ExpectEqual(distantPlayer->GetPosition().GetX(), 2, "Tick two path step X");
    test.ExpectEqual(distantPlayer->GetPosition().GetY(), 0, "Tick two path step Y");
    test.Expect(distant.GetNpcTalkEvents().empty(), "No tick-two event while out of range");
    distant.Update();
    test.ExpectEqual(distantPlayer->GetPosition().GetX(), 3, "Tick three path step X");
    test.ExpectEqual(distantPlayer->GetPosition().GetY(), 0, "Tick three path step Y");
    test.Expect(distant.GetNpcTalkEvents().empty(), "No tick-three event while out of range");
    distant.Update();
    test.ExpectEqual(distantPlayer->GetPosition().GetX(), 3, "Tick four arrival X");
    test.ExpectEqual(distantPlayer->GetPosition().GetY(), 1, "Tick four path step Y");
    test.Expect(distant.GetNpcTalkEvents().empty(), "No tick-four event while out of range");
    distant.Update();
    test.ExpectEqual(distantPlayer->GetPosition().GetX(), 3, "Tick five arrival X");
    test.ExpectEqual(distantPlayer->GetPosition().GetY(), 2, "Tick five arrival Y");
    test.ExpectEqual(distant.GetNpcTalkEvents().size(), std::size_t{1},
                     "Adjacency emits on tick five");
    test.Expect(!distant.HasActiveMovementPath(distantPlayer->GetID()),
                "Ready TALK clears any remaining approach path");

    World replacements;
    Player *replacementPlayer = CreatePlayerAt(replacements, 0, 0);
    const ResourceNode &resource = replacements.GetResources().front();
    const CraftingStation &station = replacements.GetStations().front();
    replacements.QueueResourceInteraction(replacementPlayer->GetID(), resource.GetID());
    QueueTalk(replacements, replacementPlayer->GetID());
    replacements.Update();
    ExpectPendingType(test, replacements, replacementPlayer->GetID(), InteractionTargetType::NPC,
                      "Resource to NPC leaves only NPC pending");
    replacements.EnqueueCommand(InteractCommand{replacementPlayer->GetID(),
        InteractionTargetType::RESOURCE, resource.GetID(), Position{resource.GetX(), resource.GetY()}});
    replacements.Update();
    ExpectPendingType(test, replacements, replacementPlayer->GetID(), InteractionTargetType::RESOURCE,
                      "NPC to resource leaves only resource pending");
    QueueTalk(replacements, replacementPlayer->GetID());
    replacements.Update();
    replacements.EnqueueCommand(InteractCommand{replacementPlayer->GetID(),
        InteractionTargetType::STATION, station.GetID(), Position{station.GetX(), station.GetY()}});
    replacements.Update();
    ExpectPendingType(test, replacements, replacementPlayer->GetID(), InteractionTargetType::STATION,
                      "NPC to station leaves only station pending");
    QueueTalk(replacements, replacementPlayer->GetID());
    replacements.Update();
    replacements.ClearMovementPath(replacementPlayer->GetID());
    replacementPlayer->GetPosition().SetPosition(0, 0);
    QueueTalk(replacements, replacementPlayer->GetID());
    replacements.Update();
    ExpectPendingType(test, replacements, replacementPlayer->GetID(), InteractionTargetType::NPC,
                      "NPC to NPC leaves one newest pending TALK");
    replacements.EnqueueCommand(AttackCommand{replacementPlayer->GetID(), 2});
    replacements.Update();
    test.Expect(!WorldTestAccess::Pending(replacements, replacementPlayer->GetID()).has_value(),
                "NPC to melee clears TALK");
    QueueTalk(replacements, replacementPlayer->GetID());
    replacements.Update();
    replacements.EnqueueCommand(MoveCommand{replacementPlayer->GetID(), Position{1, 0}});
    replacements.Update();
    test.Expect(!WorldTestAccess::Pending(replacements, replacementPlayer->GetID()).has_value(),
                "NPC to manual movement clears TALK");
    test.Expect(replacements.GetNpcTalkEvents().empty(), "Replacements emit no stale TALK");

    World stationToNpc;
    Player *stationPlayer = CreatePlayerAt(stationToNpc, 0, 0);
    stationToNpc.QueueStationInteraction(stationPlayer->GetID(), stationToNpc.GetStations().front().GetID());
    QueueTalk(stationToNpc, stationPlayer->GetID());
    stationToNpc.Update();
    ExpectPendingType(test, stationToNpc, stationPlayer->GetID(), InteractionTargetType::NPC,
                      "Station to NPC leaves only NPC pending");

    World unreachable;
    Player *unreachablePlayer = CreatePlayerAt(unreachable, 0, 0);
    for (int y = 2; y <= 4; ++y)
        for (int x = 2; x <= 4; ++x)
            if (x != 3 || y != 3)
                unreachable.GetMap().SetTileType(x, y, TileType::WALL);
    QueueTalk(unreachable, unreachablePlayer->GetID());
    unreachable.Update();
    test.Expect(unreachable.GetNpcTalkEvents().empty(), "Unreachable guide emits no TALK");
    test.Expect(!unreachable.HasActiveMovementPath(unreachablePlayer->GetID()),
                "Unreachable request creates no movement path");
    test.Expect(!WorldTestAccess::Pending(unreachable, unreachablePlayer->GetID()).has_value(),
                "Unreachable request clears pending state");

    World removal;
    Player *removalPlayer = CreatePlayerAt(removal, 0, 0);
    QueueTalk(removal, removalPlayer->GetID());
    removal.Update();
    test.Expect(removal.HasActiveMovementPath(removalPlayer->GetID()), "Removal fixture has approach path");
    test.Expect(WorldTestAccess::Remove(removal, 1), "Permanent guide removal succeeds");
    test.Expect(!removal.HasActiveMovementPath(removalPlayer->GetID()), "Removal cancels targeting path");
    test.Expect(!WorldTestAccess::Pending(removal, removalPlayer->GetID()).has_value(),
                "Removal clears targeting interaction");
    test.Expect(removal.GetEntityByID(1) == nullptr, "Removed guide leaves entity lookup");
    test.ExpectEqual(removal.GetNpcSpawnManager().GetActiveCount(
                         NpcSpawnId::DEVELOPMENT_GUIDE_SPAWN), 0,
                     "Removal releases guide capacity");
    test.Expect(!WorldTestAccess::Remove(removal, 1), "Repeated removal is safe");
    const NpcSpawnDefinition *guideSpawn = NpcSpawnDatabase::TryGet(
        NpcSpawnId::DEVELOPMENT_GUIDE_SPAWN);
    test.Expect(removal.CreateNpc(NpcType::DEVELOPMENT_GUIDE, *guideSpawn) != 0,
                "Replacement guide can use released capacity");
    removal.Update();
    test.Expect(removal.GetNpcTalkEvents().empty(), "Removed target never emits stale TALK");

    auto combatRandom = std::make_unique<CountingRandomSource>();
    CountingRandomSource *combatCounter = combatRandom.get();
    World combatIsolation(std::move(combatRandom),
        std::make_unique<CountingRandomSource>(), std::make_unique<CountingRandomSource>());
    Player *combatPlayer = CreatePlayerAt(combatIsolation, 2, 3);
    QueueTalk(combatIsolation, combatPlayer->GetID());
    combatIsolation.Update();
    test.ExpectEqual(combatCounter->calls, 0, "TALK consumes zero combat RNG");
    combatIsolation.EnqueueCommand(AttackCommand{combatPlayer->GetID(), 1});
    combatIsolation.Update();
    test.ExpectEqual(combatCounter->calls, 0, "Rejected guide attack consumes zero combat RNG");
    test.Expect(!combatIsolation.HasPendingMeleeEngagement(combatPlayer->GetID()),
                "Rejected guide attack leaves no melee engagement");
    test.Expect(combatIsolation.GetLastMeleeAttackResult() == std::nullopt,
                "Rejected guide attack produces no combat result");
    test.Expect(combatIsolation.GetMeleeCombatFeedbacks().empty(),
                "Rejected guide attack produces no feedback");
    test.Expect(combatIsolation.GetEntityDiedEvents().empty(),
                "Rejected guide attack produces no death event");

    World ordering;
    Player *later = CreatePlayerAt(ordering, 4, 3);
    Player *earlier = CreatePlayerAt(ordering, 2, 3);
    QueueTalk(ordering, later->GetID());
    QueueTalk(ordering, earlier->GetID());
    ordering.Update();
    test.ExpectEqual(ordering.GetNpcTalkEvents().size(), std::size_t{2},
                     "Two adjacent actors produce two events");
    test.ExpectEqual(ordering.GetNpcTalkEvents()[0].actorEntityID, later->GetID(),
                     "Multiple events follow deterministic entity creation order");
    test.ExpectEqual(ordering.GetNpcTalkEvents()[1].actorEntityID, earlier->GetID(),
                     "Command enqueue order does not destabilize event order");

    World rejected;
    Player *rejectedPlayer = CreatePlayerAt(rejected, 0, 0);
    rejected.EnqueueCommand(NpcInteractionCommand{
        rejectedPlayer->GetID(), 2, NpcInteractionType::TALK});
    rejected.Update();
    test.Expect(rejected.GetNpcTalkEvents().empty(), "Monster TALK is rejected");
    rejected.EnqueueCommand(NpcInteractionCommand{
        rejectedPlayer->GetID(), 1, static_cast<NpcInteractionType>(999)});
    rejected.Update();
    test.Expect(rejected.GetCommandProcessingResults().front().resultCode ==
                    CommandResultCode::INVALID_COMMAND_DATA,
                "Unknown interaction type is invalid command data");

    return test.Finish();
}

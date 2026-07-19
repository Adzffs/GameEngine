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
static_assert(std::is_same_v<decltype(DialogueContinueCommand::sessionId), DialogueSessionId>);
static_assert(std::is_same_v<decltype(DialogueCloseCommand::sessionId), DialogueSessionId>);

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

    DialogueSessionId StartAdjacentDialogue(World &world, Player &player)
    {
        player.GetPosition().SetPosition(2, 3);
        QueueTalk(world, player.GetID());
        world.Update();
        return world.GetNpcTalkEvents().front().sessionId;
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
    test.Expect(event.sessionId != InvalidDialogueSessionId, "Event exposes nonzero session ID");
    test.Expect(event.dialogueId == DialogueId::DEVELOPMENT_GUIDE_INTRO,
                "Event exposes stable dialogue ID");
    test.Expect(event.nodeId == DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
                "Tick one publishes welcome node");
    test.Expect(event.text == "Welcome to the development world.", "Event copies server-owned text");
    test.Expect(!event.isTerminal, "Welcome node is nonterminal");
    test.Expect(!WorldTestAccess::Pending(nearby, nearbyPlayer->GetID()).has_value(),
                "Completion clears pending TALK");
    const DialogueSessionId nearbySessionId = event.sessionId;
    nearby.EnqueueCommand(DialogueContinueCommand{nearbyPlayer->GetID(), nearbySessionId});
    nearby.Update();
    test.ExpectEqual(nearby.GetCurrentTick(), 2, "First CONTINUE processes on tick two");
    test.ExpectEqual(nearby.GetNpcTalkEvents().size(), std::size_t{1},
                     "Tick two publishes one node");
    test.Expect(nearby.GetNpcTalkEvents()[0].nodeId ==
                    DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION,
                "Tick two publishes explanation node");
    test.Expect(nearby.GetNpcTalkEvents()[0].text ==
                    "This area is used to test gathering, combat, and NPC systems.",
                "Explanation text is authoritative");
    nearby.EnqueueCommand(DialogueContinueCommand{nearbyPlayer->GetID(), nearbySessionId});
    nearby.Update();
    test.ExpectEqual(nearby.GetCurrentTick(), 3, "Second CONTINUE processes on tick three");
    test.Expect(nearby.GetNpcTalkEvents()[0].nodeId ==
                    DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE,
                "Tick three publishes terminal node");
    test.Expect(nearby.GetNpcTalkEvents()[0].text ==
                    "More adventures will be added as the world grows.",
                "Terminal text is authoritative");
    test.Expect(nearby.GetNpcTalkEvents()[0].isTerminal, "Third node is terminal");
    test.Expect(nearby.GetActiveDialogueSession(nearbyPlayer->GetID()) == nullptr,
                "Terminal publication closes session");
    nearby.EnqueueCommand(DialogueContinueCommand{nearbyPlayer->GetID(), nearbySessionId});
    nearby.Update();
    test.Expect(nearby.GetNpcTalkEvents().empty(), "Stale tick-four CONTINUE publishes nothing");

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
    const DialogueSessionId distantSessionId = distant.GetNpcTalkEvents()[0].sessionId;
    distant.EnqueueCommand(DialogueContinueCommand{distantPlayer->GetID(), distantSessionId});
    distant.Update();
    test.ExpectEqual(distant.GetCurrentTick(), 6, "Distant explanation publishes on tick six");
    test.Expect(distant.GetNpcTalkEvents()[0].nodeId ==
                    DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION,
                "Distant tick six is node two");
    distant.EnqueueCommand(DialogueContinueCommand{distantPlayer->GetID(), distantSessionId});
    distant.Update();
    test.ExpectEqual(distant.GetCurrentTick(), 7, "Distant terminal publishes on tick seven");
    test.Expect(distant.GetNpcTalkEvents()[0].nodeId == DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE,
                "Distant tick seven is terminal node");
    test.Expect(distant.GetActiveDialogueSession(distantPlayer->GetID()) == nullptr,
                "Distant session closes on tick seven");

    World duplicateContinue;
    Player *duplicatePlayer = CreatePlayerAt(duplicateContinue, 2, 3);
    QueueTalk(duplicateContinue, duplicatePlayer->GetID());
    duplicateContinue.Update();
    const DialogueSessionId duplicateSession = duplicateContinue.GetNpcTalkEvents()[0].sessionId;
    duplicateContinue.EnqueueCommand(DialogueContinueCommand{duplicatePlayer->GetID(), duplicateSession});
    duplicateContinue.EnqueueCommand(DialogueContinueCommand{duplicatePlayer->GetID(), duplicateSession});
    duplicateContinue.Update();
    test.ExpectEqual(duplicateContinue.GetNpcTalkEvents().size(), std::size_t{1},
                     "Duplicate same-tick CONTINUE emits one node");
    test.Expect(duplicateContinue.GetNpcTalkEvents()[0].nodeId ==
                    DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION,
                "Duplicate CONTINUE cannot skip to terminal");

    World cancellations;
    Player *cancelPlayer = CreatePlayerAt(cancellations, 2, 3);
    DialogueSessionId cancelSession = StartAdjacentDialogue(cancellations, *cancelPlayer);
    cancellations.EnqueueCommand(MoveCommand{cancelPlayer->GetID(), Position{1, 3}});
    cancellations.Update();
    test.Expect(cancellations.GetActiveDialogueSession(cancelPlayer->GetID()) == nullptr,
                "Manual movement cancels active dialogue");
    cancelSession = StartAdjacentDialogue(cancellations, *cancelPlayer);
    cancellations.QueueMovementDestination(MovementDestinationRequest(
        cancelPlayer->GetID(), 1, 3));
    test.Expect(cancellations.GetActiveDialogueSession(cancelPlayer->GetID()) == nullptr,
                "Direct movement destination cancels active dialogue");
    cancelSession = StartAdjacentDialogue(cancellations, *cancelPlayer);
    const ResourceNode &cancelResource = cancellations.GetResources().front();
    cancellations.EnqueueCommand(InteractCommand{cancelPlayer->GetID(),
        InteractionTargetType::RESOURCE, cancelResource.GetID(),
        Position{cancelResource.GetX(), cancelResource.GetY()}});
    cancellations.Update();
    test.Expect(cancellations.GetActiveDialogueSession(cancelPlayer->GetID()) == nullptr,
                "Resource interaction cancels active dialogue");
    cancelSession = StartAdjacentDialogue(cancellations, *cancelPlayer);
    const CraftingStation &cancelStation = cancellations.GetStations().front();
    cancellations.EnqueueCommand(InteractCommand{cancelPlayer->GetID(),
        InteractionTargetType::STATION, cancelStation.GetID(),
        Position{cancelStation.GetX(), cancelStation.GetY()}});
    cancellations.Update();
    test.Expect(cancellations.GetActiveDialogueSession(cancelPlayer->GetID()) == nullptr,
                "Station interaction cancels active dialogue");
    cancelSession = StartAdjacentDialogue(cancellations, *cancelPlayer);
    QueueTalk(cancellations, cancelPlayer->GetID());
    cancellations.Update();
    const DialogueSessionId replacementSession =
        cancellations.GetNpcTalkEvents().front().sessionId;
    test.Expect(replacementSession != cancelSession,
                "New NPC interaction replaces session with fresh identity");
    cancellations.EnqueueCommand(DialogueCloseCommand{cancelPlayer->GetID(), cancelSession});
    cancellations.Update();
    test.Expect(cancellations.GetActiveDialogueSession(cancelPlayer->GetID()) != nullptr,
                "Stale CLOSE does not close replacement session");
    cancellations.EnqueueCommand(DialogueCloseCommand{cancelPlayer->GetID(), replacementSession});
    cancellations.Update();
    test.Expect(cancellations.GetActiveDialogueSession(cancelPlayer->GetID()) == nullptr,
                "Matching CLOSE cancels session");
    cancelSession = StartAdjacentDialogue(cancellations, *cancelPlayer);
    cancelPlayer->GetPosition().SetPosition(0, 0);
    cancellations.EnqueueCommand(DialogueContinueCommand{cancelPlayer->GetID(), cancelSession});
    cancellations.Update();
    test.Expect(cancellations.GetActiveDialogueSession(cancelPlayer->GetID()) == nullptr,
                "Out-of-range CONTINUE cancels session");
    test.Expect(cancellations.GetNpcTalkEvents().empty(),
                "Out-of-range cancellation emits no node");
    cancelSession = StartAdjacentDialogue(cancellations, *cancelPlayer);
    NPC *movingGuide = dynamic_cast<NPC *>(cancellations.GetEntityByID(1));
    movingGuide->GetPosition().SetPosition(8, 8);
    cancellations.EnqueueCommand(DialogueContinueCommand{cancelPlayer->GetID(), cancelSession});
    cancellations.Update();
    test.Expect(cancellations.GetActiveDialogueSession(cancelPlayer->GetID()) == nullptr,
                "NPC moving away causes CONTINUE to cancel");
    movingGuide->GetPosition().SetPosition(3, 3);
    cancelSession = StartAdjacentDialogue(cancellations, *cancelPlayer);
    cancellations.EnqueueCommand(AttackCommand{cancelPlayer->GetID(), 2});
    cancellations.Update();
    test.Expect(cancellations.GetActiveDialogueSession(cancelPlayer->GetID()) == nullptr,
                "Melee request cancels active dialogue");

    World actorDeath;
    Player *doomedPlayer = CreatePlayerAt(actorDeath, 2, 3);
    StartAdjacentDialogue(actorDeath, *doomedPlayer);
    doomedPlayer->ApplyDamage(doomedPlayer->GetMaximumHealth());
    const DialogueSessionId doomedSession =
        actorDeath.GetActiveDialogueSession(doomedPlayer->GetID())->sessionId;
    actorDeath.EnqueueCommand(DialogueContinueCommand{doomedPlayer->GetID(), doomedSession});
    actorDeath.Update();
    test.Expect(actorDeath.GetActiveDialogueSession(doomedPlayer->GetID()) == nullptr,
                "Actor death cancels active dialogue");
    test.Expect(actorDeath.GetNpcTalkEvents().empty(),
                "Dead actor queued CONTINUE publishes no node");

    World activeRemoval;
    Player *activeRemovalPlayer = CreatePlayerAt(activeRemoval, 2, 3);
    const int activeRemovalPlayerId = activeRemovalPlayer->GetID();
    StartAdjacentDialogue(activeRemoval, *activeRemovalPlayer);
    test.Expect(WorldTestAccess::Remove(activeRemoval, activeRemovalPlayerId),
                "Actor removal succeeds");
    test.Expect(activeRemoval.GetActiveDialogueSession(activeRemovalPlayerId) == nullptr,
                "Actor removal cancels active dialogue");

    World activeTargetRemoval;
    Player *targetRemovalA = CreatePlayerAt(activeTargetRemoval, 2, 3);
    Player *targetRemovalB = CreatePlayerAt(activeTargetRemoval, 4, 3);
    StartAdjacentDialogue(activeTargetRemoval, *targetRemovalA);
    StartAdjacentDialogue(activeTargetRemoval, *targetRemovalB);
    const NpcTalkEvent copiedBeforeRemoval = activeTargetRemoval.GetNpcTalkEvents().front();
    test.Expect(WorldTestAccess::Remove(activeTargetRemoval, 1),
                "Active dialogue target removal succeeds");
    test.Expect(activeTargetRemoval.GetActiveDialogueSession(targetRemovalA->GetID()) == nullptr &&
                activeTargetRemoval.GetActiveDialogueSession(targetRemovalB->GetID()) == nullptr,
                "NPC removal cancels every targeting session");
    test.Expect(copiedBeforeRemoval.text == "Welcome to the development world.",
                "NPC removal cannot invalidate an already copied event");

    World invalidIntentions;
    Player *validSessionPlayer = CreatePlayerAt(invalidIntentions, 2, 3);
    const DialogueSessionId validSession =
        StartAdjacentDialogue(invalidIntentions, *validSessionPlayer);
    auto SessionSurvives = [&]()
    {
        const ActiveDialogueSession *active = invalidIntentions.GetActiveDialogueSession(
            validSessionPlayer->GetID());
        return active != nullptr && active->sessionId == validSession;
    };
    invalidIntentions.EnqueueCommand(InteractCommand{validSessionPlayer->GetID(),
        InteractionTargetType::RESOURCE, 999999, Position{5, 5}});
    invalidIntentions.Update();
    test.Expect(SessionSurvives(), "Invalid resource target preserves valid session");
    invalidIntentions.EnqueueCommand(InteractCommand{validSessionPlayer->GetID(),
        InteractionTargetType::STATION, 999999, Position{14, 9}});
    invalidIntentions.Update();
    test.Expect(SessionSurvives(), "Invalid station target preserves valid session");
    invalidIntentions.EnqueueCommand(NpcInteractionCommand{
        validSessionPlayer->GetID(), 999999, NpcInteractionType::TALK});
    invalidIntentions.Update();
    test.Expect(SessionSurvives(), "Invalid NPC target preserves valid session");
    invalidIntentions.EnqueueCommand(NpcInteractionCommand{
        validSessionPlayer->GetID(), 1, static_cast<NpcInteractionType>(999)});
    invalidIntentions.Update();
    test.Expect(SessionSurvives(), "Unsupported NPC interaction preserves valid session");
    invalidIntentions.EnqueueCommand(MoveCommand{
        validSessionPlayer->GetID(), Position{-1, -1}});
    invalidIntentions.Update();
    test.Expect(SessionSurvives(), "Invalid movement destination preserves valid session");
    invalidIntentions.EnqueueCommand(AttackCommand{validSessionPlayer->GetID(), 1});
    invalidIntentions.Update();
    test.Expect(SessionSurvives(), "Rejected melee target preserves valid session");
    invalidIntentions.EnqueueCommand(DialogueContinueCommand{
        validSessionPlayer->GetID(), validSession + 100});
    invalidIntentions.EnqueueCommand(DialogueCloseCommand{
        validSessionPlayer->GetID(), validSession + 100});
    invalidIntentions.Update();
    test.Expect(SessionSurvives(), "Stale dialogue commands preserve valid session");

    World unreachableReplacement;
    Player *unreachableReplacementPlayer = CreatePlayerAt(unreachableReplacement, 2, 3);
    const DialogueSessionId preservedSession =
        StartAdjacentDialogue(unreachableReplacement, *unreachableReplacementPlayer);
    unreachableReplacementPlayer->GetPosition().SetPosition(0, 0);
    for (int y = 2; y <= 4; ++y)
        for (int x = 2; x <= 4; ++x)
            if (x != 3 || y != 3)
                unreachableReplacement.GetMap().SetTileType(x, y, TileType::WALL);
    QueueTalk(unreachableReplacement, unreachableReplacementPlayer->GetID());
    unreachableReplacement.Update();
    test.Expect(unreachableReplacement.GetActiveDialogueSession(
                    unreachableReplacementPlayer->GetID()) != nullptr &&
                unreachableReplacement.GetActiveDialogueSession(
                    unreachableReplacementPlayer->GetID())->sessionId == preservedSession,
                "Unreachable replacement TALK preserves active session");

    World continueThenClose;
    Player *continueClosePlayer = CreatePlayerAt(continueThenClose, 2, 3);
    DialogueSessionId orderSession = StartAdjacentDialogue(
        continueThenClose, *continueClosePlayer);
    continueThenClose.EnqueueCommand(DialogueContinueCommand{
        continueClosePlayer->GetID(), orderSession});
    continueThenClose.EnqueueCommand(DialogueCloseCommand{
        continueClosePlayer->GetID(), orderSession});
    continueThenClose.Update();
    test.ExpectEqual(continueThenClose.GetNpcTalkEvents().size(), std::size_t{1},
                     "CONTINUE then CLOSE publishes the continued node once");
    test.Expect(continueThenClose.GetActiveDialogueSession(
                    continueClosePlayer->GetID()) == nullptr,
                "CONTINUE then CLOSE ends the session");

    World closeThenContinue;
    Player *closeContinuePlayer = CreatePlayerAt(closeThenContinue, 2, 3);
    orderSession = StartAdjacentDialogue(closeThenContinue, *closeContinuePlayer);
    closeThenContinue.EnqueueCommand(DialogueCloseCommand{
        closeContinuePlayer->GetID(), orderSession});
    closeThenContinue.EnqueueCommand(DialogueContinueCommand{
        closeContinuePlayer->GetID(), orderSession});
    closeThenContinue.Update();
    test.Expect(closeThenContinue.GetNpcTalkEvents().empty(),
                "CLOSE then CONTINUE publishes no node");
    test.Expect(closeThenContinue.GetActiveDialogueSession(
                    closeContinuePlayer->GetID()) == nullptr,
                "CLOSE then CONTINUE remains closed");

    World talkThenStaleContinue;
    Player *talkStalePlayer = CreatePlayerAt(talkThenStaleContinue, 2, 3);
    const DialogueSessionId oldTalkSession = StartAdjacentDialogue(
        talkThenStaleContinue, *talkStalePlayer);
    QueueTalk(talkThenStaleContinue, talkStalePlayer->GetID());
    talkThenStaleContinue.EnqueueCommand(DialogueContinueCommand{
        talkStalePlayer->GetID(), oldTalkSession});
    talkThenStaleContinue.Update();
    test.ExpectEqual(talkThenStaleContinue.GetNpcTalkEvents().size(), std::size_t{1},
                     "New TALK then stale CONTINUE publishes only new welcome");
    test.Expect(talkThenStaleContinue.GetNpcTalkEvents()[0].nodeId ==
                    DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
                "New TALK remains authoritative over later stale CONTINUE");

    World continueThenTalk;
    Player *continueTalkPlayer = CreatePlayerAt(continueThenTalk, 2, 3);
    const DialogueSessionId continuedOldSession = StartAdjacentDialogue(
        continueThenTalk, *continueTalkPlayer);
    continueThenTalk.EnqueueCommand(DialogueContinueCommand{
        continueTalkPlayer->GetID(), continuedOldSession});
    QueueTalk(continueThenTalk, continueTalkPlayer->GetID());
    continueThenTalk.Update();
    test.ExpectEqual(continueThenTalk.GetNpcTalkEvents().size(), std::size_t{2},
                     "Old CONTINUE then new TALK publishes both accepted commands in order");
    test.Expect(continueThenTalk.GetNpcTalkEvents()[0].nodeId ==
                    DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION &&
                continueThenTalk.GetNpcTalkEvents()[1].nodeId ==
                    DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
                "Old continuation event precedes replacement welcome event");

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
    DialogueSessionId combatSession = combatIsolation.GetNpcTalkEvents()[0].sessionId;
    combatIsolation.EnqueueCommand(DialogueContinueCommand{combatPlayer->GetID(), combatSession});
    combatIsolation.Update();
    test.ExpectEqual(combatCounter->calls, 0, "CONTINUE consumes zero combat RNG");
    combatIsolation.EnqueueCommand(DialogueCloseCommand{combatPlayer->GetID(), combatSession});
    combatIsolation.Update();
    test.ExpectEqual(combatCounter->calls, 0, "CLOSE consumes zero combat RNG");
    combatSession = StartAdjacentDialogue(combatIsolation, *combatPlayer);
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
    const DialogueSessionId laterSession = ordering.GetNpcTalkEvents()[0].sessionId;
    const DialogueSessionId earlierSession = ordering.GetNpcTalkEvents()[1].sessionId;
    test.Expect(laterSession != earlierSession, "Concurrent actors receive unique sessions");
    ordering.EnqueueCommand(DialogueContinueCommand{earlier->GetID(), earlierSession});
    ordering.Update();
    test.Expect(ordering.GetActiveDialogueSession(later->GetID()) != nullptr &&
                ordering.GetActiveDialogueSession(later->GetID())->currentNodeId ==
                    DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
                "One actor advancing leaves the other session unchanged");
    test.Expect(ordering.GetActiveDialogueSession(earlier->GetID())->currentNodeId ==
                    DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION,
                "Second actor progresses independently");
    ordering.EnqueueCommand(DialogueContinueCommand{earlier->GetID(), earlierSession});
    ordering.EnqueueCommand(DialogueContinueCommand{later->GetID(), laterSession});
    ordering.Update();
    test.ExpectEqual(ordering.GetNpcTalkEvents().size(), std::size_t{2},
                     "Different actors may each advance once in one update");
    test.Expect(ordering.GetNpcTalkEvents()[0].actorEntityID == earlier->GetID() &&
                    ordering.GetNpcTalkEvents()[1].actorEntityID == later->GetID(),
                "CONTINUE event ordering follows deterministic command queue order");

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

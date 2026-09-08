#include "TestSupport.h"
#include "../src/Dialogue/DialoguePresentationState.h"
#include "../src/Player/Player.h"
#include "../src/World/World.h"

namespace
{
    Player* CreatePlayer(World& world)
    {
        auto* player = dynamic_cast<Player*>(
            world.GetEntityByID(world.CreatePlayer()));
        player->GetPosition().SetPosition(2, 3);
        return player;
    }

    void Open(World& world, Player& player)
    {
        world.EnqueueCommand(NpcInteractionCommand{
            player.GetID(), 1, NpcInteractionType::TALK});
        world.Update();
    }

    void Reopen(World& world, Player& player)
    {
        const auto* session = world.GetActiveDialogueSession(player.GetID());
        if (session != nullptr)
        {
            world.EnqueueCommand(DialogueCloseCommand{
                player.GetID(), session->sessionId});
            world.Update();
        }
        Open(world, player);
    }

    const NpcTalkEvent* LocalEvent(const World& world, int actor)
    {
        for (const auto& event : world.GetNpcTalkEvents())
            if (event.actorEntityID == actor)
                return &event;
        return nullptr;
    }
}

int main()
{
    TestContext test;
    World world(1U, 2U, 3U);
    Player* player = CreatePlayer(world);
    DialoguePresentationState presentation;

    Open(world, *player);
    const NpcTalkEvent* available = LocalEvent(world, player->GetID());
    const ActiveDialogueSession* session =
        world.GetActiveDialogueSession(player->GetID());
    test.Expect(available != nullptr &&
                    available->nodeId == DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME &&
                    available->questAction == QuestDialogueAction::ACCEPT_GATHERING_BASICS,
                "Real World opening publishes Accept for AVAILABLE");
    presentation.Synchronize(player->GetID(), world.GetNpcTalkEvents(), session);
    auto accept = presentation.MakeQuestCommand();
    const auto* acceptCommand = accept
        ? std::get_if<QuestAcceptCommand>(&*accept) : nullptr;
    test.Expect(acceptCommand != nullptr &&
                    acceptCommand->actorEntityID == player->GetID() &&
                    acceptCommand->npcEntityID == 1 &&
                    acceptCommand->dialogueSessionId == session->sessionId &&
                    acceptCommand->questId == QuestId::GATHERING_BASICS,
                "Client presentation forwards exact authoritative Accept identity");

    world.EnqueueCommand(*accept);
    world.Update();
    Reopen(world, *player);
    const NpcTalkEvent* active = LocalEvent(world, player->GetID());
    test.Expect(active != nullptr &&
                    active->questAction == QuestDialogueAction::NONE,
                "Real World opening publishes no quest action for ACTIVE");

    player->GetQuestJournal().Get() = {
        QuestId::GATHERING_BASICS, QuestState::READY_TO_COMPLETE, 10};
    Reopen(world, *player);
    const NpcTalkEvent* ready = LocalEvent(world, player->GetID());
    session = world.GetActiveDialogueSession(player->GetID());
    test.Expect(ready != nullptr &&
                    ready->questAction == QuestDialogueAction::COMPLETE_GATHERING_BASICS,
                "Real World opening publishes Complete for READY_TO_COMPLETE");
    presentation.Synchronize(player->GetID(), world.GetNpcTalkEvents(), session);
    auto complete = presentation.MakeQuestCommand();
    const auto* completeCommand = complete
        ? std::get_if<QuestCompleteCommand>(&*complete) : nullptr;
    test.Expect(completeCommand != nullptr &&
                    completeCommand->actorEntityID == player->GetID() &&
                    completeCommand->npcEntityID == 1 &&
                    completeCommand->dialogueSessionId == session->sessionId &&
                    completeCommand->questId == QuestId::GATHERING_BASICS,
                "Client presentation forwards exact authoritative Complete identity");

    player->GetQuestJournal().Get().state = QuestState::COMPLETED;
    Reopen(world, *player);
    const NpcTalkEvent* completed = LocalEvent(world, player->GetID());
    test.Expect(completed != nullptr &&
                    completed->questAction == QuestDialogueAction::NONE,
                "Real World opening publishes no quest action for COMPLETED");

    session = world.GetActiveDialogueSession(player->GetID());
    world.EnqueueCommand(DialogueContinueCommand{
        player->GetID(), session->sessionId});
    world.Update();
    const NpcTalkEvent* explanation = LocalEvent(world, player->GetID());
    test.Expect(explanation != nullptr &&
                    explanation->nodeId != DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME &&
                    explanation->questAction == QuestDialogueAction::NONE,
                "Non-opening World dialogue node publishes no quest action");

    const DialogueSessionId terminalPathSession =
        world.GetActiveDialogueSession(player->GetID())->sessionId;
    world.EnqueueCommand(DialogueContinueCommand{
        player->GetID(), terminalPathSession});
    world.Update();
    const NpcTalkEvent* choice = LocalEvent(world, player->GetID());
    test.Expect(choice != nullptr && !choice->choices.empty(),
                "Real World dialogue publishes its authored branch choices");
    world.EnqueueCommand(DialogueChooseCommand{
        player->GetID(), terminalPathSession, choice->choices.front().id});
    world.Update();
    world.EnqueueCommand(DialogueContinueCommand{
        player->GetID(), terminalPathSession});
    world.Update();
    const NpcTalkEvent* terminal = LocalEvent(world, player->GetID());
    test.Expect(terminal != nullptr && terminal->isTerminal &&
                    terminal->questAction == QuestDialogueAction::NONE,
                "Real World terminal node publishes no quest action");

    return test.Finish();
}

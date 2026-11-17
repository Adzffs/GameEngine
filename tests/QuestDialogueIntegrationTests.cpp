#include "TestSupport.h"
#include "../src/Dialogue/DialoguePresentationState.h"
#include "../src/NPC/NPC.h"
#include "../src/Player/Player.h"
#include "../src/Quest/QuestSystem.h"
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

    NPC* FindNpc(World& world, NpcType type)
    {
        for (const auto& entity : world.GetEntities())
        {
            auto* npc = dynamic_cast<NPC*>(entity.get());
            if (npc != nullptr && npc->GetNpcType() == type) return npc;
        }
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
                    available->questActions.size() == 3 &&
                    available->questActions.front().kind == QuestDialogueActionKind::ACCEPT &&
                    available->questActions.front().questId == QuestId::GATHERING_BASICS,
                "Real World opening publishes Accept for AVAILABLE");
    presentation.Synchronize(player->GetID(), world.GetNpcTalkEvents(), session);
    auto accept = presentation.MakeQuestCommand(0);
    const auto miningAccept = presentation.MakeQuestCommand(1);
    const auto* miningAcceptCommand = miningAccept
        ? std::get_if<QuestAcceptCommand>(&*miningAccept) : nullptr;
    test.Expect(miningAcceptCommand != nullptr &&
                    miningAcceptCommand->questId == QuestId::MINING_BASICS,
                "Second authoritative action forwards Mining Basics");
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
                    active->questActions.size() == 2 &&
                    active->questActions.front().questId == QuestId::MINING_BASICS,
                "Real World opening retains independent Mining acceptance");

    QuestSystem::TryRestore(player->GetQuestJournal(), {
        {QuestId::GATHERING_BASICS, QuestState::READY_TO_COMPLETE, 10},
        {QuestId::MINING_BASICS, QuestState::ACTIVE, 0},
        {QuestId::TRACKS_IN_THE_ASH, QuestState::AVAILABLE, 0},
        {QuestId::SHIELD_OR_SHELTER, QuestState::AVAILABLE, 0}, {QuestId::FANGS_AT_THE_GATE, QuestState::AVAILABLE, 0}, {QuestId::HEART_OF_THE_ASH, QuestState::AVAILABLE, 0}});
    Reopen(world, *player);
    const NpcTalkEvent* ready = LocalEvent(world, player->GetID());
    session = world.GetActiveDialogueSession(player->GetID());
    test.Expect(ready != nullptr &&
                    ready->questActions.size() == 3 &&
                    ready->questActions.front().kind == QuestDialogueActionKind::COMPLETE,
                "Real World opening publishes Complete for READY_TO_COMPLETE");
    presentation.Synchronize(player->GetID(), world.GetNpcTalkEvents(), session);
    auto complete = presentation.MakeQuestCommand(0);
    const auto* completeCommand = complete
        ? std::get_if<QuestCompleteCommand>(&*complete) : nullptr;
    test.Expect(completeCommand != nullptr &&
                    completeCommand->actorEntityID == player->GetID() &&
                    completeCommand->npcEntityID == 1 &&
                    completeCommand->dialogueSessionId == session->sessionId &&
                    completeCommand->questId == QuestId::GATHERING_BASICS,
                "Client presentation forwards exact authoritative Complete identity");

    QuestSystem::TryRestore(player->GetQuestJournal(), {
        {QuestId::GATHERING_BASICS, QuestState::COMPLETED, 10},
        {QuestId::MINING_BASICS, QuestState::ACTIVE, 0},
        {QuestId::TRACKS_IN_THE_ASH, QuestState::AVAILABLE, 0},
        {QuestId::SHIELD_OR_SHELTER, QuestState::AVAILABLE, 0}, {QuestId::FANGS_AT_THE_GATE, QuestState::AVAILABLE, 0}, {QuestId::HEART_OF_THE_ASH, QuestState::AVAILABLE, 0}});
    Reopen(world, *player);
    const NpcTalkEvent* completed = LocalEvent(world, player->GetID());
    test.Expect(completed != nullptr &&
                    completed->questActions.size() == 2 &&
                    completed->questActions.front().questId == QuestId::TRACKS_IN_THE_ASH,
                "Completed Gathering no longer publishes its action");

    session = world.GetActiveDialogueSession(player->GetID());
    world.EnqueueCommand(DialogueContinueCommand{
        player->GetID(), session->sessionId});
    world.Update();
    const NpcTalkEvent* explanation = LocalEvent(world, player->GetID());
    test.Expect(explanation != nullptr &&
                    explanation->nodeId != DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME &&
                    explanation->questActions.empty(),
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
                    terminal->questActions.empty(),
                "Real World terminal node publishes no quest action");

    World investigationWorld(11U, 12U, 13U);
    Player* investigator = CreatePlayer(investigationWorld);
    Open(investigationWorld, *investigator);
    const ActiveDialogueSession* maraSession =
        investigationWorld.GetActiveDialogueSession(investigator->GetID());
    test.Expect(maraSession != nullptr,
                "Mara dialogue opens for the local mystery");
    if (maraSession != nullptr)
    {
        investigationWorld.EnqueueCommand(QuestAcceptCommand{
            investigator->GetID(), 1, maraSession->sessionId,
            QuestId::TRACKS_IN_THE_ASH});
        investigationWorld.Update();
    }
    NPC* sera = FindNpc(investigationWorld, NpcType::SERA_FLINT);
    test.Expect(sera != nullptr &&
                    investigator->GetQuestJournal().TryGet(
                        QuestId::TRACKS_IN_THE_ASH)->state == QuestState::ACTIVE,
                "Mara offers the local mystery as a real quest");
    if (sera != nullptr)
    {
        investigator->GetPosition().SetPosition(
            sera->GetPosition().GetX() - 1, sera->GetPosition().GetY());
        investigationWorld.EnqueueCommand(NpcInteractionCommand{
            investigator->GetID(), sera->GetID(), NpcInteractionType::TALK});
        investigationWorld.Update();
    }
    const QuestRecord* investigated = investigator->GetQuestJournal().TryGet(
        QuestId::TRACKS_IN_THE_ASH);
    test.Expect(investigated != nullptr &&
                    investigated->state == QuestState::READY_TO_COMPLETE &&
                    investigated->progress == 1,
                "Speaking with Sera at the east road solves the investigation");
    NPC* mara = FindNpc(investigationWorld, NpcType::DEVELOPMENT_GUIDE);
    if (mara != nullptr)
    {
        investigator->GetPosition().SetPosition(
            mara->GetPosition().GetX() - 1, mara->GetPosition().GetY());
        investigationWorld.EnqueueCommand(NpcInteractionCommand{
            investigator->GetID(), mara->GetID(), NpcInteractionType::TALK});
        investigationWorld.Update();
    }
    maraSession = investigationWorld.GetActiveDialogueSession(
        investigator->GetID());
    test.Expect(maraSession != nullptr,
                "Mara dialogue reopens after gathering the evidence");
    if (maraSession != nullptr)
    {
        investigationWorld.EnqueueCommand(QuestCompleteCommand{
            investigator->GetID(), 1, maraSession->sessionId,
            QuestId::TRACKS_IN_THE_ASH});
        investigationWorld.Update();
    }
    test.Expect(
                    investigator->GetQuestJournal().TryGet(
                        QuestId::TRACKS_IN_THE_ASH)->state == QuestState::COMPLETED &&
                    investigator->GetInventory().GetItemAmount(ItemType::COINS) == 15,
                "Returning the evidence to Mara completes the mystery and rewards it once");

    const auto resolveCraftingDilemma = [&](NpcType recipientType,
        ItemType expectedReward, int expectedAmount)
    {
        World choiceWorld(21U, 22U, 23U);
        Player* crafter = CreatePlayer(choiceWorld);
        QuestSystem::TryRestore(crafter->GetQuestJournal(), {
            {QuestId::GATHERING_BASICS, QuestState::AVAILABLE, 0},
            {QuestId::MINING_BASICS, QuestState::AVAILABLE, 0},
            {QuestId::TRACKS_IN_THE_ASH, QuestState::COMPLETED, 1},
            {QuestId::SHIELD_OR_SHELTER, QuestState::AVAILABLE, 0},
            {QuestId::FANGS_AT_THE_GATE, QuestState::UNAVAILABLE, 0},
            {QuestId::HEART_OF_THE_ASH, QuestState::UNAVAILABLE, 0}});
        NPC* torren = FindNpc(choiceWorld, NpcType::TORREN_ASH);
        NPC* recipient = FindNpc(choiceWorld, recipientType);
        if (torren == nullptr || recipient == nullptr) return false;
        crafter->GetPosition().SetPosition(
            torren->GetPosition().GetX() - 1, torren->GetPosition().GetY());
        choiceWorld.EnqueueCommand(NpcInteractionCommand{
            crafter->GetID(), torren->GetID(), NpcInteractionType::TALK});
        choiceWorld.Update();
        const ActiveDialogueSession* choiceSession =
            choiceWorld.GetActiveDialogueSession(crafter->GetID());
        if (choiceSession == nullptr) return false;
        choiceWorld.EnqueueCommand(QuestAcceptCommand{
            crafter->GetID(), torren->GetID(), choiceSession->sessionId,
            QuestId::SHIELD_OR_SHELTER});
        choiceWorld.Update();
        QuestSystem::RecordCrafted(crafter->GetQuestJournal(),
            ItemType::WOODEN_SHIELD, 1);
        if (recipientType != NpcType::TORREN_ASH)
        {
            crafter->GetPosition().SetPosition(
                recipient->GetPosition().GetX() - 1,
                recipient->GetPosition().GetY());
            choiceWorld.EnqueueCommand(NpcInteractionCommand{
                crafter->GetID(), recipient->GetID(), NpcInteractionType::TALK});
            choiceWorld.Update();
            choiceSession = choiceWorld.GetActiveDialogueSession(crafter->GetID());
        }
        if (choiceSession == nullptr) return false;
        choiceWorld.EnqueueCommand(QuestCompleteCommand{
            crafter->GetID(), recipient->GetID(), choiceSession->sessionId,
            QuestId::SHIELD_OR_SHELTER});
        choiceWorld.Update();
        const QuestRecord* record = crafter->GetQuestJournal().TryGet(
            QuestId::SHIELD_OR_SHELTER);
        const ItemType otherReward = expectedReward == ItemType::COINS
            ? ItemType::COOKED_MEAT : ItemType::COINS;
        return record != nullptr && record->state == QuestState::COMPLETED &&
            crafter->GetInventory().GetItemAmount(expectedReward) == expectedAmount &&
            crafter->GetInventory().GetItemAmount(otherReward) == 0 &&
            !QuestSystem::Complete(crafter->GetQuestJournal(),
                QuestId::SHIELD_OR_SHELTER, crafter->GetInventory());
    };
    test.Expect(resolveCraftingDilemma(
                    NpcType::TORREN_ASH, ItemType::COINS, 25),
                "Returning the shield plan to Torren chooses the coin outcome");
    test.Expect(resolveCraftingDilemma(
                    NpcType::NESSA_THORNE, ItemType::COOKED_MEAT, 3),
                "Taking the shield plan to Nessa chooses the supply outcome");

    World combatWorld(31U, 32U, 33U);
    Player* defender = CreatePlayer(combatWorld);
    QuestSystem::TryRestore(defender->GetQuestJournal(), {
        {QuestId::GATHERING_BASICS, QuestState::AVAILABLE, 0},
        {QuestId::MINING_BASICS, QuestState::AVAILABLE, 0},
        {QuestId::TRACKS_IN_THE_ASH, QuestState::COMPLETED, 1},
        {QuestId::SHIELD_OR_SHELTER, QuestState::COMPLETED, 1},
        {QuestId::FANGS_AT_THE_GATE, QuestState::AVAILABLE, 0},
        {QuestId::HEART_OF_THE_ASH, QuestState::UNAVAILABLE, 0}});
    NPC* seraCombat = FindNpc(combatWorld, NpcType::SERA_FLINT);
    if (seraCombat != nullptr)
    {
        defender->GetPosition().SetPosition(
            seraCombat->GetPosition().GetX() - 1,
            seraCombat->GetPosition().GetY());
        combatWorld.EnqueueCommand(NpcInteractionCommand{
            defender->GetID(), seraCombat->GetID(), NpcInteractionType::TALK});
        combatWorld.Update();
    }
    const ActiveDialogueSession* combatSession =
        combatWorld.GetActiveDialogueSession(defender->GetID());
    if (combatSession != nullptr && seraCombat != nullptr)
    {
        combatWorld.EnqueueCommand(QuestAcceptCommand{
            defender->GetID(), seraCombat->GetID(), combatSession->sessionId,
            QuestId::FANGS_AT_THE_GATE});
        combatWorld.Update();
    }
    QuestSystem::RecordDefeated(defender->GetQuestJournal(),
        NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER, 3);
    const QuestRecord* combatReady = defender->GetQuestJournal().TryGet(
        QuestId::FANGS_AT_THE_GATE);
    test.Expect(combatReady != nullptr &&
                    combatReady->state == QuestState::READY_TO_COMPLETE,
                "Three authored Ashfang defeats make the combat quest ready");
    if (combatSession != nullptr && seraCombat != nullptr)
    {
        combatWorld.EnqueueCommand(QuestCompleteCommand{
            defender->GetID(), seraCombat->GetID(), combatSession->sessionId,
            QuestId::FANGS_AT_THE_GATE});
        combatWorld.Update();
    }
    test.Expect(defender->GetQuestJournal().TryGet(
                    QuestId::FANGS_AT_THE_GATE)->state == QuestState::COMPLETED &&
                    defender->GetInventory().GetItemAmount(
                        ItemType::COOKED_MEAT) == 5,
                "Reporting to Sera completes the escalation and grants supplies");

    World conclusionWorld(41U, 42U, 43U);
    Player* champion = CreatePlayer(conclusionWorld);
    QuestSystem::TryRestore(champion->GetQuestJournal(), {
        {QuestId::GATHERING_BASICS, QuestState::AVAILABLE, 0},
        {QuestId::MINING_BASICS, QuestState::AVAILABLE, 0},
        {QuestId::TRACKS_IN_THE_ASH, QuestState::COMPLETED, 1},
        {QuestId::SHIELD_OR_SHELTER, QuestState::COMPLETED, 1},
        {QuestId::FANGS_AT_THE_GATE, QuestState::COMPLETED, 3},
        {QuestId::HEART_OF_THE_ASH, QuestState::AVAILABLE, 0}});
    NPC* maraConclusion = FindNpc(
        conclusionWorld, NpcType::DEVELOPMENT_GUIDE);
    NPC* brannConclusion = FindNpc(
        conclusionWorld, NpcType::BRANN_DEEPWELL);
    if (maraConclusion != nullptr)
    {
        champion->GetPosition().SetPosition(
            maraConclusion->GetPosition().GetX() - 1,
            maraConclusion->GetPosition().GetY());
        conclusionWorld.EnqueueCommand(NpcInteractionCommand{
            champion->GetID(), maraConclusion->GetID(),
            NpcInteractionType::TALK});
        conclusionWorld.Update();
    }
    const ActiveDialogueSession* conclusionStart =
        conclusionWorld.GetActiveDialogueSession(champion->GetID());
    if (conclusionStart != nullptr && maraConclusion != nullptr)
    {
        conclusionWorld.EnqueueCommand(QuestAcceptCommand{
            champion->GetID(), maraConclusion->GetID(),
            conclusionStart->sessionId, QuestId::HEART_OF_THE_ASH});
        conclusionWorld.Update();
    }
    QuestSystem::RecordDefeated(champion->GetQuestJournal(),
        NpcType::ELITE_DEVELOPMENT_MONSTER);
    if (brannConclusion != nullptr)
    {
        champion->GetPosition().SetPosition(
            brannConclusion->GetPosition().GetX() - 1,
            brannConclusion->GetPosition().GetY());
        conclusionWorld.EnqueueCommand(NpcInteractionCommand{
            champion->GetID(), brannConclusion->GetID(),
            NpcInteractionType::TALK});
        conclusionWorld.Update();
    }
    const ActiveDialogueSession* conclusionHandIn =
        conclusionWorld.GetActiveDialogueSession(champion->GetID());
    if (conclusionHandIn != nullptr && brannConclusion != nullptr)
    {
        conclusionWorld.EnqueueCommand(QuestCompleteCommand{
            champion->GetID(), brannConclusion->GetID(),
            conclusionHandIn->sessionId, QuestId::HEART_OF_THE_ASH});
        conclusionWorld.Update();
    }
    const QuestRecord* conclusion = champion->GetQuestJournal().TryGet(
        QuestId::HEART_OF_THE_ASH);
    test.Expect(conclusion != nullptr &&
                    conclusion->state == QuestState::COMPLETED &&
                    champion->GetInventory().GetItemAmount(
                        ItemType::STEEL_BAR) == 2 &&
                    champion->GetInventory().GetItemAmount(
                        ItemType::COINS) == 0,
                "Giving Brann the alpha trophy chooses useful steel over coins");

    return test.Finish();
}

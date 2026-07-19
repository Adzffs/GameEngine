#include "TestSupport.h"
#include "../src/Content/ContentValidator.h"
#include "../src/Dialogue/DialogueDefinitionDatabase.h"
#include "../src/Player/Player.h"
#include "../src/World/World.h"

#include <cstddef>
#include <type_traits>

static_assert(std::is_same_v<decltype(DialogueChooseCommand::actorEntityID), int>);
static_assert(std::is_same_v<decltype(DialogueChooseCommand::sessionId), DialogueSessionId>);
static_assert(std::is_same_v<decltype(DialogueChooseCommand::choiceId), DialogueChoiceId>);
static_assert(!std::is_pointer_v<decltype(DialogueEventChoice::text)>);

namespace
{
    Player *CreateAdjacentPlayer(World &world)
    {
        Player *player = dynamic_cast<Player *>(world.GetEntityByID(world.CreatePlayer()));
        player->GetPosition().SetPosition(2, 3);
        return player;
    }

    DialogueSessionId ReachPrompt(World &world, Player &player)
    {
        world.EnqueueCommand(NpcInteractionCommand{
            player.GetID(), 1, NpcInteractionType::TALK});
        world.Update();
        const DialogueSessionId session = world.GetNpcTalkEvents()[0].sessionId;
        world.EnqueueCommand(DialogueContinueCommand{player.GetID(), session});
        world.Update();
        world.EnqueueCommand(DialogueContinueCommand{player.GetID(), session});
        world.Update();
        return session;
    }
}

int main()
{
    TestContext test;
    const DialogueDefinition *authored = DialogueDefinitionDatabase::TryGet(
        DialogueId::DEVELOPMENT_GUIDE_INTRO);
    const DialogueNodeDefinition *prompt = DialogueDefinitionDatabase::TryGetNode(
        DialogueId::DEVELOPMENT_GUIDE_INTRO,
        DialogueNodeId::DEVELOPMENT_GUIDE_TOPIC_PROMPT);
    test.Expect(authored != nullptr && prompt != nullptr, "Authored choice graph resolves");
    test.Expect(DialogueDefinitionDatabase::TryGetChoice(
                    DialogueId::DEVELOPMENT_GUIDE_INTRO,
                    DialogueNodeId::DEVELOPMENT_GUIDE_TOPIC_PROMPT,
                    DialogueChoiceId::NONE) == nullptr,
                "NONE choice fails safely");
    test.Expect(DialogueDefinitionDatabase::TryGetChoice(
                    DialogueId::DEVELOPMENT_GUIDE_INTRO,
                    DialogueNodeId::DEVELOPMENT_GUIDE_TOPIC_PROMPT,
                    static_cast<DialogueChoiceId>(999)) == nullptr,
                "Unknown choice fails safely");
    if (prompt != nullptr)
    {
        test.ExpectEqual(prompt->choices.size(), std::size_t{2}, "Prompt has exactly two choices");
        test.Expect(prompt->choices[0].text == "Tell me about gathering." &&
                        prompt->choices[0].destinationNodeId ==
                            DialogueNodeId::DEVELOPMENT_GUIDE_GATHERING_RESPONSE,
                    "Gathering choice has exact content and destination");
        test.Expect(prompt->choices[1].text == "Tell me about combat." &&
                        prompt->choices[1].destinationNodeId ==
                            DialogueNodeId::DEVELOPMENT_GUIDE_COMBAT_RESPONSE,
                    "Combat choice has exact content and destination");
    }

    if (authored != nullptr)
    {
        DialogueDefinition duplicate = *authored;
        duplicate.nodes[2].choices[1].id = duplicate.nodes[2].choices[0].id;
        test.Expect(!ContentValidator::ValidateDialogueDefinition(duplicate).IsValid(),
                    "Duplicate choice IDs are rejected");
        DialogueDefinition empty = *authored;
        empty.nodes[2].choices[0].text.clear();
        test.Expect(!ContentValidator::ValidateDialogueDefinition(empty).IsValid(),
                    "Empty choice text is rejected");
        DialogueDefinition missing = *authored;
        missing.nodes[2].choices[0].destinationNodeId = static_cast<DialogueNodeId>(999);
        test.Expect(!ContentValidator::ValidateDialogueDefinition(missing).IsValid(),
                    "Unknown choice destination is rejected");
        DialogueDefinition badContinue = *authored;
        badContinue.nodes[0].choices = prompt->choices;
        test.Expect(!ContentValidator::ValidateDialogueDefinition(badContinue).IsValid(),
                    "Continue node with choices is rejected");
        DialogueDefinition shortChoice = *authored;
        shortChoice.nodes[2].choices.pop_back();
        test.Expect(!ContentValidator::ValidateDialogueDefinition(shortChoice).IsValid(),
                    "Choice node with fewer than two choices is rejected");
        DialogueDefinition badTerminal = *authored;
        badTerminal.nodes.back().choices = prompt->choices;
        test.Expect(!ContentValidator::ValidateDialogueDefinition(badTerminal).IsValid(),
                    "Terminal node with choices is rejected");
        DialogueDefinition unknownKind = *authored;
        unknownKind.nodes[0].kind = static_cast<DialogueNodeKind>(999);
        test.Expect(!ContentValidator::ValidateDialogueDefinition(unknownKind).IsValid(),
                    "Unknown node kind is rejected");
        DialogueDefinition continueWithoutSuccessor = *authored;
        continueWithoutSuccessor.nodes[0].nextNodeId.reset();
        test.Expect(!ContentValidator::ValidateDialogueDefinition(
                        continueWithoutSuccessor).IsValid(),
                    "Continue node without successor is rejected");
        DialogueDefinition choiceWithSuccessor = *authored;
        choiceWithSuccessor.nodes[2].nextNodeId = DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE;
        test.Expect(!ContentValidator::ValidateDialogueDefinition(choiceWithSuccessor).IsValid(),
                    "Choice node with automatic successor is rejected");
        DialogueDefinition zeroChoices = *authored;
        zeroChoices.nodes[2].choices.clear();
        test.Expect(!ContentValidator::ValidateDialogueDefinition(zeroChoices).IsValid(),
                    "Choice node with zero choices is rejected");
        DialogueDefinition noneChoice = *authored;
        noneChoice.nodes[2].choices[0].id = DialogueChoiceId::NONE;
        test.Expect(!ContentValidator::ValidateDialogueDefinition(noneChoice).IsValid(),
                    "NONE choice ID is rejected by validation");
        DialogueDefinition unknownChoice = *authored;
        unknownChoice.nodes[2].choices[0].id = static_cast<DialogueChoiceId>(999);
        test.Expect(!ContentValidator::ValidateDialogueDefinition(unknownChoice).IsValid(),
                    "Unknown choice ID is rejected by validation");
        DialogueDefinition terminalWithSuccessor = *authored;
        terminalWithSuccessor.nodes.back().nextNodeId =
            DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME;
        test.Expect(!ContentValidator::ValidateDialogueDefinition(
                        terminalWithSuccessor).IsValid(),
                    "Terminal node with successor is rejected");
        DialogueDefinition branchCycle = *authored;
        branchCycle.nodes[4].nextNodeId =
            DialogueNodeId::DEVELOPMENT_GUIDE_COMBAT_RESPONSE;
        test.Expect(!ContentValidator::ValidateDialogueDefinition(branchCycle).IsValid(),
                    "Cycle in only the combat branch rejects the whole graph");
        DialogueDefinition unreachableBranch = *authored;
        unreachableBranch.nodes[2].choices[1].destinationNodeId =
            DialogueNodeId::DEVELOPMENT_GUIDE_GATHERING_RESPONSE;
        test.Expect(!ContentValidator::ValidateDialogueDefinition(unreachableBranch).IsValid(),
                    "Unreachable authored branch node is rejected");
        DialogueDefinition cycleAfterReconvergence = *authored;
        cycleAfterReconvergence.nodes.back().kind = DialogueNodeKind::CONTINUE;
        cycleAfterReconvergence.nodes.back().nextNodeId =
            DialogueNodeId::DEVELOPMENT_GUIDE_TOPIC_PROMPT;
        test.Expect(!ContentValidator::ValidateDialogueDefinition(
                        cycleAfterReconvergence).IsValid(),
                    "Cycle after branch reconvergence is rejected");
        DialogueDefinition brokenCombatPath = *authored;
        brokenCombatPath.nodes[4].nextNodeId.reset();
        test.Expect(!ContentValidator::ValidateDialogueDefinition(brokenCombatPath).IsValid(),
                    "One branch without a terminal path rejects the whole graph");
    }

    World invalidThenValid;
    Player *player = CreateAdjacentPlayer(invalidThenValid);
    const DialogueSessionId session = ReachPrompt(invalidThenValid, *player);
    const NpcTalkEvent promptEvent = invalidThenValid.GetNpcTalkEvents()[0];
    test.Expect(promptEvent.nodeKind == DialogueNodeKind::CHOICE &&
                    promptEvent.choices.size() == 2 &&
                    promptEvent.choices[0].id == DialogueChoiceId::DEVELOPMENT_GUIDE_GATHERING &&
                    promptEvent.choices[1].id == DialogueChoiceId::DEVELOPMENT_GUIDE_COMBAT,
                "Prompt event exposes ordered value-only choices");
    invalidThenValid.EnqueueCommand(DialogueContinueCommand{player->GetID(), session});
    invalidThenValid.EnqueueCommand(DialogueChooseCommand{
        player->GetID(), session, DialogueChoiceId::NONE});
    invalidThenValid.EnqueueCommand(DialogueChooseCommand{
        player->GetID(), session, static_cast<DialogueChoiceId>(999)});
    invalidThenValid.EnqueueCommand(DialogueChooseCommand{
        player->GetID(), session, DialogueChoiceId::DEVELOPMENT_GUIDE_GATHERING});
    invalidThenValid.EnqueueCommand(DialogueChooseCommand{
        player->GetID(), session, DialogueChoiceId::DEVELOPMENT_GUIDE_COMBAT});
    invalidThenValid.Update();
    test.ExpectEqual(invalidThenValid.GetNpcTalkEvents().size(), std::size_t{1},
                     "Invalid commands do not consume same-update progression");
    test.Expect(invalidThenValid.GetNpcTalkEvents()[0].nodeId ==
                    DialogueNodeId::DEVELOPMENT_GUIDE_GATHERING_RESPONSE,
                "Valid gathering choice advances after invalid commands and wins queue order");
    test.Expect(promptEvent.text == "What would you like to hear about?" &&
                    promptEvent.choices.size() == 2 &&
                    promptEvent.choices[0].text == "Tell me about gathering." &&
                    promptEvent.choices[1].text == "Tell me about combat.",
                "Copied prompt event remains valid after session advancement and event clearing");

    World chooseOnContinue;
    Player *continuePlayer = CreateAdjacentPlayer(chooseOnContinue);
    chooseOnContinue.EnqueueCommand(NpcInteractionCommand{
        continuePlayer->GetID(), 1, NpcInteractionType::TALK});
    chooseOnContinue.Update();
    const DialogueSessionId continueSession =
        chooseOnContinue.GetNpcTalkEvents()[0].sessionId;
    chooseOnContinue.EnqueueCommand(DialogueChooseCommand{
        continuePlayer->GetID(), continueSession,
        DialogueChoiceId::DEVELOPMENT_GUIDE_GATHERING});
    chooseOnContinue.Update();
    test.Expect(chooseOnContinue.GetNpcTalkEvents().empty() &&
                    chooseOnContinue.GetActiveDialogueSession(continuePlayer->GetID()) != nullptr &&
                    chooseOnContinue.GetActiveDialogueSession(continuePlayer->GetID())->currentNodeId ==
                        DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
                "CHOOSE on a continue node preserves the current session and node");

    World firstWins;
    Player *firstPlayer = CreateAdjacentPlayer(firstWins);
    const DialogueSessionId firstSession = ReachPrompt(firstWins, *firstPlayer);
    firstWins.EnqueueCommand(DialogueChooseCommand{firstPlayer->GetID(), firstSession,
        DialogueChoiceId::DEVELOPMENT_GUIDE_COMBAT});
    firstWins.EnqueueCommand(DialogueChooseCommand{firstPlayer->GetID(), firstSession,
        DialogueChoiceId::DEVELOPMENT_GUIDE_GATHERING});
    firstWins.EnqueueCommand(DialogueContinueCommand{firstPlayer->GetID(), firstSession});
    firstWins.Update();
    test.ExpectEqual(firstWins.GetNpcTalkEvents().size(), std::size_t{1},
                     "Choose then choose/continue advances only once");
    test.Expect(firstWins.GetNpcTalkEvents()[0].nodeId ==
                    DialogueNodeId::DEVELOPMENT_GUIDE_COMBAT_RESPONSE,
                "First valid queued choice wins");

    World closeThenChoose;
    Player *closeFirstPlayer = CreateAdjacentPlayer(closeThenChoose);
    const DialogueSessionId closeFirstSession = ReachPrompt(closeThenChoose, *closeFirstPlayer);
    closeThenChoose.EnqueueCommand(DialogueCloseCommand{
        closeFirstPlayer->GetID(), closeFirstSession});
    closeThenChoose.EnqueueCommand(DialogueChooseCommand{
        closeFirstPlayer->GetID(), closeFirstSession,
        DialogueChoiceId::DEVELOPMENT_GUIDE_GATHERING});
    closeThenChoose.Update();
    test.Expect(closeThenChoose.GetNpcTalkEvents().empty() &&
                    closeThenChoose.GetActiveDialogueSession(closeFirstPlayer->GetID()) == nullptr,
                "CLOSE then CHOOSE closes without publishing a branch");

    World chooseThenClose;
    Player *chooseFirstPlayer = CreateAdjacentPlayer(chooseThenClose);
    const DialogueSessionId chooseFirstSession = ReachPrompt(chooseThenClose, *chooseFirstPlayer);
    chooseThenClose.EnqueueCommand(DialogueChooseCommand{
        chooseFirstPlayer->GetID(), chooseFirstSession,
        DialogueChoiceId::DEVELOPMENT_GUIDE_GATHERING});
    chooseThenClose.EnqueueCommand(DialogueCloseCommand{
        chooseFirstPlayer->GetID(), chooseFirstSession});
    chooseThenClose.Update();
    test.Expect(chooseThenClose.GetNpcTalkEvents().size() == 1 &&
                    chooseThenClose.GetNpcTalkEvents()[0].nodeId ==
                        DialogueNodeId::DEVELOPMENT_GUIDE_GATHERING_RESPONSE &&
                    chooseThenClose.GetActiveDialogueSession(chooseFirstPlayer->GetID()) == nullptr,
                "CHOOSE then CLOSE retains the response event and closes the session");
    chooseThenClose.EnqueueCommand(DialogueContinueCommand{
        chooseFirstPlayer->GetID(), chooseFirstSession});
    chooseThenClose.Update();
    test.Expect(chooseThenClose.GetNpcTalkEvents().empty(),
                "Cancellation after branch response prevents terminal publication");

    World promptMovement;
    Player *movingPlayer = CreateAdjacentPlayer(promptMovement);
    const DialogueSessionId movingSession = ReachPrompt(promptMovement, *movingPlayer);
    promptMovement.EnqueueCommand(MoveCommand{movingPlayer->GetID(), Position{1, 3}});
    promptMovement.Update();
    promptMovement.EnqueueCommand(DialogueChooseCommand{
        movingPlayer->GetID(), movingSession,
        DialogueChoiceId::DEVELOPMENT_GUIDE_COMBAT});
    promptMovement.Update();
    test.Expect(promptMovement.GetActiveDialogueSession(movingPlayer->GetID()) == nullptr &&
                    promptMovement.GetNpcTalkEvents().empty(),
                "Movement cancellation at prompt prevents delayed branch selection");

    World adjacentCombat;
    Player *adjacentCombatPlayer = CreateAdjacentPlayer(adjacentCombat);
    const DialogueSessionId adjacentCombatSession =
        ReachPrompt(adjacentCombat, *adjacentCombatPlayer);
    adjacentCombat.EnqueueCommand(DialogueChooseCommand{
        adjacentCombatPlayer->GetID(), adjacentCombatSession,
        DialogueChoiceId::DEVELOPMENT_GUIDE_COMBAT});
    adjacentCombat.Update();
    test.Expect(adjacentCombat.GetCurrentTick() == 4 &&
                    adjacentCombat.GetNpcTalkEvents().size() == 1 &&
                    adjacentCombat.GetNpcTalkEvents()[0].nodeId ==
                        DialogueNodeId::DEVELOPMENT_GUIDE_COMBAT_RESPONSE &&
                    adjacentCombat.GetNpcTalkEvents()[0].text ==
                        "Combat tests your equipment, skills, positioning, and timing." &&
                    adjacentCombat.GetNpcTalkEvents()[0].choices.empty() &&
                    !adjacentCombat.GetNpcTalkEvents()[0].isTerminal,
                "Adjacent combat branch publishes exact response on tick four");
    adjacentCombat.EnqueueCommand(DialogueContinueCommand{
        adjacentCombatPlayer->GetID(), adjacentCombatSession});
    adjacentCombat.Update();
    test.Expect(adjacentCombat.GetCurrentTick() == 5 &&
                    adjacentCombat.GetNpcTalkEvents().size() == 1 &&
                    adjacentCombat.GetNpcTalkEvents()[0].nodeId ==
                        DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE &&
                    adjacentCombat.GetNpcTalkEvents()[0].choices.empty() &&
                    adjacentCombat.GetNpcTalkEvents()[0].isTerminal &&
                    adjacentCombat.GetActiveDialogueSession(
                        adjacentCombatPlayer->GetID()) == nullptr,
                "Adjacent combat branch reconverges and closes on tick five");
    adjacentCombat.EnqueueCommand(DialogueChooseCommand{
        adjacentCombatPlayer->GetID(), adjacentCombatSession,
        DialogueChoiceId::DEVELOPMENT_GUIDE_COMBAT});
    adjacentCombat.EnqueueCommand(DialogueContinueCommand{
        adjacentCombatPlayer->GetID(), adjacentCombatSession});
    adjacentCombat.Update();
    test.Expect(adjacentCombat.GetCurrentTick() == 6 &&
                    adjacentCombat.GetNpcTalkEvents().empty(),
                "Adjacent stale branch commands publish nothing on tick six");

    World stale;
    Player *stalePlayer = CreateAdjacentPlayer(stale);
    const DialogueSessionId oldSession = ReachPrompt(stale, *stalePlayer);
    stale.EnqueueCommand(NpcInteractionCommand{stalePlayer->GetID(), 1, NpcInteractionType::TALK});
    stale.Update();
    const DialogueSessionId replacement = stale.GetNpcTalkEvents()[0].sessionId;
    stale.EnqueueCommand(DialogueChooseCommand{stalePlayer->GetID(), oldSession,
        DialogueChoiceId::DEVELOPMENT_GUIDE_GATHERING});
    stale.Update();
    test.Expect(stale.GetNpcTalkEvents().empty() &&
                    stale.GetActiveDialogueSession(stalePlayer->GetID())->sessionId == replacement,
                "Stale choice preserves replacement session");

    World multiple;
    Player *actorA = CreateAdjacentPlayer(multiple);
    Player *actorB = CreateAdjacentPlayer(multiple);
    const DialogueSessionId sessionA = ReachPrompt(multiple, *actorA);
    const DialogueSessionId sessionB = ReachPrompt(multiple, *actorB);
    multiple.EnqueueCommand(DialogueChooseCommand{actorA->GetID(), sessionB,
        DialogueChoiceId::DEVELOPMENT_GUIDE_COMBAT});
    multiple.EnqueueCommand(DialogueChooseCommand{actorB->GetID(), sessionB,
        DialogueChoiceId::DEVELOPMENT_GUIDE_COMBAT});
    multiple.EnqueueCommand(DialogueChooseCommand{actorA->GetID(), sessionA,
        DialogueChoiceId::DEVELOPMENT_GUIDE_GATHERING});
    multiple.Update();
    test.Expect(sessionA != sessionB && multiple.GetNpcTalkEvents().size() == 2,
                "Multiple actors have distinct sessions and advance independently");
    test.Expect(multiple.GetNpcTalkEvents()[0].actorEntityID == actorB->GetID() &&
                    multiple.GetNpcTalkEvents()[0].nodeId ==
                        DialogueNodeId::DEVELOPMENT_GUIDE_COMBAT_RESPONSE &&
                    multiple.GetNpcTalkEvents()[1].actorEntityID == actorA->GetID() &&
                    multiple.GetNpcTalkEvents()[1].nodeId ==
                        DialogueNodeId::DEVELOPMENT_GUIDE_GATHERING_RESPONSE,
                "Multiple actor branch events follow reversed command order, not entity ID");
    multiple.EnqueueCommand(DialogueContinueCommand{actorB->GetID(), sessionB});
    multiple.EnqueueCommand(DialogueContinueCommand{actorA->GetID(), sessionA});
    multiple.Update();
    test.Expect(multiple.GetNpcTalkEvents().size() == 2 &&
                    multiple.GetNpcTalkEvents()[0].nodeId == DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE &&
                    multiple.GetNpcTalkEvents()[1].nodeId == DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE &&
                    multiple.GetActiveDialogueSession(actorA->GetID()) == nullptr &&
                    multiple.GetActiveDialogueSession(actorB->GetID()) == nullptr,
                "Independent actors reconverge on the same terminal in command order");

    return test.Finish();
}

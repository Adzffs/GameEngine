#include "TestSupport.h"

#include "../src/Dialogue/DialoguePresentationState.h"

#include <variant>

namespace
{
    NpcTalkEvent MakeEvent(
        int actorID,
        DialogueSessionId sessionID,
        DialogueNodeKind kind,
        bool terminal = false)
    {
        return NpcTalkEvent{
            actorID,
            1,
            NpcType::DEVELOPMENT_GUIDE,
            sessionID,
            DialogueId::DEVELOPMENT_GUIDE_INTRO,
            DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
            "Published node",
            kind,
            terminal,
            kind == DialogueNodeKind::CHOICE
                ? std::vector<DialogueEventChoice>{
                      {DialogueChoiceId::DEVELOPMENT_GUIDE_COMBAT,
                       "Combat"}}
                : std::vector<DialogueEventChoice>{}};
    }

    ActiveDialogueSession MakeSession(int actorID, DialogueSessionId sessionID)
    {
        return ActiveDialogueSession{
            sessionID,
            actorID,
            1,
            NpcType::DEVELOPMENT_GUIDE,
            DialogueId::DEVELOPMENT_GUIDE_INTRO,
            DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
            1};
    }
}

int main()
{
    TestContext test;
    DialoguePresentationState state;
    ActiveDialogueSession session = MakeSession(4, 42);

    state.Synchronize(4, {MakeEvent(9, 80, DialogueNodeKind::CONTINUE)}, nullptr);
    test.Expect(!state.IsOpen(), "Another actor's event is ignored");

    state.Synchronize(4, {MakeEvent(4, 42, DialogueNodeKind::CONTINUE)}, &session);
    test.Expect(state.IsOpen() && state.GetEvent()->sessionId == 42 &&
                    state.GetNpcName() == "Development guide",
                "Local event opens the presentation with NPC identity");
    test.Expect(state.CanTrade(), "Opening service node exposes Trade");

    NpcTalkEvent ordinary = MakeEvent(4, 42, DialogueNodeKind::CONTINUE);
    ordinary.nodeId = DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION;
    state.Synchronize(4, {ordinary}, &session);
    test.Expect(!state.CanTrade(), "Ordinary dialogue node hides Trade");
    NpcTalkEvent nonShop = ordinary;
    nonShop.nodeId = DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME;
    nonShop.npcType = NpcType::PASSIVE_DEVELOPMENT_MONSTER;
    state.Synchronize(4, {nonShop}, &session);
    test.Expect(!state.CanTrade(), "Non-shop NPC hides Trade");
    NpcTalkEvent stale = MakeEvent(4, 99, DialogueNodeKind::CONTINUE);
    state.Synchronize(4, {stale}, &session);
    test.Expect(!state.CanTrade(), "Stale event cannot expose Trade");

    auto continued = state.MakeContinueCommand();
    const auto *continueCommand = continued.has_value()
        ? std::get_if<DialogueContinueCommand>(&*continued)
        : nullptr;
    test.Expect(continueCommand != nullptr &&
                    continueCommand->actorEntityID == 4 &&
                    continueCommand->sessionId == 42,
                "Continue preserves the published session ID");

    state.Synchronize(4, {MakeEvent(4, 42, DialogueNodeKind::CHOICE)}, &session);
    auto chosen = state.MakeChoiceCommand(0);
    const auto *choiceCommand = chosen.has_value()
        ? std::get_if<DialogueChooseCommand>(&*chosen)
        : nullptr;
    test.Expect(choiceCommand != nullptr && choiceCommand->sessionId == 42 &&
                    choiceCommand->choiceId ==
                        DialogueChoiceId::DEVELOPMENT_GUIDE_COMBAT,
                "Choice preserves exact published session and choice IDs");

    auto closed = state.MakeCloseCommand();
    const auto *closeCommand = closed.has_value()
        ? std::get_if<DialogueCloseCommand>(&*closed)
        : nullptr;
    test.Expect(closeCommand != nullptr && closeCommand->sessionId == 42,
                "Close preserves the published session ID");

    ActiveDialogueSession newer = MakeSession(4, 43);
    state.Synchronize(4, {MakeEvent(4, 43, DialogueNodeKind::CONTINUE)}, &newer);
    continued = state.MakeContinueCommand();
    continueCommand = std::get_if<DialogueContinueCommand>(&*continued);
    test.Expect(continueCommand->sessionId == 43,
                "A newer event replaces controls from the older session");

    state.Synchronize(4, {}, nullptr);
    test.Expect(!state.IsOpen(),
                "A non-terminal panel closes when its authoritative session ends");

    state.Synchronize(4, {MakeEvent(4, 43, DialogueNodeKind::TERMINAL, true)}, nullptr);
    test.Expect(state.IsOpen() && state.GetEvent()->isTerminal,
                "Published terminal snapshot remains available for dismissal");
    state.DismissTerminal();
    test.Expect(!state.IsOpen(), "Terminal close dismisses the final snapshot");

    return test.Finish();
}

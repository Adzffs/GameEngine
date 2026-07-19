#include "TestSupport.h"
#include "../src/Dialogue/DialogueSystem.h"

#include <type_traits>

static_assert(!std::is_pointer_v<decltype(ActiveDialogueSession::actorEntityID)>);
static_assert(!std::is_pointer_v<decltype(ActiveDialogueSession::npcEntityID)>);

int main()
{
    TestContext test;
    DialogueSystem system;
    const DialogueSessionId first = system.Start(4, 1, NpcType::DEVELOPMENT_GUIDE,
        DialogueId::DEVELOPMENT_GUIDE_INTRO,
        DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME, 1);
    test.Expect(first != InvalidDialogueSessionId, "Start allocates nonzero session ID");
    test.Expect(system.GetSession(4) != nullptr, "Session lookup by actor succeeds");
    test.ExpectEqual(system.GetSessionCount(), std::size_t{1}, "One actor has one session");
    const DialogueSessionId replacement = system.Start(4, 1, NpcType::DEVELOPMENT_GUIDE,
        DialogueId::DEVELOPMENT_GUIDE_INTRO,
        DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME, 2);
    test.Expect(replacement != first, "Restart allocates fresh session ID");
    test.Expect(system.Start(4, 1, NpcType::DEVELOPMENT_GUIDE,
                    DialogueId::NONE, DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME, 2) ==
                    InvalidDialogueSessionId &&
                system.GetSession(4)->sessionId == replacement,
                "Invalid replacement cannot erase valid active session");
    test.Expect(!system.Advance(4, first, DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION, 3),
                "Stale session cannot advance replacement");
    test.Expect(!system.Close(4, first), "Stale session cannot close replacement");
    test.Expect(system.Advance(4, replacement,
        DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION, 3),
        "Matching session advances");
    test.Expect(!system.Advance(4, replacement,
        DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE, 3),
        "Session advances at most once per tick");
    const DialogueSessionId other = system.Start(5, 1, NpcType::DEVELOPMENT_GUIDE,
        DialogueId::DEVELOPMENT_GUIDE_INTRO,
        DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME, 3);
    test.Expect(other != replacement, "Different actors receive distinct IDs");
    test.Expect(system.CancelActor(4), "Actor cancellation succeeds");
    test.Expect(system.GetSession(4) == nullptr && system.GetSession(5) != nullptr,
                "Actor cancellation leaves other actor unchanged");
    system.Start(4, 1, NpcType::DEVELOPMENT_GUIDE,
        DialogueId::DEVELOPMENT_GUIDE_INTRO,
        DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME, 4);
    test.ExpectEqual(system.CancelTarget(1), std::size_t{2},
                     "Target cancellation removes all matching sessions");
    test.ExpectEqual(system.GetSessionCount(), std::size_t{0},
                     "Target cancellation leaves no stale session");
    system.Start(4, 1, NpcType::DEVELOPMENT_GUIDE,
        DialogueId::DEVELOPMENT_GUIDE_INTRO,
        DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME, 5);
    const DialogueSessionId differentTarget = system.Start(4, 99,
        NpcType::DEVELOPMENT_GUIDE, DialogueId::DEVELOPMENT_GUIDE_INTRO,
        DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME, 6);
    test.ExpectEqual(system.CancelTarget(1), std::size_t{0},
                     "Old target cannot cancel replacement targeting another NPC");
    test.Expect(system.GetSession(4) != nullptr &&
                    system.GetSession(4)->sessionId == differentTarget,
                "Different-target replacement remains active");
    return test.Finish();
}

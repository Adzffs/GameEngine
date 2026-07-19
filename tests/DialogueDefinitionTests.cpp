#include "TestSupport.h"
#include "../src/Content/ContentValidator.h"
#include "../src/Dialogue/DialogueDefinitionDatabase.h"
#include "../src/NPC/NpcDefinitionDatabase.h"

int main()
{
    TestContext test;
    const DialogueDefinition *dialogue = DialogueDefinitionDatabase::TryGet(
        DialogueId::DEVELOPMENT_GUIDE_INTRO);
    test.Expect(dialogue != nullptr, "Development dialogue resolves");
    test.Expect(DialogueDefinitionDatabase::TryGet(DialogueId::NONE) == nullptr,
                "NONE dialogue fails safely");
    test.Expect(DialogueDefinitionDatabase::TryGet(static_cast<DialogueId>(999)) == nullptr,
                "Unknown dialogue fails safely");
    if (dialogue != nullptr)
    {
        test.Expect(dialogue->startNodeId == DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
                    "Start node has stable identity");
        const auto *welcome = DialogueDefinitionDatabase::TryGetNode(
            dialogue->id, DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME);
        const auto *explanation = DialogueDefinitionDatabase::TryGetNode(
            dialogue->id, DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION);
        const auto *future = DialogueDefinitionDatabase::TryGetNode(
            dialogue->id, DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE);
        test.Expect(welcome != nullptr && explanation != nullptr && future != nullptr,
                    "All stable nodes resolve");
        test.Expect(welcome->text == "Welcome to the development world.",
                    "Welcome text is exact");
        test.Expect(explanation->text ==
            "This area is used to test gathering, combat, and NPC systems.",
            "Explanation text is exact");
        test.Expect(future->text ==
            "More adventures will be added as the world grows.",
            "Terminal text is exact");
        test.Expect(welcome->nextNodeId == DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION,
                    "Welcome points to explanation");
        test.Expect(explanation->nextNodeId == DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE,
                    "Explanation points to future");
        test.Expect(!future->nextNodeId.has_value(), "Future node is terminal");
        test.Expect(ContentValidator::ValidateDialogueDefinition(*dialogue).IsValid(),
                    "Built-in dialogue validates");

        DialogueDefinition duplicateNode = *dialogue;
        duplicateNode.nodes.push_back(duplicateNode.nodes.front());
        test.Expect(!ContentValidator::ValidateDialogueDefinition(duplicateNode).IsValid(),
                    "Duplicate node ID is rejected");
        DialogueDefinition missingStart = *dialogue;
        missingStart.startNodeId = static_cast<DialogueNodeId>(999);
        test.Expect(!ContentValidator::ValidateDialogueDefinition(missingStart).IsValid(),
                    "Missing start node is rejected");
        DialogueDefinition brokenNext = *dialogue;
        brokenNext.nodes[0].nextNodeId = static_cast<DialogueNodeId>(999);
        test.Expect(!ContentValidator::ValidateDialogueDefinition(brokenNext).IsValid(),
                    "Broken next reference is rejected");
        DialogueDefinition emptyText = *dialogue;
        emptyText.nodes[1].text.clear();
        test.Expect(!ContentValidator::ValidateDialogueDefinition(emptyText).IsValid(),
                    "Empty node text is rejected");
        DialogueDefinition cycle = *dialogue;
        cycle.nodes[2].nextNodeId = DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME;
        test.Expect(!ContentValidator::ValidateDialogueDefinition(cycle).IsValid(),
                    "Cycle is rejected");
        DialogueDefinition unreachable = *dialogue;
        unreachable.nodes[0].nextNodeId = DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE;
        test.Expect(!ContentValidator::ValidateDialogueDefinition(unreachable).IsValid(),
                    "Unreachable node is rejected");
        DialogueDefinition noTerminal = *dialogue;
        noTerminal.nodes[2].nextNodeId = DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION;
        test.Expect(!ContentValidator::ValidateDialogueDefinition(noTerminal).IsValid(),
                    "Graph without terminal is rejected");
        test.Expect(!ContentValidator::ValidateDialogueDefinitions({*dialogue, *dialogue}).IsValid(),
                    "Duplicate dialogue IDs are rejected");
    }

    const NpcDefinition *guide = NpcDefinitionDatabase::TryGet(NpcType::DEVELOPMENT_GUIDE);
    const NpcDefinition *passive = NpcDefinitionDatabase::TryGet(
        NpcType::PASSIVE_DEVELOPMENT_MONSTER);
    test.Expect(guide->dialogueId == DialogueId::DEVELOPMENT_GUIDE_INTRO,
                "Guide references development dialogue");
    test.Expect(passive->dialogueId == DialogueId::NONE,
                "Monster has no dialogue reference");
    NpcDefinition dialogueWithoutTalk = *guide;
    dialogueWithoutTalk.interactions.clear();
    test.Expect(!ContentValidator::ValidateNpcDefinition(dialogueWithoutTalk).IsValid(),
                "Dialogue without TALK is rejected");
    NpcDefinition unknownDialogue = *guide;
    unknownDialogue.dialogueId = static_cast<DialogueId>(999);
    test.Expect(!ContentValidator::ValidateNpcDefinition(unknownDialogue).IsValid(),
                "Unknown dialogue reference is rejected");
    return test.Finish();
}

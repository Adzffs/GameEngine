#include "DialogueDefinitionDatabase.h"

namespace
{
    const DialogueDefinition DevelopmentGuideIntro{
        DialogueId::DEVELOPMENT_GUIDE_INTRO,
        DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
        {
            {DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
             "Welcome to the development world.",
             DialogueNodeKind::CONTINUE,
             DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION,
             {}, true},
            {DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION,
             "This area is used to test gathering, combat, and NPC systems.",
             DialogueNodeKind::CONTINUE,
             DialogueNodeId::DEVELOPMENT_GUIDE_TOPIC_PROMPT,
             {}},
            {DialogueNodeId::DEVELOPMENT_GUIDE_TOPIC_PROMPT,
             "What would you like to hear about?",
             DialogueNodeKind::CHOICE,
             std::nullopt,
             {{DialogueChoiceId::DEVELOPMENT_GUIDE_GATHERING,
               "Tell me about gathering.",
               DialogueNodeId::DEVELOPMENT_GUIDE_GATHERING_RESPONSE},
              {DialogueChoiceId::DEVELOPMENT_GUIDE_COMBAT,
               "Tell me about combat.",
               DialogueNodeId::DEVELOPMENT_GUIDE_COMBAT_RESPONSE}}},
            {DialogueNodeId::DEVELOPMENT_GUIDE_GATHERING_RESPONSE,
             "Gathering lets you collect resources and turn them into useful items.",
             DialogueNodeKind::CONTINUE,
             DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE,
             {}},
            {DialogueNodeId::DEVELOPMENT_GUIDE_COMBAT_RESPONSE,
             "Combat tests your equipment, skills, positioning, and timing.",
             DialogueNodeKind::CONTINUE,
             DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE,
             {}},
            {DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE,
             "More adventures will be added as the world grows.",
             DialogueNodeKind::TERMINAL,
             std::nullopt,
             {}}
        }};
}

namespace DialogueDefinitionDatabase
{
    const std::vector<DialogueId> &GetAllDialogueIds()
    {
        static const std::vector<DialogueId> ids{
            DialogueId::DEVELOPMENT_GUIDE_INTRO};
        return ids;
    }

    const DialogueDefinition *TryGet(DialogueId id)
    {
        return id == DialogueId::DEVELOPMENT_GUIDE_INTRO
                   ? &DevelopmentGuideIntro
                   : nullptr;
    }

    const DialogueNodeDefinition *TryGetNode(DialogueId dialogueId,
                                              DialogueNodeId nodeId)
    {
        const DialogueDefinition *definition = TryGet(dialogueId);
        if (definition == nullptr || !IsValidDialogueNodeId(nodeId))
            return nullptr;
        for (const DialogueNodeDefinition &node : definition->nodes)
            if (node.id == nodeId)
                return &node;
        return nullptr;
    }

    const DialogueChoiceDefinition *TryGetChoice(
        DialogueId dialogueId, DialogueNodeId nodeId, DialogueChoiceId choiceId)
    {
        const DialogueNodeDefinition *node = TryGetNode(dialogueId, nodeId);
        if (node == nullptr || node->kind != DialogueNodeKind::CHOICE ||
            !IsValidDialogueChoiceId(choiceId))
            return nullptr;
        for (const DialogueChoiceDefinition &choice : node->choices)
            if (choice.id == choiceId)
                return &choice;
        return nullptr;
    }
}

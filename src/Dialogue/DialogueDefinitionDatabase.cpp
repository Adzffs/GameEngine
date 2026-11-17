#include "DialogueDefinitionDatabase.h"

namespace
{
    const DialogueDefinition DevelopmentGuideIntro{
        DialogueId::DEVELOPMENT_GUIDE_INTRO,
        DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
        {
            {DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
             "Welcome to Embervale. I'm Mara, the village warden.",
             DialogueNodeKind::CONTINUE,
             DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION,
             {}, true},
            {DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION,
             "The wilds provide what we need, but lately they have been pushing back.",
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
             "Start with the nearby trees and rocks. Bring your finds to the workshop, and waste nothing.",
             DialogueNodeKind::CONTINUE,
             DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE,
             {}},
            {DialogueNodeId::DEVELOPMENT_GUIDE_COMBAT_RESPONSE,
             "Choose your stance before a fight, carry food, and never mistake an Ashfang for a Mossling.",
             DialogueNodeKind::CONTINUE,
             DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE,
             {}},
            {DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE,
             "Return when you're ready. Embervale has work for capable hands.",
             DialogueNodeKind::TERMINAL,
             std::nullopt,
             {}}
        }};

    const DialogueDefinition TorrenAshIntro{
        DialogueId::TORREN_ASH_INTRO,
        DialogueNodeId::TORREN_ASH_WELCOME,
        {{DialogueNodeId::TORREN_ASH_WELCOME,
          "Mara sent you? Good. Bring me honest metal and I'll make it earn its weight.",
          DialogueNodeKind::TERMINAL,
          std::nullopt,
          {}}}};
}

namespace DialogueDefinitionDatabase
{
    const std::vector<DialogueId> &GetAllDialogueIds()
    {
        static const std::vector<DialogueId> ids{
            DialogueId::DEVELOPMENT_GUIDE_INTRO,
            DialogueId::TORREN_ASH_INTRO};
        return ids;
    }

    const DialogueDefinition *TryGet(DialogueId id)
    {
        switch (id)
        {
        case DialogueId::DEVELOPMENT_GUIDE_INTRO: return &DevelopmentGuideIntro;
        case DialogueId::TORREN_ASH_INTRO: return &TorrenAshIntro;
        case DialogueId::NONE: default: return nullptr;
        }
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

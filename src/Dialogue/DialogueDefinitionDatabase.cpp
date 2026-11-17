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
          DialogueNodeKind::CONTINUE,
          DialogueNodeId::TORREN_ASH_FAREWELL,
          {}},
         {DialogueNodeId::TORREN_ASH_FAREWELL,
          "A wooden shield is simple work. Deciding who needs it most is not.",
          DialogueNodeKind::TERMINAL,
          std::nullopt,
          {}}}};

    const DialogueDefinition EliaReedIntro{
        DialogueId::ELIA_REED_INTRO, DialogueNodeId::ELIA_REED_WELCOME,
        {{DialogueNodeId::ELIA_REED_WELCOME,
          "Your bank space is safe. The village ledger, regrettably, is not.",
          DialogueNodeKind::TERMINAL, std::nullopt, {}}}};
    const DialogueDefinition NessaThorneIntro{
        DialogueId::NESSA_THORNE_INTRO, DialogueNodeId::NESSA_THORNE_WELCOME,
        {{DialogueNodeId::NESSA_THORNE_WELCOME,
          "Take what the woodland can spare, and notice what it refuses to give.",
          DialogueNodeKind::CONTINUE,
          DialogueNodeId::NESSA_THORNE_FAREWELL, {}},
         {DialogueNodeId::NESSA_THORNE_FAREWELL,
          "A shield at the woodland path could bring a scout home alive.",
          DialogueNodeKind::TERMINAL, std::nullopt, {}}}};
    const DialogueDefinition BrannDeepwellIntro{
        DialogueId::BRANN_DEEPWELL_INTRO, DialogueNodeId::BRANN_DEEPWELL_WELCOME,
        {{DialogueNodeId::BRANN_DEEPWELL_WELCOME,
          "Old route's perfectly safe. Mostly safe. Safer if you walk ahead.",
          DialogueNodeKind::TERMINAL, std::nullopt, {}}}};
    const DialogueDefinition SeraFlintIntro{
        DialogueId::SERA_FLINT_INTRO, DialogueNodeId::SERA_FLINT_WELCOME,
        {{DialogueNodeId::SERA_FLINT_WELCOME,
          "Ashfang tracks on the village road. Fresh ones. Prepare before you follow.",
          DialogueNodeKind::TERMINAL, std::nullopt, {}}}};
}

namespace DialogueDefinitionDatabase
{
    const std::vector<DialogueId> &GetAllDialogueIds()
    {
        static const std::vector<DialogueId> ids{
            DialogueId::DEVELOPMENT_GUIDE_INTRO,
            DialogueId::TORREN_ASH_INTRO,
            DialogueId::ELIA_REED_INTRO,
            DialogueId::NESSA_THORNE_INTRO,
            DialogueId::BRANN_DEEPWELL_INTRO,
            DialogueId::SERA_FLINT_INTRO};
        return ids;
    }

    const DialogueDefinition *TryGet(DialogueId id)
    {
        switch (id)
        {
        case DialogueId::DEVELOPMENT_GUIDE_INTRO: return &DevelopmentGuideIntro;
        case DialogueId::TORREN_ASH_INTRO: return &TorrenAshIntro;
        case DialogueId::ELIA_REED_INTRO: return &EliaReedIntro;
        case DialogueId::NESSA_THORNE_INTRO: return &NessaThorneIntro;
        case DialogueId::BRANN_DEEPWELL_INTRO: return &BrannDeepwellIntro;
        case DialogueId::SERA_FLINT_INTRO: return &SeraFlintIntro;
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

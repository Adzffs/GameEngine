#include "DialogueDefinitionDatabase.h"

namespace
{
    const DialogueDefinition DevelopmentGuideIntro{
        DialogueId::DEVELOPMENT_GUIDE_INTRO,
        DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
        {
            {DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
             "Welcome to Embervale. I'm Mara Vale, the village warden. Accept a task before gathering, crafting or fighting for its progress.",
             DialogueNodeKind::CONTINUE,
             DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION,
             {}, true},
            {DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION,
             "Prepare a sword, shield and food. Start with a Mossling; ask me about Tracks in the Ash when you want to follow Sera's report.",
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
             "Equip an axe or pickaxe. The crafting table makes planks and shields; the furnace smelts bars and forges swords. Bank spare materials and keep your tools.",
             DialogueNodeKind::CONTINUE,
             DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE,
             {}},
            {DialogueNodeId::DEVELOPMENT_GUIDE_COMBAT_RESPONSE,
             "Mosslings are passive. Ashfangs attack on sight, and the Cinder Warden is tougher. Equip your gear and carry food before seeking a fight.",
             DialogueNodeKind::CONTINUE,
             DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE,
             {}},
            {DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE,
             "Tracks in the Ash follows Sera's report. After it, Torren offers Shield or Shelter; Sera's Ashfang task comes next.",
             DialogueNodeKind::TERMINAL,
             std::nullopt,
             {}}
        }};

    const DialogueDefinition TorrenAshIntro{
        DialogueId::TORREN_ASH_INTRO,
        DialogueNodeId::TORREN_ASH_WELCOME,
        {{DialogueNodeId::TORREN_ASH_WELCOME,
          "Mara sent you? Good. Finish Tracks in the Ash, then accept Shield or Shelter here before making its shield. Let your work speak for you.",
          DialogueNodeKind::CONTINUE,
          DialogueNodeId::TORREN_ASH_FAREWELL,
          {}},
         {DialogueNodeId::TORREN_ASH_FAREWELL,
          "Choose my coin reward or Nessa's food reward for the task. Either completes it, and you keep your shield. The furnace forges your sword.",
          DialogueNodeKind::TERMINAL,
          std::nullopt,
          {}}}};

    const DialogueDefinition EliaReedIntro{
        DialogueId::ELIA_REED_INTRO, DialogueNodeId::ELIA_REED_WELCOME,
        {{DialogueNodeId::ELIA_REED_WELCOME,
          "Your bank space is safe. Open the bank marker to store spare items or withdraw supplies. Keep your tools with you; crafting uses materials from your bag.",
          DialogueNodeKind::TERMINAL, std::nullopt, {}}}};
    const DialogueDefinition NessaThorneIntro{
        DialogueId::NESSA_THORNE_INTRO, DialogueNodeId::NESSA_THORNE_WELCOME,
        {{DialogueNodeId::NESSA_THORNE_WELCOME,
          "Take what the woodland can spare. Equip an axe; only normal Logs count for Gathering Basics. Oak and Willow will have their uses as you improve.",
          DialogueNodeKind::CONTINUE,
          DialogueNodeId::NESSA_THORNE_FAREWELL, {}},
         {DialogueNodeId::NESSA_THORNE_FAREWELL,
          "After crafting a shield for Shield or Shelter, choose my food reward or Torren's coins. Either choice completes the task; you keep the shield.",
          DialogueNodeKind::TERMINAL, std::nullopt, {}}}};
    const DialogueDefinition BrannDeepwellIntro{
        DialogueId::BRANN_DEEPWELL_INTRO, DialogueNodeId::BRANN_DEEPWELL_WELCOME,
        {{DialogueNodeId::BRANN_DEEPWELL_WELCOME,
          "The quarry's perfectly safe. Mostly safe. Equip a pickaxe for Copper and Tin, and keep clear of Stonebacks until you have better gear.",
          DialogueNodeKind::CONTINUE,
          DialogueNodeId::BRANN_DEEPWELL_FAREWELL, {}},
         {DialogueNodeId::BRANN_DEEPWELL_FAREWELL,
          "After defeating the Cinder Warden for Heart of the Ash, report to Mara for coins or to me for Steel bars. Only one of us can reward that victory.",
          DialogueNodeKind::TERMINAL, std::nullopt, {}}}};
    const DialogueDefinition SeraFlintIntro{
        DialogueId::SERA_FLINT_INTRO, DialogueNodeId::SERA_FLINT_WELCOME,
        {{DialogueNodeId::SERA_FLINT_WELCOME,
          "Ashfang tracks. Fresh ones. Accept Mara's Tracks in the Ash before speaking to me for the report, then return to her.",
          DialogueNodeKind::CONTINUE,
          DialogueNodeId::SERA_FLINT_FAREWELL, {}},
         {DialogueNodeId::SERA_FLINT_FAREWELL,
          "After Shield or Shelter, accept Fangs at the Gate here. Defeat Ashfangs until your journal records three, then return to me for food.",
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

#include "DialogueDefinitionDatabase.h"

namespace
{
    const DialogueDefinition DevelopmentGuideIntro{
        DialogueId::DEVELOPMENT_GUIDE_INTRO,
        DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
        {
            {DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
             "Welcome to the development world.",
             DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION},
            {DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION,
             "This area is used to test gathering, combat, and NPC systems.",
             DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE},
            {DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE,
             "More adventures will be added as the world grows.",
             std::nullopt}
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
}

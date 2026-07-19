#pragma once

#include "DialogueDefinition.h"

#include <vector>

namespace DialogueDefinitionDatabase
{
    const std::vector<DialogueId> &GetAllDialogueIds();
    const DialogueDefinition *TryGet(DialogueId id);
    const DialogueNodeDefinition *TryGetNode(DialogueId dialogueId,
                                              DialogueNodeId nodeId);
    const DialogueChoiceDefinition *TryGetChoice(
        DialogueId dialogueId, DialogueNodeId nodeId, DialogueChoiceId choiceId);
}

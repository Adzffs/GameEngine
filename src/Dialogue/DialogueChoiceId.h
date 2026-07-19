#pragma once

enum class DialogueChoiceId
{
    NONE = 0,
    DEVELOPMENT_GUIDE_GATHERING = 1,
    DEVELOPMENT_GUIDE_COMBAT = 2
};

constexpr bool IsValidDialogueChoiceId(DialogueChoiceId id)
{
    return id == DialogueChoiceId::DEVELOPMENT_GUIDE_GATHERING ||
           id == DialogueChoiceId::DEVELOPMENT_GUIDE_COMBAT;
}

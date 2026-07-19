#pragma once

enum class DialogueId
{
    NONE,
    DEVELOPMENT_GUIDE_INTRO
};

constexpr bool IsValidDialogueId(DialogueId id)
{
    return id == DialogueId::DEVELOPMENT_GUIDE_INTRO;
}

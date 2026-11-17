#pragma once

enum class DialogueId
{
    NONE,
    DEVELOPMENT_GUIDE_INTRO,
    TORREN_ASH_INTRO
};

constexpr bool IsValidDialogueId(DialogueId id)
{
    return id == DialogueId::DEVELOPMENT_GUIDE_INTRO ||
           id == DialogueId::TORREN_ASH_INTRO;
}

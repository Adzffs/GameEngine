#pragma once

enum class DialogueId
{
    NONE,
    DEVELOPMENT_GUIDE_INTRO,
    TORREN_ASH_INTRO,
    ELIA_REED_INTRO,
    NESSA_THORNE_INTRO,
    BRANN_DEEPWELL_INTRO,
    SERA_FLINT_INTRO
};

constexpr bool IsValidDialogueId(DialogueId id)
{
    return id == DialogueId::DEVELOPMENT_GUIDE_INTRO ||
           id == DialogueId::TORREN_ASH_INTRO ||
           id == DialogueId::ELIA_REED_INTRO ||
           id == DialogueId::NESSA_THORNE_INTRO ||
           id == DialogueId::BRANN_DEEPWELL_INTRO ||
           id == DialogueId::SERA_FLINT_INTRO;
}

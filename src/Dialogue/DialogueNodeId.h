#pragma once

enum class DialogueNodeId
{
    NONE = 0,
    DEVELOPMENT_GUIDE_WELCOME = 1,
    DEVELOPMENT_GUIDE_EXPLANATION = 2,
    DEVELOPMENT_GUIDE_FUTURE = 3,
    DEVELOPMENT_GUIDE_TOPIC_PROMPT = 4,
    DEVELOPMENT_GUIDE_GATHERING_RESPONSE = 5,
    DEVELOPMENT_GUIDE_COMBAT_RESPONSE = 6,
    TORREN_ASH_WELCOME = 7,
    ELIA_REED_WELCOME = 8,
    NESSA_THORNE_WELCOME = 9,
    BRANN_DEEPWELL_WELCOME = 10,
    SERA_FLINT_WELCOME = 11,
    TORREN_ASH_FAREWELL = 12,
    NESSA_THORNE_FAREWELL = 13,
    SERA_FLINT_FAREWELL = 14,
    BRANN_DEEPWELL_FAREWELL = 15
};

constexpr bool IsValidDialogueNodeId(DialogueNodeId id)
{
    return id == DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME ||
           id == DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION ||
           id == DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE ||
           id == DialogueNodeId::DEVELOPMENT_GUIDE_TOPIC_PROMPT ||
           id == DialogueNodeId::DEVELOPMENT_GUIDE_GATHERING_RESPONSE ||
           id == DialogueNodeId::DEVELOPMENT_GUIDE_COMBAT_RESPONSE ||
           id == DialogueNodeId::TORREN_ASH_WELCOME ||
           id == DialogueNodeId::ELIA_REED_WELCOME ||
           id == DialogueNodeId::NESSA_THORNE_WELCOME ||
           id == DialogueNodeId::BRANN_DEEPWELL_WELCOME ||
           id == DialogueNodeId::SERA_FLINT_WELCOME ||
           id == DialogueNodeId::TORREN_ASH_FAREWELL ||
           id == DialogueNodeId::NESSA_THORNE_FAREWELL ||
           id == DialogueNodeId::SERA_FLINT_FAREWELL ||
           id == DialogueNodeId::BRANN_DEEPWELL_FAREWELL;
}

#pragma once

enum class DialogueNodeId
{
    NONE = 0,
    DEVELOPMENT_GUIDE_WELCOME = 1,
    DEVELOPMENT_GUIDE_EXPLANATION = 2,
    DEVELOPMENT_GUIDE_FUTURE = 3,
    DEVELOPMENT_GUIDE_TOPIC_PROMPT = 4,
    DEVELOPMENT_GUIDE_GATHERING_RESPONSE = 5,
    DEVELOPMENT_GUIDE_COMBAT_RESPONSE = 6
};

constexpr bool IsValidDialogueNodeId(DialogueNodeId id)
{
    return id == DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME ||
           id == DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION ||
           id == DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE ||
           id == DialogueNodeId::DEVELOPMENT_GUIDE_TOPIC_PROMPT ||
           id == DialogueNodeId::DEVELOPMENT_GUIDE_GATHERING_RESPONSE ||
           id == DialogueNodeId::DEVELOPMENT_GUIDE_COMBAT_RESPONSE;
}

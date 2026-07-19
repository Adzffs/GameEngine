#pragma once

enum class DialogueNodeId
{
    NONE,
    DEVELOPMENT_GUIDE_WELCOME,
    DEVELOPMENT_GUIDE_EXPLANATION,
    DEVELOPMENT_GUIDE_FUTURE
};

constexpr bool IsValidDialogueNodeId(DialogueNodeId id)
{
    return id == DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME ||
           id == DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION ||
           id == DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE;
}

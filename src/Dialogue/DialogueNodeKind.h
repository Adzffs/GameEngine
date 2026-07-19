#pragma once

enum class DialogueNodeKind
{
    CONTINUE,
    CHOICE,
    TERMINAL
};

constexpr bool IsValidDialogueNodeKind(DialogueNodeKind kind)
{
    return kind == DialogueNodeKind::CONTINUE ||
           kind == DialogueNodeKind::CHOICE ||
           kind == DialogueNodeKind::TERMINAL;
}

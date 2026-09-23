#pragma once
#include "QuestId.h"

enum class QuestDialogueActionKind : int
{
    NONE = 0,
    ACCEPT = 1,
    COMPLETE = 2
};

struct QuestDialogueAction
{
    QuestDialogueActionKind kind = QuestDialogueActionKind::NONE;
    QuestId questId = QuestId::NONE;
};

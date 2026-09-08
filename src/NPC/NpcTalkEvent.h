#pragma once

#include "NpcType.h"
#include "../Dialogue/DialogueId.h"
#include "../Dialogue/DialogueEventChoice.h"
#include "../Dialogue/DialogueNodeId.h"
#include "../Dialogue/DialogueNodeKind.h"
#include "../Dialogue/DialogueSessionId.h"
#include "../Quest/QuestDialogueAction.h"
#include <string>
#include <vector>

struct NpcTalkEvent
{
    int actorEntityID;
    int npcEntityID;
    NpcType npcType;
    DialogueSessionId sessionId;
    DialogueId dialogueId;
    DialogueNodeId nodeId;
    std::string text;
    DialogueNodeKind nodeKind;
    bool isTerminal;
    std::vector<DialogueEventChoice> choices;
    bool offersTrade = false;
    QuestDialogueAction questAction = QuestDialogueAction::NONE;
};

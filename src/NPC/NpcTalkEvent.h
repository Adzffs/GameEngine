#pragma once

#include "NpcType.h"
#include "../Dialogue/DialogueId.h"
#include "../Dialogue/DialogueNodeId.h"
#include "../Dialogue/DialogueSessionId.h"
#include <string>

struct NpcTalkEvent
{
    int actorEntityID;
    int npcEntityID;
    NpcType npcType;
    DialogueSessionId sessionId;
    DialogueId dialogueId;
    DialogueNodeId nodeId;
    std::string text;
    bool isTerminal;
};

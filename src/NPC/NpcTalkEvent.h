#pragma once

#include "NpcType.h"
#include <string>

struct NpcTalkEvent
{
    int actorEntityID;
    int npcEntityID;
    NpcType npcType;
    std::string text;
};

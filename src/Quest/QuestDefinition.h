#pragma once
#include "QuestId.h"
#include "QuestState.h"
#include "QuestObjectiveType.h"
#include "../Dialogue/DialogueId.h"
#include "../Dialogue/DialogueNodeId.h"
#include "../Inventory/ItemType.h"
#include "../NPC/NpcType.h"

struct QuestGiverDefinition
{
    NpcType npcType;
    DialogueId dialogueId;
    DialogueNodeId interactionNodeId;
};

struct QuestDefinition
{
    QuestId id;
    const char *name;
    QuestState initialState;
    ItemType objectiveItem;
    int requiredAmount;
    ItemType rewardItem;
    int rewardAmount;
    QuestGiverDefinition giver;
    QuestObjectiveType objectiveType = QuestObjectiveType::GATHER_ITEM;
    NpcType objectiveNpc = NpcType::NONE;
    const char *objectiveText = nullptr;
};

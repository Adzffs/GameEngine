#pragma once
#include "QuestId.h"
#include "QuestState.h"
#include "QuestObjectiveType.h"
#include "QuestCategory.h"
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

struct QuestCompletionDefinition
{
    NpcType npcType = NpcType::NONE;
    DialogueId dialogueId = DialogueId::NONE;
    DialogueNodeId interactionNodeId = DialogueNodeId::NONE;
    ItemType rewardItem = ItemType::NONE;
    int rewardAmount = 0;
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
    QuestCompletionDefinition alternateCompletion{};
    QuestCategory category = QuestCategory::SIDE;
};

#pragma once
#include "QuestId.h"
#include "../Inventory/ItemType.h"
struct QuestDefinition{QuestId id; const char* name; ItemType objectiveItem; int requiredAmount; ItemType rewardItem; int rewardAmount;};

#pragma once
#include "QuestId.h"
#include "QuestState.h"
struct QuestUpdateEvent{int actorEntityID; QuestId questId; QuestState state; int progress;};

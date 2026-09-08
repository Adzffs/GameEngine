#pragma once
#include "QuestId.h"
#include "QuestState.h"
struct QuestRecord{QuestId id=QuestId::GATHERING_BASICS; QuestState state=QuestState::AVAILABLE; int progress=0;};
class QuestJournal{public: const QuestRecord& Get()const{return record;} QuestRecord& Get(){return record;} private: QuestRecord record;};

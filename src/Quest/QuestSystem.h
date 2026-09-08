#pragma once
#include "QuestJournal.h"
#include "QuestDefinition.h"
class Inventory;
class QuestSystem{public: static bool Accept(QuestJournal&); static bool RecordGathered(QuestJournal&,ItemType); static bool Complete(QuestJournal&,Inventory&);};

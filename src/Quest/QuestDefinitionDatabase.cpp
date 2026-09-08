#include "QuestDefinitionDatabase.h"
namespace { constexpr QuestDefinition GatheringBasics{QuestId::GATHERING_BASICS,"Gathering Basics",ItemType::LOG,10,ItemType::COINS,10}; }
const QuestDefinition* QuestDefinitionDatabase::TryGet(QuestId id){return id==QuestId::GATHERING_BASICS?&GatheringBasics:nullptr;}

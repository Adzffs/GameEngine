#include "QuestSystem.h"
#include "../Inventory/Inventory.h"
#include "QuestDefinitionDatabase.h"
bool QuestSystem::Accept(QuestJournal&j){auto&r=j.Get();if(r.state!=QuestState::AVAILABLE)return false;r.state=QuestState::ACTIVE;return true;}
bool QuestSystem::RecordGathered(QuestJournal&j,ItemType item){auto&r=j.Get();auto*d=QuestDefinitionDatabase::TryGet(r.id);if(!d||r.state!=QuestState::ACTIVE||item!=d->objectiveItem)return false;if(++r.progress>=d->requiredAmount){r.progress=d->requiredAmount;r.state=QuestState::READY_TO_COMPLETE;}return true;}
bool QuestSystem::Complete(QuestJournal&j,Inventory&i){auto&r=j.Get();auto*d=QuestDefinitionDatabase::TryGet(r.id);if(!d||r.state!=QuestState::READY_TO_COMPLETE||!i.AddItem(d->rewardItem,d->rewardAmount))return false;r.state=QuestState::COMPLETED;return true;}

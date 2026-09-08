#pragma once
#include "QuestDefinitionDatabase.h"
#include "QuestJournal.h"
#include <string>
struct QuestPresentation{QuestId id;QuestState state;int progress;int required;std::string title;};
class QuestPresentationState{public:void Synchronize(const QuestJournal&j){const auto&r=j.Get();const auto*d=QuestDefinitionDatabase::TryGet(r.id);view={r.id,r.state,r.progress,d?d->requiredAmount:0,d?d->name:"Unknown quest"};}const QuestPresentation&Get()const{return view;}private:QuestPresentation view{QuestId::GATHERING_BASICS,QuestState::AVAILABLE,0,0,""};};

#pragma once
enum class QuestId : int { NONE=0, GATHERING_BASICS=1 };
constexpr bool IsValidQuestId(QuestId id){return id==QuestId::GATHERING_BASICS;}

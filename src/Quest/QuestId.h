#pragma once
enum class QuestId : int { NONE=0, GATHERING_BASICS=1, MINING_BASICS=2 };
constexpr bool IsValidQuestId(QuestId id)
{
    return id == QuestId::GATHERING_BASICS || id == QuestId::MINING_BASICS;
}

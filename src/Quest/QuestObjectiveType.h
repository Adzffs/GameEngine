#pragma once

enum class QuestObjectiveType
{
    GATHER_ITEM,
    TALK_TO_CHARACTER
};

constexpr bool IsValidQuestObjectiveType(QuestObjectiveType type)
{
    return type == QuestObjectiveType::GATHER_ITEM ||
           type == QuestObjectiveType::TALK_TO_CHARACTER;
}

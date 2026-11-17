#pragma once

enum class QuestObjectiveType
{
    GATHER_ITEM,
    TALK_TO_CHARACTER,
    CRAFT_ITEM
};

constexpr bool IsValidQuestObjectiveType(QuestObjectiveType type)
{
    return type == QuestObjectiveType::GATHER_ITEM ||
           type == QuestObjectiveType::TALK_TO_CHARACTER ||
           type == QuestObjectiveType::CRAFT_ITEM;
}

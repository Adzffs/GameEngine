#pragma once

enum class QuestCategory
{
    TUTORIAL,
    MAIN,
    SIDE
};

constexpr bool IsValidQuestCategory(QuestCategory category)
{
    return category == QuestCategory::TUTORIAL ||
        category == QuestCategory::MAIN ||
        category == QuestCategory::SIDE;
}

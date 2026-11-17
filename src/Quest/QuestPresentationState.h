#pragma once

#include "QuestDefinitionDatabase.h"
#include "QuestJournal.h"
#include "../Item/ItemDatabase.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

struct QuestPresentation
{
    QuestId id = QuestId::NONE;
    QuestState state = QuestState::UNAVAILABLE;
    int progress = 0;
    int required = 0;
    std::string title;
    std::string objective;
    QuestCategory category = QuestCategory::SIDE;
    std::string reward;
    int rewardAmount = 0;
};

class QuestPresentationState
{
public:
    void Synchronize(const QuestJournal &journal)
    {
        std::vector<QuestPresentation> replacement;
        replacement.reserve(journal.GetRecords().size());
        for (const auto &[questId, record] : journal.GetRecords())
        {
            const QuestDefinition *definition =
                QuestDefinitionDatabase::TryGet(questId);
            if (definition == nullptr) continue;
            std::string objective;
            if (definition->objectiveType == QuestObjectiveType::TALK_TO_CHARACTER ||
                definition->objectiveType == QuestObjectiveType::DEFEAT_CHARACTER)
                objective = definition->objectiveText == nullptr
                    ? "Speak with the character" : definition->objectiveText;
            else
            {
                objective = ItemDatabase::Get(definition->objectiveItem).GetName();
                if (definition->requiredAmount != 1) objective += "s";
            }
            replacement.push_back({questId, record.state, record.progress,
                definition->requiredAmount, definition->name,
                std::move(objective), definition->category,
                ItemDatabase::Get(definition->rewardItem).GetName(),
                definition->rewardAmount});
        }
        std::stable_sort(replacement.begin(), replacement.end(),
            [](const QuestPresentation &left, const QuestPresentation &right)
            {
                const auto priority = [](QuestState state)
                {
                    switch (state)
                    {
                    case QuestState::READY_TO_COMPLETE: return 0;
                    case QuestState::ACTIVE: return 1;
                    case QuestState::AVAILABLE: return 2;
                    case QuestState::COMPLETED: return 3;
                    case QuestState::UNAVAILABLE: return 4;
                    }
                    return 4;
                };
                return priority(left.state) < priority(right.state);
            });
        quests = std::move(replacement);
    }

    const std::vector<QuestPresentation> &GetQuests() const
    {
        return quests;
    }

private:
    std::vector<QuestPresentation> quests;
};

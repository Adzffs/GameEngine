#pragma once

#include "QuestDefinitionDatabase.h"
#include "QuestJournal.h"
#include "../Item/ItemDatabase.h"

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
            std::string objective =
                ItemDatabase::Get(definition->objectiveItem).GetName();
            if (definition->requiredAmount != 1) objective += "s";
            replacement.push_back({questId, record.state, record.progress,
                definition->requiredAmount, definition->name,
                std::move(objective)});
        }
        quests = std::move(replacement);
    }

    const std::vector<QuestPresentation> &GetQuests() const
    {
        return quests;
    }

private:
    std::vector<QuestPresentation> quests;
};

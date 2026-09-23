#include "QuestJournal.h"

#include "QuestDefinitionDatabase.h"

QuestJournal::QuestJournal()
    : QuestJournal(QuestDefinitionDatabase::GetAll())
{
}

QuestJournal::QuestJournal(const std::vector<QuestDefinition> &definitions)
{
    for (const QuestDefinition &definition : definitions)
    {
        records.emplace(definition.id, QuestRecord{
            definition.id, definition.initialState, 0});
    }
}

const QuestRecord *QuestJournal::TryGet(QuestId questId) const
{
    const auto found = records.find(questId);
    return found == records.end() ? nullptr : &found->second;
}

const std::map<QuestId, QuestRecord> &QuestJournal::GetRecords() const
{
    return records;
}

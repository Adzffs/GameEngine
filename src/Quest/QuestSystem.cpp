#include "QuestSystem.h"
#include "../Inventory/Inventory.h"
#include "QuestDefinitionDatabase.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>

bool QuestSystem::IsValidRecord(
    const QuestRecord &record, const QuestDefinition &definition)
{
    if (record.id != definition.id || record.progress < 0 ||
        record.progress > definition.requiredAmount)
        return false;
    switch (record.state)
    {
    case QuestState::UNAVAILABLE:
    case QuestState::AVAILABLE:
        return record.progress == 0;
    case QuestState::ACTIVE:
        return record.progress < definition.requiredAmount;
    case QuestState::READY_TO_COMPLETE:
    case QuestState::COMPLETED:
        return record.progress == definition.requiredAmount;
    }
    return false;
}

bool QuestSystem::Accept(QuestJournal &journal, QuestId questId)
{
    const QuestDefinition *definition = QuestDefinitionDatabase::TryGet(questId);
    auto found = journal.records.find(questId);
    if (definition == nullptr || found == journal.records.end() ||
        found->second.state != QuestState::AVAILABLE)
        return false;
    found->second.state = QuestState::ACTIVE;
    return true;
}

std::vector<QuestId> QuestSystem::RecordGathered(
    QuestJournal &journal, ItemType itemType, int quantity)
{
    return RecordGatheredAgainstDefinitions(journal, itemType, quantity,
        QuestDefinitionDatabase::GetAll());
}

std::vector<QuestId> QuestSystem::RecordGatheredAgainstDefinitions(
    QuestJournal &journal, ItemType itemType, int quantity,
    const std::vector<QuestDefinition> &definitions)
{
    std::vector<QuestId> changed;
    if (quantity <= 0) return changed;
    for (const QuestDefinition &definition : definitions)
    {
        auto found = journal.records.find(definition.id);
        if (found == journal.records.end() ||
            found->second.state != QuestState::ACTIVE ||
            definition.objectiveItem != itemType)
            continue;
        QuestRecord &record = found->second;
        const std::int64_t total =
            static_cast<std::int64_t>(record.progress) + quantity;
        record.progress = static_cast<int>(std::min<std::int64_t>(
            total, definition.requiredAmount));
        if (record.progress == definition.requiredAmount)
            record.state = QuestState::READY_TO_COMPLETE;
        changed.push_back(definition.id);
    }
    return changed;
}

bool QuestSystem::Complete(
    QuestJournal &journal, QuestId questId, Inventory &inventory)
{
    const QuestDefinition *definition = QuestDefinitionDatabase::TryGet(questId);
    auto found = journal.records.find(questId);
    if (definition == nullptr || found == journal.records.end() ||
        found->second.state != QuestState::READY_TO_COMPLETE ||
        !inventory.AddItem(definition->rewardItem, definition->rewardAmount))
        return false;
    found->second.state = QuestState::COMPLETED;
    return true;
}

bool QuestSystem::TryRestore(
    QuestJournal &journal, const std::vector<QuestRecord> &records)
{
    const auto &definitions = QuestDefinitionDatabase::GetAll();
    if (records.size() != definitions.size()) return false;
    std::map<QuestId, QuestRecord> replacement;
    for (const QuestRecord &record : records)
    {
        if (record.id == QuestId::NONE) return false;
        const QuestDefinition *definition = QuestDefinitionDatabase::TryGet(record.id);
        if (definition == nullptr || !IsValidRecord(record, *definition) ||
            !replacement.emplace(record.id, record).second)
            return false;
    }
    for (const QuestDefinition &definition : definitions)
        if (!replacement.contains(definition.id)) return false;
    journal.records = std::move(replacement);
    return true;
}

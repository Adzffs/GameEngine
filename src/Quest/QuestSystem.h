#pragma once
#include "QuestJournal.h"
#include "QuestDefinition.h"
#include <vector>
class Inventory;
struct QuestSystemTestAccess;
class QuestSystem
{
public:
    static bool Accept(QuestJournal &, QuestId);
    static std::vector<QuestId> RecordGathered(
        QuestJournal &, ItemType, int quantity);
    static bool Complete(QuestJournal &, QuestId, Inventory &);
    static bool TryRestore(
        QuestJournal &, const std::vector<QuestRecord> &records);

private:
    friend struct QuestSystemTestAccess;
    static std::vector<QuestId> RecordGatheredAgainstDefinitions(
        QuestJournal &, ItemType, int quantity,
        const std::vector<QuestDefinition> &definitions);
    static bool IsValidRecord(
        const QuestRecord &, const QuestDefinition &);
};

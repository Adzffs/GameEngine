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
    static std::vector<QuestId> RecordTalkedTo(
        QuestJournal &, NpcType);
    static std::vector<QuestId> RecordCrafted(
        QuestJournal &, ItemType, int quantity);
    static bool Complete(QuestJournal &, QuestId, Inventory &);
    static bool TryRestore(
        QuestJournal &, const std::vector<QuestRecord> &records);

private:
    friend struct QuestSystemTestAccess;
    friend class World;
    static std::vector<QuestId> RecordGatheredAgainstDefinitions(
        QuestJournal &, ItemType, int quantity,
        const std::vector<QuestDefinition> &definitions);
    static std::vector<QuestId> RecordTalkedToAgainstDefinitions(
        QuestJournal &, NpcType,
        const std::vector<QuestDefinition> &definitions);
    static std::vector<QuestId> RecordCraftedAgainstDefinitions(
        QuestJournal &, ItemType, int quantity,
        const std::vector<QuestDefinition> &definitions);
    static bool CompleteWithReward(
        QuestJournal &, QuestId, Inventory &, ItemType, int);
    static bool IsValidRecord(
        const QuestRecord &, const QuestDefinition &);
};

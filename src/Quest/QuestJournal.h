#pragma once
#include "QuestId.h"
#include "QuestState.h"
#include "QuestDefinition.h"
#include <map>
#include <vector>

struct QuestRecord
{
    QuestId id = QuestId::NONE;
    QuestState state = QuestState::UNAVAILABLE;
    int progress = 0;
};

class QuestSystem;
struct QuestJournalTestAccess;

class QuestJournal
{
public:
    QuestJournal();
    const QuestRecord *TryGet(QuestId questId) const;
    const std::map<QuestId, QuestRecord> &GetRecords() const;

private:
    friend class QuestSystem;
    friend struct QuestJournalTestAccess;
    explicit QuestJournal(const std::vector<QuestDefinition> &definitions);
    std::map<QuestId, QuestRecord> records;
};

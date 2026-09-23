#include "TestSupport.h"
#include "../src/Inventory/Inventory.h"
#include "../src/Quest/QuestDefinitionDatabase.h"
#include "../src/Quest/QuestSystem.h"

#include <type_traits>

namespace
{
    const QuestRecord *Gathering(const QuestJournal &journal)
    {
        return journal.TryGet(QuestId::GATHERING_BASICS);
    }
    std::vector<QuestRecord> Records(
        QuestState gatheringState, int gatheringProgress,
        QuestState miningState = QuestState::AVAILABLE, int miningProgress = 0)
    {
        return {{QuestId::GATHERING_BASICS, gatheringState, gatheringProgress},
                {QuestId::MINING_BASICS, miningState, miningProgress}};
    }
}

struct QuestJournalTestAccess
{
    static QuestJournal FromDefinitions(
        const std::vector<QuestDefinition> &definitions)
    {
        return QuestJournal(definitions);
    }
    static void ActivateAll(QuestJournal &journal)
    {
        for (auto &[id, record] : journal.records)
            record.state = QuestState::ACTIVE;
    }
};

struct QuestSystemTestAccess
{
    static std::vector<QuestId> RecordGathered(
        QuestJournal &journal, ItemType itemType, int quantity,
        const std::vector<QuestDefinition> &definitions)
    {
        return QuestSystem::RecordGatheredAgainstDefinitions(
            journal, itemType, quantity, definitions);
    }
};

int main()
{
    TestContext test;
    static_assert(std::is_same_v<
        decltype(std::declval<const QuestJournal &>().GetRecords()),
        const std::map<QuestId, QuestRecord> &>);
    QuestJournal journal;
    const QuestRecord *initial = Gathering(journal);
    test.Expect(initial != nullptr && initial->state == QuestState::AVAILABLE &&
                    initial->progress == 0 && journal.GetRecords().size() == 2 &&
                    journal.TryGet(QuestId::MINING_BASICS)->state == QuestState::AVAILABLE,
                "Default journal derives canonical initial records");
    test.Expect(QuestSystem::Accept(journal, QuestId::GATHERING_BASICS) &&
                    !QuestSystem::Accept(journal, QuestId::GATHERING_BASICS) &&
                    !QuestSystem::Accept(journal, static_cast<QuestId>(999)),
                "Generic Accept succeeds exactly once and rejects unknown ID");
    test.Expect(QuestSystem::RecordGathered(journal, ItemType::OAK_LOG, 1).empty() &&
                    QuestSystem::RecordGathered(journal, ItemType::WILLOW_LOG, 1).empty() &&
                    QuestSystem::RecordGathered(journal, ItemType::LOG, 0).empty() &&
                    QuestSystem::RecordGathered(journal, ItemType::LOG, -1).empty(),
                "Other logs and non-positive quantities do not progress");
    const auto changed = QuestSystem::RecordGathered(journal, ItemType::LOG, 6);
    test.Expect(changed == std::vector<QuestId>{QuestId::GATHERING_BASICS} &&
                    Gathering(journal)->progress == 6 &&
                    Gathering(journal)->state == QuestState::ACTIVE,
                "Quantity-based progress reports deterministic changed IDs");
    QuestSystem::RecordGathered(journal, ItemType::LOG, 100);
    test.Expect(Gathering(journal)->progress == 10 &&
                    Gathering(journal)->state == QuestState::READY_TO_COMPLETE,
                "Oversized quantity caps directly and transitions once");

    Inventory inventory;
    test.Expect(QuestSystem::Complete(
                    journal, QuestId::GATHERING_BASICS, inventory) &&
                    inventory.GetItemAmount(ItemType::COINS) == 10,
                "Generic completion grants exact reward");
    test.Expect(!QuestSystem::Complete(
                    journal, QuestId::GATHERING_BASICS, inventory) &&
                    inventory.GetItemAmount(ItemType::COINS) == 10,
                "Completion cannot reward twice");

    QuestJournal restored;
    test.Expect(QuestSystem::TryRestore(restored, Records(
                    QuestState::ACTIVE, 4)) &&
                    Gathering(restored)->progress == 4,
                "Transactional restoration accepts complete valid schema");
    const QuestRecord before = *Gathering(restored);
    const std::vector<std::vector<QuestRecord>> rejected{
        {{QuestId::NONE, QuestState::AVAILABLE, 0}},
        {{static_cast<QuestId>(999), QuestState::AVAILABLE, 0}},
        {{QuestId::GATHERING_BASICS, QuestState::ACTIVE, 10},
         {QuestId::MINING_BASICS, QuestState::AVAILABLE, 0}},
        {},
        {{QuestId::GATHERING_BASICS, QuestState::ACTIVE, 4},
         {QuestId::GATHERING_BASICS, QuestState::ACTIVE, 4}}};
    for (const auto &records : rejected)
    {
        test.Expect(!QuestSystem::TryRestore(restored, records) &&
                        Gathering(restored)->state == before.state &&
                        Gathering(restored)->progress == before.progress,
                    "Each failed restoration leaves journal unchanged");
    }

    QuestJournal fullJournal;
    QuestSystem::TryRestore(fullJournal, Records(
        QuestState::READY_TO_COMPLETE, 10));
    Inventory full;
    for (int index = 0; index < Inventory::SlotCount; ++index)
        full.AddItem(ItemType::LOG, 1);
    test.Expect(!QuestSystem::Complete(fullJournal,
                    QuestId::GATHERING_BASICS, full) &&
                    Gathering(fullJournal)->state == QuestState::READY_TO_COMPLETE &&
                    full.GetItemAmount(ItemType::COINS) == 0,
                "Atomic reward failure preserves inventory and quest state");

    QuestDefinition first = *QuestDefinitionDatabase::TryGet(
        QuestId::GATHERING_BASICS);
    QuestDefinition second = first;
    first.id = static_cast<QuestId>(2);
    second.id = static_cast<QuestId>(3);
    std::vector<QuestDefinition> synthetic{first, second};
    QuestJournal multiple = QuestJournalTestAccess::FromDefinitions(synthetic);
    QuestJournalTestAccess::ActivateAll(multiple);
    const auto syntheticChanged = QuestSystemTestAccess::RecordGathered(
        multiple, ItemType::LOG, 1, synthetic);
    test.Expect(syntheticChanged == std::vector<QuestId>{
                    static_cast<QuestId>(2), static_cast<QuestId>(3)} &&
                    multiple.TryGet(static_cast<QuestId>(2))->progress == 1 &&
                    multiple.TryGet(static_cast<QuestId>(3))->progress == 1,
                "Multiple matching synthetic records progress in canonical order");
    return test.Finish();
}

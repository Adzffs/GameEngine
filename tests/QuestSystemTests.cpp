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
                {QuestId::MINING_BASICS, miningState, miningProgress},
                {QuestId::TRACKS_IN_THE_ASH, QuestState::AVAILABLE, 0},
                {QuestId::SHIELD_OR_SHELTER, QuestState::AVAILABLE, 0},
                {QuestId::FANGS_AT_THE_GATE, QuestState::AVAILABLE, 0},
                {QuestId::HEART_OF_THE_ASH, QuestState::AVAILABLE, 0}};
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
    static std::vector<QuestId> RecordTalkedTo(
        QuestJournal &journal, NpcType npcType,
        const std::vector<QuestDefinition> &definitions)
    {
        return QuestSystem::RecordTalkedToAgainstDefinitions(
            journal, npcType, definitions);
    }
    static std::vector<QuestId> RecordCrafted(
        QuestJournal &journal, ItemType itemType, int quantity,
        const std::vector<QuestDefinition> &definitions)
    {
        return QuestSystem::RecordCraftedAgainstDefinitions(
            journal, itemType, quantity, definitions);
    }
    static bool CompleteWithReward(QuestJournal &journal, QuestId questId,
        Inventory &inventory, ItemType rewardItem, int rewardAmount)
    {
        return QuestSystem::CompleteWithReward(
            journal, questId, inventory, rewardItem, rewardAmount);
    }
    static std::vector<QuestId> RecordDefeated(
        QuestJournal &journal, NpcType npcType, int quantity,
        const std::vector<QuestDefinition> &definitions)
    {
        return QuestSystem::RecordDefeatedAgainstDefinitions(
            journal, npcType, quantity, definitions);
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
                    initial->progress == 0 && journal.GetRecords().size() == 6 &&
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
         {QuestId::MINING_BASICS, QuestState::AVAILABLE, 0},
         {QuestId::TRACKS_IN_THE_ASH, QuestState::AVAILABLE, 0},
         {QuestId::SHIELD_OR_SHELTER, QuestState::AVAILABLE, 0},
         {QuestId::FANGS_AT_THE_GATE, QuestState::AVAILABLE, 0}},
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

    QuestDefinition investigation = first;
    investigation.objectiveType = QuestObjectiveType::TALK_TO_CHARACTER;
    investigation.objectiveItem = ItemType::NONE;
    investigation.objectiveNpc = NpcType::SERA_FLINT;
    investigation.objectiveText = "Ask Sera about the tracks";
    investigation.requiredAmount = 1;
    std::vector<QuestDefinition> investigationDefinitions{investigation};
    QuestJournal investigationJournal =
        QuestJournalTestAccess::FromDefinitions(investigationDefinitions);
    QuestJournalTestAccess::ActivateAll(investigationJournal);
    test.Expect(
        QuestSystemTestAccess::RecordTalkedTo(investigationJournal,
            NpcType::BRANN_DEEPWELL, investigationDefinitions).empty() &&
        investigationJournal.TryGet(investigation.id)->progress == 0,
        "Talking to the wrong character does not advance an investigation");
    const auto investigated = QuestSystemTestAccess::RecordTalkedTo(
        investigationJournal, NpcType::SERA_FLINT, investigationDefinitions);
    test.Expect(investigated == std::vector<QuestId>{investigation.id} &&
                    investigationJournal.TryGet(investigation.id)->progress == 1 &&
                    investigationJournal.TryGet(investigation.id)->state ==
                        QuestState::READY_TO_COMPLETE,
                "Talking to the authored character completes an investigation objective");

    QuestDefinition crafting = first;
    crafting.objectiveType = QuestObjectiveType::CRAFT_ITEM;
    crafting.objectiveItem = ItemType::WOODEN_PLANKS;
    crafting.requiredAmount = 4;
    std::vector<QuestDefinition> craftingDefinitions{crafting};
    QuestJournal craftingJournal =
        QuestJournalTestAccess::FromDefinitions(craftingDefinitions);
    QuestJournalTestAccess::ActivateAll(craftingJournal);
    test.Expect(QuestSystemTestAccess::RecordCrafted(craftingJournal,
                    ItemType::LOG, 4, craftingDefinitions).empty() &&
                    QuestSystemTestAccess::RecordCrafted(craftingJournal,
                    ItemType::WOODEN_PLANKS, 0, craftingDefinitions).empty(),
                "Wrong outputs and failed crafts do not advance crafting objectives");
    QuestSystemTestAccess::RecordCrafted(craftingJournal,
        ItemType::WOODEN_PLANKS, 2, craftingDefinitions);
    const auto crafted = QuestSystemTestAccess::RecordCrafted(craftingJournal,
        ItemType::WOODEN_PLANKS, 5, craftingDefinitions);
    test.Expect(crafted == std::vector<QuestId>{crafting.id} &&
                    craftingJournal.TryGet(crafting.id)->progress == 4 &&
                    craftingJournal.TryGet(crafting.id)->state ==
                        QuestState::READY_TO_COMPLETE,
                "Successful crafted output advances and caps a crafting objective");

    QuestJournal choiceJournal;
    QuestSystem::TryRestore(choiceJournal, Records(
        QuestState::READY_TO_COMPLETE, 10));
    Inventory choiceInventory;
    test.Expect(QuestSystemTestAccess::CompleteWithReward(choiceJournal,
                    QuestId::GATHERING_BASICS, choiceInventory,
                    ItemType::COOKED_MEAT, 3) &&
                    choiceInventory.GetItemAmount(ItemType::COOKED_MEAT) == 3 &&
                    choiceInventory.GetItemAmount(ItemType::COINS) == 0 &&
                    !QuestSystem::Complete(choiceJournal,
                        QuestId::GATHERING_BASICS, choiceInventory),
                "A chosen alternate reward completes once and blocks the other outcome");

    QuestDefinition combat = first;
    combat.objectiveType = QuestObjectiveType::DEFEAT_CHARACTER;
    combat.objectiveItem = ItemType::NONE;
    combat.objectiveNpc = NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER;
    combat.objectiveText = "Defeat three Ashfangs on the east road";
    combat.requiredAmount = 3;
    std::vector<QuestDefinition> combatDefinitions{combat};
    QuestJournal combatJournal =
        QuestJournalTestAccess::FromDefinitions(combatDefinitions);
    QuestJournalTestAccess::ActivateAll(combatJournal);
    test.Expect(QuestSystemTestAccess::RecordDefeated(combatJournal,
                    NpcType::PASSIVE_DEVELOPMENT_MONSTER, 3,
                    combatDefinitions).empty() &&
                    QuestSystemTestAccess::RecordDefeated(combatJournal,
                    NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER, 0,
                    combatDefinitions).empty(),
                "Wrong enemies and invalid kill counts do not advance combat objectives");
    QuestSystemTestAccess::RecordDefeated(combatJournal,
        NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER, 1, combatDefinitions);
    const auto defeated = QuestSystemTestAccess::RecordDefeated(combatJournal,
        NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER, 5, combatDefinitions);
    test.Expect(defeated == std::vector<QuestId>{combat.id} &&
                    combatJournal.TryGet(combat.id)->progress == 3 &&
                    combatJournal.TryGet(combat.id)->state ==
                        QuestState::READY_TO_COMPLETE,
                "Authored enemy defeats advance and cap combat objectives");
    return test.Finish();
}

#include "TestSupport.h"
#include "../src/Action/ActionCancelReason.h"
#include "../src/Core/RandomSource.h"
#include "../src/Core/SeededRandom.h"
#include "../src/Item/ItemDatabase.h"
#include "../src/Player/Player.h"
#include "../src/Quest/QuestDefinitionDatabase.h"
#include "../src/Quest/QuestSystem.h"
#include "../src/World/World.h"

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
class RecordingRandom final : public RandomSource
{
public:
    explicit RecordingRandom(std::vector<int> values) : values(std::move(values)) {}
    int NextIntInclusive(int minimum, int maximum) override
    {
        if (next >= values.size()) throw std::runtime_error("RNG exhausted");
        ranges.emplace_back(minimum, maximum);
        return values[next++];
    }
    std::vector<int> values;
    std::size_t next = 0;
    std::vector<std::pair<int, int>> ranges;
};

Player* CreatePlayer(World& world)
{
    auto* player = dynamic_cast<Player*>(world.GetEntityByID(world.CreatePlayer()));
    player->GetPosition().SetPosition(2, 3);
    return player;
}

const QuestRecord* Record(const Player& player, QuestId id)
{
    return player.GetQuestJournal().TryGet(id);
}

const ResourceNode* FindResource(const World& world, ResourceType type)
{
    for (const auto& resource : world.GetResources())
        if (resource.GetResourceType() == type) return &resource;
    return nullptr;
}

int FindSlot(const Inventory& inventory, ItemType type)
{
    const auto& slots = inventory.GetSlots();
    for (int index = 0; index < static_cast<int>(slots.size()); ++index)
        if (!slots[index].IsEmpty() && slots[index].GetItemType() == type)
            return index;
    return -1;
}

bool StartGather(World& world, Player& player, ResourceType type)
{
    const ResourceNode* resource = FindResource(world, type);
    if (resource == nullptr) return false;
    while (resource->GetRemainingUses() == 0) world.Update();
    const int slot = FindSlot(player.GetInventory(), ItemType::BRONZE_PICKAXE);
    if (slot >= 0 && !world.TryEquipInventoryItem(player.GetID(), slot)) return false;
    player.GetPosition().SetPosition(resource->GetX() - 1, resource->GetY());
    world.QueueResourceInteraction(player.GetID(), resource->GetID());
    world.Update();
    return world.GetActionForEntity(player.GetID()) != nullptr;
}

void FinishGather(World& world)
{
    for (int tick = 0;
         tick < ItemDatabase::Get(ItemType::BRONZE_PICKAXE).GetActionDurationTicks();
         ++tick)
        world.Update();
}

const ActiveDialogueSession* OpenGuide(World& world, Player& player)
{
    player.GetPosition().SetPosition(2, 3);
    world.EnqueueCommand(NpcInteractionCommand{
        player.GetID(), 1, NpcInteractionType::TALK});
    world.Update();
    return world.GetActiveDialogueSession(player.GetID());
}

CommandResultCode Result(const World& world, std::uint64_t commandID)
{
    for (const auto& result : world.GetCommandProcessingResults())
        if (result.commandID == commandID) return result.resultCode;
    return CommandResultCode::INVALID_COMMAND_DATA;
}
}

int main()
{
    TestContext test;

    const auto& definitions = QuestDefinitionDatabase::GetAll();
    test.Expect(definitions.size() == 2 &&
                    definitions[0].id == QuestId::GATHERING_BASICS &&
                    definitions[1].id == QuestId::MINING_BASICS,
                "Two-quest catalogue order is stable");
    QuestJournal defaults;
    test.Expect(defaults.GetRecords().size() == 2 &&
                    defaults.TryGet(QuestId::GATHERING_BASICS)->state == QuestState::AVAILABLE &&
                    defaults.TryGet(QuestId::MINING_BASICS)->state == QuestState::AVAILABLE,
                "Both quests initialize independently available");

    {
        QuestJournal journal;
        test.Expect(QuestSystem::Accept(journal, QuestId::MINING_BASICS),
                    "Mining accepts through generic QuestSystem");
        test.Expect(QuestSystem::RecordGathered(journal, ItemType::LOG, 5).empty() &&
                        QuestSystem::RecordGathered(journal, ItemType::TIN_ORE, 5).empty() &&
                        QuestSystem::RecordGathered(journal, ItemType::IRON_ORE, 5).empty() &&
                        QuestSystem::RecordGathered(journal, ItemType::COAL, 5).empty() &&
                        QuestSystem::RecordGathered(journal, ItemType::COPPER_ORE, 0).empty() &&
                        QuestSystem::RecordGathered(journal, ItemType::COPPER_ORE, -1).empty(),
                    "Wrong resources and invalid quantities do not progress Mining");
        QuestSystem::RecordGathered(journal, ItemType::COPPER_ORE, 99);
        test.Expect(journal.TryGet(QuestId::MINING_BASICS)->progress == 5 &&
                        journal.TryGet(QuestId::MINING_BASICS)->state == QuestState::READY_TO_COMPLETE,
                    "Mining progress caps at five");
        test.Expect(journal.TryGet(QuestId::GATHERING_BASICS)->state == QuestState::AVAILABLE,
                    "Mining progress does not affect Gathering Basics");
    }

    {
        auto random = std::make_unique<RecordingRandom>(std::vector<int>(5, 1));
        RecordingRandom* probe = random.get();
        World world(std::make_unique<SeededRandom>(1),
                    std::make_unique<SeededRandom>(2), std::move(random));
        Player* player = CreatePlayer(world);
        const ActiveDialogueSession* session = OpenGuide(world, *player);
        test.Expect(session != nullptr, "Development Guide dialogue opens");
        const auto acceptID = world.EnqueueCommand(QuestAcceptCommand{
            player->GetID(), 1, session->sessionId, QuestId::MINING_BASICS});
        world.Update();
        test.Expect(Result(world, acceptID) == CommandResultCode::ACCEPTED &&
                        Record(*player, QuestId::MINING_BASICS)->state == QuestState::ACTIVE,
                    "Mining accepts through authoritative World queue");
        for (int count = 0; count < 5; ++count)
        {
            test.Expect(StartGather(world, *player, ResourceType::COPPER_ROCK),
                        "Copper gather starts through World action lifecycle");
            FinishGather(world);
        }
        test.Expect(player->GetInventory().GetItemAmount(ItemType::COPPER_ORE) == 5 &&
                        Record(*player, QuestId::MINING_BASICS)->progress == 5 &&
                        Record(*player, QuestId::MINING_BASICS)->state == QuestState::READY_TO_COMPLETE,
                    "Five inserted Copper Ore make Mining ready");
        player->GetInventory().RemoveItem(ItemType::COPPER_ORE, 5);
        test.Expect(Record(*player, QuestId::MINING_BASICS)->progress == 5,
                    "Removing Copper Ore does not reduce recorded progress");
        test.Expect(probe->ranges.size() == 5,
                    "Quest tracking consumes no additional RNG");
        bool orderedRanges = true;
        for (const auto& range : probe->ranges)
            orderedRanges = orderedRanges && range == std::pair<int, int>{1, 100};
        test.Expect(orderedRanges, "Gathering RNG range and order remain unchanged");

        session = OpenGuide(world, *player);
        const auto first = world.EnqueueCommand(QuestCompleteCommand{
            player->GetID(), 1, session->sessionId, QuestId::MINING_BASICS});
        const auto duplicate = world.EnqueueCommand(QuestCompleteCommand{
            player->GetID(), 1, session->sessionId, QuestId::MINING_BASICS});
        world.Update();
        test.Expect(Result(world, first) == CommandResultCode::ACCEPTED &&
                        Result(world, duplicate) == CommandResultCode::GAMEPLAY_REJECTED &&
                        player->GetInventory().GetItemAmount(ItemType::COINS) == 10 &&
                        Record(*player, QuestId::MINING_BASICS)->state == QuestState::COMPLETED,
                    "Same-tick Mining completion rewards exactly ten Coins once");
        test.Expect(Record(*player, QuestId::GATHERING_BASICS)->state == QuestState::AVAILABLE,
                    "Mining completion leaves Gathering unchanged");
    }

    {
        auto random = std::make_unique<RecordingRandom>(std::vector<int>{100, 1});
        World world(std::make_unique<SeededRandom>(3),
                    std::make_unique<SeededRandom>(4), std::move(random));
        Player* player = CreatePlayer(world);
        QuestSystem::Accept(player->GetQuestJournal(), QuestId::MINING_BASICS);
        test.Expect(StartGather(world, *player, ResourceType::COPPER_ROCK),
                    "Failed Copper roll starts");
        FinishGather(world);
        test.Expect(Record(*player, QuestId::MINING_BASICS)->progress == 0,
                    "Failed roll does not progress Mining");
        test.Expect(StartGather(world, *player, ResourceType::COPPER_ROCK),
                    "Cancelled Copper gather starts");
        world.CancelActionsForEntity(player->GetID(), ActionCancelReason::PLAYER_MOVED);
        FinishGather(world);
        test.Expect(Record(*player, QuestId::MINING_BASICS)->progress == 0,
                    "Cancelled action does not progress Mining");
    }

    {
        auto random = std::make_unique<RecordingRandom>(std::vector<int>{1, 1});
        World world(std::make_unique<SeededRandom>(5),
                    std::make_unique<SeededRandom>(6), std::move(random));
        Player* player = CreatePlayer(world);
        QuestSystem::Accept(player->GetQuestJournal(), QuestId::MINING_BASICS);
        test.Expect(StartGather(world, *player, ResourceType::TIN_ROCK),
                    "Tin gather starts through the same World lifecycle");
        FinishGather(world);
        test.Expect(player->GetInventory().GetItemAmount(ItemType::TIN_ORE) == 1 &&
                        Record(*player, QuestId::MINING_BASICS)->progress == 0,
                    "Successfully inserted non-Copper ore does not progress Mining");
        test.Expect(StartGather(world, *player, ResourceType::COPPER_ROCK),
                    "Stale Copper gather starts");
        player->GetPosition().SetPosition(50, 50);
        FinishGather(world);
        test.Expect(Record(*player, QuestId::MINING_BASICS)->progress == 0,
                    "Out-of-range stale completion does not progress Mining");
    }

    {
        auto random = std::make_unique<RecordingRandom>(std::vector<int>{1});
        World world(std::make_unique<SeededRandom>(7),
                    std::make_unique<SeededRandom>(8), std::move(random));
        Player* player = CreatePlayer(world);
        QuestSystem::Accept(player->GetQuestJournal(), QuestId::MINING_BASICS);
        test.Expect(StartGather(world, *player, ResourceType::COPPER_ROCK),
                    "Inventory-full Copper gather starts");
        while (player->GetInventory().AddItem(ItemType::LOG, 1)) {}
        FinishGather(world);
        test.Expect(player->GetInventory().GetItemAmount(ItemType::COPPER_ORE) == 0 &&
                        Record(*player, QuestId::MINING_BASICS)->progress == 0,
                    "Failed Copper insertion does not progress Mining");
    }

    {
        QuestJournal journal;
        test.Expect(QuestSystem::TryRestore(journal, {
                        {QuestId::GATHERING_BASICS, QuestState::READY_TO_COMPLETE, 10},
                        {QuestId::MINING_BASICS, QuestState::READY_TO_COMPLETE, 5}}),
                    "Both quests restore ready for reward test");
        Inventory inventory;
        test.Expect(QuestSystem::Complete(journal, QuestId::GATHERING_BASICS, inventory) &&
                        QuestSystem::Complete(journal, QuestId::MINING_BASICS, inventory) &&
                        inventory.GetItemAmount(ItemType::COINS) == 20,
                    "Completing both quests grants exactly twenty reward Coins");

        QuestJournal fullJournal;
        QuestSystem::TryRestore(fullJournal, {
            {QuestId::GATHERING_BASICS, QuestState::AVAILABLE, 0},
            {QuestId::MINING_BASICS, QuestState::READY_TO_COMPLETE, 5}});
        Inventory full;
        for (int slot = 0; slot < Inventory::SlotCount; ++slot)
            full.AddItem(ItemType::LOG, 1);
        test.Expect(!QuestSystem::Complete(
                        fullJournal, QuestId::MINING_BASICS, full) &&
                        fullJournal.TryGet(QuestId::MINING_BASICS)->state ==
                            QuestState::READY_TO_COMPLETE &&
                        full.GetItemAmount(ItemType::COINS) == 0,
                    "Atomic Mining reward failure preserves ready state");
    }

    return test.Finish();
}

#include "TestSupport.h"
#include "../src/World/World.h"
#include "../src/Player/Player.h"
#include "../src/NPC/NPC.h"
#include "../src/Quest/QuestSystem.h"
#include "../src/Tutorial/StarterJourney.h"

namespace
{
    int Slot(const Player &player, ItemType item)
    {
        const auto &slots = player.GetInventory().GetSlots();
        for (int i = 0; i < Inventory::SlotCount; ++i)
            if (!slots[i].IsEmpty() && slots[i].GetItemType() == item) return i;
        return -1;
    }
    NPC *FindNpc(World &world, NpcType type)
    {
        for (const auto &entity : world.GetEntities())
            if (auto *npc = dynamic_cast<NPC *>(entity.get()); npc && npc->GetNpcType() == type)
                return npc;
        return nullptr;
    }
    void Talk(World &world, Player &player, NPC &npc)
    {
        player.GetPosition().SetPosition(npc.GetPosition().GetX() - 1, npc.GetPosition().GetY());
        world.EnqueueCommand(NpcInteractionCommand{player.GetID(), npc.GetID(), NpcInteractionType::TALK});
        world.Update();
    }
}

int main()
{
    TestContext test;
    {
        World world(71U);
        auto &first = *dynamic_cast<Player *>(world.GetEntityByID(world.CreatePlayer(PlayerInitializationMode::FRESH_START)));
        auto &second = *dynamic_cast<Player *>(world.GetEntityByID(world.CreatePlayer(PlayerInitializationMode::FRESH_START)));
        for (auto *player : {&first, &second})
        {
            player->GetInventory().AddItem(ItemType::COINS, 2);
            player->GetBank().TryDeposit(player->GetInventory(), Slot(*player, ItemType::COINS), 1);
            player->GetTutorialState().TryRestore(TutorialStep::LOOT_AND_BANK, false);
        }
        first.GetPosition().SetPosition(19, 3);
        auto *bank = world.GetStationAt(20, 3);
        test.Expect(bank != nullptr, "Authored bank exists for checkpoint fixture");
        if (!bank) return test.Finish();
        world.EnqueueCommand(InteractCommand{first.GetID(), InteractionTargetType::STATION,
            bank->GetID(), Position{20, 3}});
        world.Update();
        test.Expect(first.GetTutorialState().GetStep() == TutorialStep::LOOT_AND_BANK &&
            second.GetTutorialState().GetStep() == TutorialStep::LOOT_AND_BANK,
            "Historic bank contents and opening a session cannot complete a new deposit checkpoint");
        const auto *session = world.GetBankSession(first.GetID());
        test.Expect(session != nullptr, "Real bank session opens");
        if (!session) return test.Finish();
        const auto sessionID = session->sessionId;
        world.EnqueueCommand(BankWithdrawCommand{first.GetID(), sessionID,
            first.GetBank().GetRevision(), ItemType::COINS, 1});
        world.Update();
        test.Expect(first.GetTutorialState().GetStep() == TutorialStep::LOOT_AND_BANK,
            "A successful withdrawal cannot impersonate a deposit");
        world.EnqueueCommand(BankDepositCommand{first.GetID(), sessionID,
            first.GetBank().GetRevision(), Slot(first, ItemType::COINS), 0});
        world.Update();
        test.Expect(first.GetTutorialState().GetStep() == TutorialStep::LOOT_AND_BANK,
            "A rejected deposit cannot complete the checkpoint");
        world.EnqueueCommand(BankDepositCommand{first.GetID(), sessionID,
            first.GetBank().GetRevision(), Slot(first, ItemType::COINS), 1});
        world.Update();
        test.Expect(first.GetTutorialState().GetStep() == TutorialStep::RETURN_AND_CHOOSE,
            "An accepted deposit with a real revision change completes the checkpoint");
        test.Expect(second.GetTutorialState().GetStep() == TutorialStep::LOOT_AND_BANK,
            "Another actor cannot inherit the depositor's progress");
        first.GetTutorialState().TryRestore(TutorialStep::LOOT_AND_BANK, false);
        world.Update();
        test.Expect(first.GetTutorialState().GetStep() == TutorialStep::LOOT_AND_BANK,
            "A prior tick's deposit cannot complete a later checkpoint");
        world.EnqueueCommand(BankDepositInventoryCommand{first.GetID(), sessionID, first.GetBank().GetRevision()});
        world.Update();
        test.Expect(first.GetTutorialState().GetStep() == TutorialStep::RETURN_AND_CHOOSE,
            "A committed deposit-all also completes the checkpoint");
        first.GetTutorialState().TryRestore(TutorialStep::LOOT_AND_BANK, false);
        world.EnqueueCommand(BankDepositInventoryCommand{first.GetID(), sessionID, first.GetBank().GetRevision()});
        world.Update();
        test.Expect(first.GetTutorialState().GetStep() == TutorialStep::LOOT_AND_BANK,
            "An empty deposit-all cannot manufacture a new deposit");
    }
    {
        World world(73U);
        auto &player = *dynamic_cast<Player *>(world.GetEntityByID(world.CreatePlayer(PlayerInitializationMode::FRESH_START)));
        test.Expect(QuestSystem::Accept(player.GetQuestJournal(), QuestId::GATHERING_BASICS),
            "Gathering quest is available for early-reward fixture");
        QuestSystem::RecordGathered(player.GetQuestJournal(), ItemType::LOG, 10);
        test.Expect(QuestSystem::Complete(player.GetQuestJournal(), QuestId::GATHERING_BASICS, player.GetInventory()),
            "Early reward can legitimately precede combat");
        player.GetTutorialState().TryRestore(TutorialStep::RETURN_AND_CHOOSE, false);
        world.Update();
        test.Expect(player.GetTutorialState().GetStep() == TutorialStep::RETURN_AND_CHOOSE,
            "An earlier claimed reward does not prove a physical return to Mara");
        const auto guidance = StarterJourney::Describe(player);
        test.Expect(guidance.objective == "Return to Mara" &&
            guidance.progress.find("already claimed") != std::string::npos,
            "Guidance does not ask for an already claimed reward again");
        auto *other = FindNpc(world, NpcType::TORREN_ASH);
        auto *mara = FindNpc(world, NpcType::DEVELOPMENT_GUIDE);
        test.Expect(other && mara, "Both dialogue targets exist");
        if (!other || !mara) return test.Finish();
        Talk(world, player, *other);
        test.Expect(player.GetTutorialState().GetStep() == TutorialStep::RETURN_AND_CHOOSE,
            "Talking to another character does not impersonate returning to Mara");
        Talk(world, player, *mara);
        test.Expect(player.GetTutorialState().IsCompleted() && !player.GetTutorialState().IsSkipped(),
            "A real return to Mara completes the already-rewarded route");

        auto &unfinished = *dynamic_cast<Player *>(world.GetEntityByID(world.CreatePlayer(PlayerInitializationMode::FRESH_START)));
        unfinished.GetTutorialState().TryRestore(TutorialStep::RETURN_AND_CHOOSE, false);
        Talk(world, unfinished, *mara);
        test.Expect(unfinished.GetTutorialState().GetStep() == TutorialStep::RETURN_AND_CHOOSE,
            "Mara conversation still requires the outstanding Gathering Basics reward");
    }
    return test.Finish();
}

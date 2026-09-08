#include "TestSupport.h"
#include "../src/Command/ServerCommand.h"
#include "../src/Core/RandomSource.h"
#include "../src/Player/Player.h"
#include "../src/World/World.h"

#include <climits>
#include <memory>

namespace
{
    class CountingRandomSource final : public RandomSource
    {
    public:
        int NextIntInclusive(int minimum, int) override
        {
            ++calls;
            return minimum;
        }
        int calls = 0;
    };

    Player *PlayerAt(World &world, int x, int y)
    {
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(world.CreatePlayer()));
        player->GetPosition().SetPosition(x, y);
        return player;
    }
    void Trade(World &world, int actor)
    { world.EnqueueCommand(NpcInteractionCommand{actor, 1, NpcInteractionType::TRADE}); }
}

int main()
{
    TestContext test;
    World world;
    Player *player = PlayerAt(world, 2, 3);
    player->GetInventory().AddItem(ItemType::COINS, 100);
    Trade(world, player->GetID());
    world.Update();
    test.ExpectEqual(world.GetShopOpenedEvents().size(), std::size_t{1},
                     "Adjacent trade opens on tick one");
    const ShopOpenedEvent opened = world.GetShopOpenedEvents().front();
    test.Expect(opened.sessionId != InvalidShopSessionId && opened.entries.size() == 9,
                "Open event owns session and ordered entries");
    world.EnqueueCommand(ShopBuyCommand{player->GetID(), opened.sessionId,
        ItemType::COOKED_MEAT, 2});
    world.Update();
    test.ExpectEqual(player->GetInventory().GetItemAmount(ItemType::COINS), 84,
                     "Buy removes exact coins");
    test.ExpectEqual(player->GetInventory().GetItemAmount(ItemType::COOKED_MEAT), 2,
                     "Buy adds exact items");
    test.ExpectEqual(world.GetShopTransactionEvents().size(), std::size_t{1},
                     "Buy publishes one event");
    world.EnqueueCommand(ShopSellCommand{player->GetID(), opened.sessionId,
        ItemType::COOKED_MEAT, 1});
    world.Update();
    test.ExpectEqual(player->GetInventory().GetItemAmount(ItemType::COINS), 87,
                     "Sell adds exact coins");
    test.ExpectEqual(player->GetInventory().GetItemAmount(ItemType::COOKED_MEAT), 1,
                     "Sell removes exact item");
    const Inventory before = player->GetInventory();
    world.EnqueueCommand(ShopBuyCommand{player->GetID(), opened.sessionId,
        ItemType::LOG, 1});
    world.Update();
    test.ExpectEqual(player->GetInventory().GetItemAmount(ItemType::COINS),
                     before.GetItemAmount(ItemType::COINS) - 1,
                     "Authored one-Coin Log buy removes exact currency");
    test.ExpectEqual(player->GetInventory().GetItemAmount(ItemType::LOG), 1,
                     "Authored Log buy grants one Log");
    test.Expect(world.GetActiveShopSession(player->GetID()) != nullptr,
                "Malformed transaction preserves valid session");
    world.EnqueueCommand(ShopCloseCommand{player->GetID(), opened.sessionId});
    world.Update();
    test.Expect(world.GetActiveShopSession(player->GetID()) == nullptr,
                "Matching close ends session");

    World distant;
    Player *walker = PlayerAt(distant, 0, 0);
    Trade(distant, walker->GetID());
    for (int tick = 1; tick <= 5; ++tick)
    {
        distant.Update();
        if (tick < 5)
            test.Expect(distant.GetShopOpenedEvents().empty(), "No early open event");
    }
    test.Expect(walker->GetPosition().GetX() == 3 && walker->GetPosition().GetY() == 2,
                "Distant route reaches (3,2) on tick five");
    test.ExpectEqual(distant.GetShopOpenedEvents().size(), std::size_t{1},
                     "Distant trade opens on tick five");
    test.Expect(!distant.HasActiveMovementPath(walker->GetID()),
                "Approach movement clears after opening");
    distant.Update();
    test.Expect(distant.GetShopOpenedEvents().empty(),
                "Open event is not republished on tick six");

    World ordering;
    Player *actorA = PlayerAt(ordering, 2, 3);
    Player *actorB = PlayerAt(ordering, 4, 3);
    actorA->GetInventory().AddItem(ItemType::COINS, 100);
    actorB->GetInventory().AddItem(ItemType::LOG, 2);
    Trade(ordering, actorA->GetID());
    Trade(ordering, actorB->GetID());
    ordering.Update();
    const ShopSessionId sessionA =
        ordering.GetActiveShopSession(actorA->GetID())->sessionId;
    const ShopSessionId sessionB =
        ordering.GetActiveShopSession(actorB->GetID())->sessionId;
    test.Expect(sessionA != sessionB, "Multiple actors receive unique sessions");
    ordering.EnqueueCommand(ShopSellCommand{actorB->GetID(), sessionB,
        ItemType::LOG, 1});
    ordering.EnqueueCommand(ShopBuyCommand{actorA->GetID(), sessionA,
        ItemType::COOKED_MEAT, 1});
    ordering.Update();
    test.ExpectEqual(ordering.GetShopTransactionEvents().size(), std::size_t{2},
                     "Two actors transact in one update");
    test.Expect(ordering.GetShopTransactionEvents()[0].actorEntityID == actorB->GetID() &&
                    ordering.GetShopTransactionEvents()[1].actorEntityID == actorA->GetID(),
                "Transaction events follow reverse-ID command queue order");
    test.ExpectEqual(actorB->GetInventory().GetItemAmount(ItemType::COINS), 1,
                     "Actor B sale updates only B");
    test.ExpectEqual(actorA->GetInventory().GetItemAmount(ItemType::COOKED_MEAT), 1,
                     "Actor A purchase updates only A");
    ordering.EnqueueCommand(ShopCloseCommand{actorA->GetID(), sessionA});
    ordering.Update();
    test.Expect(ordering.GetActiveShopSession(actorA->GetID()) == nullptr &&
                    ordering.GetActiveShopSession(actorB->GetID()) != nullptr,
                "Closing actor A preserves actor B");

    World sameUpdate;
    Player *sequenced = PlayerAt(sameUpdate, 2, 3);
    sequenced->GetInventory().AddItem(ItemType::COINS, 16);
    Trade(sameUpdate, sequenced->GetID());
    sameUpdate.Update();
    const ShopSessionId sequenceSession =
        sameUpdate.GetActiveShopSession(sequenced->GetID())->sessionId;
    sameUpdate.EnqueueCommand(ShopBuyCommand{sequenced->GetID(), sequenceSession,
        ItemType::COOKED_MEAT, 1});
    sameUpdate.EnqueueCommand(ShopBuyCommand{sequenced->GetID(), sequenceSession,
        ItemType::COOKED_MEAT, 1});
    sameUpdate.EnqueueCommand(ShopCloseCommand{sequenced->GetID(), sequenceSession});
    sameUpdate.EnqueueCommand(ShopBuyCommand{sequenced->GetID(), sequenceSession,
        ItemType::COOKED_MEAT, 1});
    sameUpdate.Update();
    test.ExpectEqual(sequenced->GetInventory().GetItemAmount(ItemType::COINS), 0,
                     "Same-update BUY commands see prior inventory mutation");
    test.ExpectEqual(sequenced->GetInventory().GetItemAmount(ItemType::COOKED_MEAT), 2,
                     "CLOSE before final BUY prevents third purchase");
    test.ExpectEqual(sameUpdate.GetShopTransactionEvents().size(), std::size_t{2},
                     "Only successful pre-close transactions publish");

    World failures;
    Player *failureActor = PlayerAt(failures, 2, 3);
    failureActor->GetInventory().AddItem(ItemType::COINS, INT_MAX);
    Trade(failures, failureActor->GetID());
    failures.Update();
    const ShopSessionId failureSession =
        failures.GetActiveShopSession(failureActor->GetID())->sessionId;
    const Inventory failureBefore = failureActor->GetInventory();
    failures.EnqueueCommand(ShopBuyCommand{failureActor->GetID(), failureSession,
        ItemType::COOKED_MEAT, INT_MAX});
    failures.EnqueueCommand(ShopSellCommand{failureActor->GetID(), failureSession,
        ItemType::COINS, 1});
    failures.EnqueueCommand(ShopCloseCommand{failureActor->GetID(), failureSession + 1});
    failures.Update();
    test.ExpectEqual(failureActor->GetInventory().GetItemAmount(ItemType::COINS),
                     failureBefore.GetItemAmount(ItemType::COINS),
                     "Overflow and currency sale failures preserve inventory");
    test.Expect(failures.GetShopTransactionEvents().empty() &&
                    failures.GetActiveShopSession(failureActor->GetID()) != nullptr,
                "Malformed and stale commands preserve session and publish nothing");

    auto combatRandom = std::make_unique<CountingRandomSource>();
    auto rewardRandom = std::make_unique<CountingRandomSource>();
    auto gatheringRandom = std::make_unique<CountingRandomSource>();
    CountingRandomSource *combatCounter = combatRandom.get();
    CountingRandomSource *rewardCounter = rewardRandom.get();
    CountingRandomSource *gatheringCounter = gatheringRandom.get();
    World isolated(std::move(combatRandom), std::move(rewardRandom),
                   std::move(gatheringRandom));
    Player *isolatedActor = PlayerAt(isolated, 2, 3);
    isolatedActor->GetInventory().AddItem(ItemType::COINS, 8);
    Trade(isolated, isolatedActor->GetID());
    isolated.Update();
    const ShopSessionId isolatedSession =
        isolated.GetActiveShopSession(isolatedActor->GetID())->sessionId;
    isolated.EnqueueCommand(ShopBuyCommand{isolatedActor->GetID(), isolatedSession,
        ItemType::COOKED_MEAT, 1});
    isolated.EnqueueCommand(ShopSellCommand{isolatedActor->GetID(), isolatedSession,
        ItemType::COOKED_MEAT, 1});
    isolated.EnqueueCommand(ShopCloseCommand{isolatedActor->GetID(), isolatedSession});
    isolated.Update();
    test.ExpectEqual(combatCounter->calls, 0, "Shop commands consume no combat RNG");
    test.ExpectEqual(rewardCounter->calls, 0, "Shop commands consume no reward RNG");
    test.ExpectEqual(gatheringCounter->calls, 0, "Shop commands consume no gathering RNG");
    test.Expect(isolated.GetActionForEntity(isolatedActor->GetID()) == nullptr &&
                    !isolated.HasPendingMeleeEngagement(isolatedActor->GetID()),
                "Shop commands create no action or melee engagement");
    test.Expect(isolated.GetLastMeleeAttackResult() == std::nullopt &&
                    isolated.GetMeleeCombatFeedbacks().empty() &&
                    isolated.GetEntityDiedEvents().empty(),
                "Shop commands produce no combat, feedback, or death output");
    return test.Finish();
}

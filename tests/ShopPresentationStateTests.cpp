#include "TestSupport.h"
#include "../src/Shop/ShopPresentationState.h"

#include <variant>

int main()
{
    TestContext test;
    ShopPresentationState presentation;
    ShopOpenedEvent event{
        4,
        1,
        NpcType::DEVELOPMENT_GUIDE,
        77,
        ShopId::DEVELOPMENT_GUIDE_SUPPLIES,
        "Supplies",
        ItemType::COINS,
        {
            {ItemType::LOG, 8, std::nullopt},
            {ItemType::OAK_LOG, std::nullopt, 3},
            {ItemType::WILLOW_LOG, 12, 4},
            {ItemType::COPPER_ORE, std::nullopt, std::nullopt},
        }};
    ActiveShopSession active{
        77,
        4,
        1,
        NpcType::DEVELOPMENT_GUIDE,
        ShopId::DEVELOPMENT_GUIDE_SUPPLIES,
        1};

    presentation.Synchronize(4, {event}, &active);
    test.Expect(presentation.IsOpen(), "Matching identity opens shop");

    const auto buyOnly = presentation.MakeBuy(0, 2);
    const auto *buy = buyOnly
        ? std::get_if<ShopBuyCommand>(&*buyOnly) : nullptr;
    test.Expect(buy != nullptr && buy->actorEntityID == 4 &&
                    buy->shopSessionId == 77 &&
                    buy->itemType == ItemType::LOG && buy->quantity == 2,
                "Buy requires price and preserves exact command identity");
    test.Expect(!presentation.MakeSell(0).has_value(),
                "Buy-only row cannot manufacture Sell command");

    const auto sellOnly = presentation.MakeSell(1, 3);
    const auto *sell = sellOnly
        ? std::get_if<ShopSellCommand>(&*sellOnly) : nullptr;
    test.Expect(sell != nullptr && sell->actorEntityID == 4 &&
                    sell->shopSessionId == 77 &&
                    sell->itemType == ItemType::OAK_LOG && sell->quantity == 3,
                "Sell requires price and preserves exact command identity");
    test.Expect(!presentation.MakeBuy(1).has_value(),
                "Sell-only row cannot manufacture Buy command");

    test.Expect(presentation.MakeBuy(2).has_value() &&
                    presentation.MakeSell(2).has_value(),
                "Both-price row exposes Buy and Sell commands");
    test.Expect(!presentation.MakeBuy(3).has_value() &&
                    !presentation.MakeSell(3).has_value(),
                "Unavailable row exposes neither command");
    test.Expect(!presentation.MakeBuy(2, 0).has_value() &&
                    !presentation.MakeSell(2, -1).has_value(),
                "Non-positive quantities remain rejected");

    presentation.ReconcileRejectedCommand();
    presentation.Synchronize(4, {}, &active);
    test.Expect(presentation.HasRejection(),
                "No-event synchronization preserves rejection feedback");
    presentation.Synchronize(4, {event}, &active);
    test.Expect(!presentation.HasRejection(),
                "Authoritative replacement clears rejection feedback");

    ActiveShopSession mismatch = active;
    mismatch.npcEntityID = 9;
    presentation.ReconcileRejectedCommand();
    presentation.Synchronize(4, {event}, &mismatch);
    test.Expect(!presentation.IsOpen() && !presentation.HasRejection(),
                "NPC mismatch clears shop and feedback");

    presentation.Synchronize(4, {event}, &active);
    ShopOpenedEvent bad = event;
    bad.sessionId = 78;
    presentation.ReconcileRejectedCommand();
    presentation.Synchronize(4, {bad}, &active);
    test.Expect(!presentation.IsOpen() && !presentation.HasRejection(),
                "Session mismatch clears shop and feedback");

    presentation.Synchronize(4, {event}, &active);
    presentation.Dismiss();
    presentation.Synchronize(4, {event}, &active);
    test.Expect(!presentation.IsOpen() && !presentation.HasRejection(),
                "Dismissed session stays closed without feedback");
    presentation.Synchronize(4, {}, nullptr);
    test.Expect(!presentation.MakeBuy(0).has_value() &&
                    !presentation.MakeSell(0).has_value() &&
                    !presentation.MakeClose().has_value(),
                "Stale presentation cannot create shop commands");

    return test.Finish();
}

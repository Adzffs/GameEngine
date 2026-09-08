#include "TestSupport.h"
#include "../src/Core/Engine.h"

struct GraphicsTestAccess
{
    static bool Click(Graphics& graphics, float x, float y)
    {
        return graphics.HandleShopClick(x, y);
    }
};

struct EngineTestAccess
{
    static bool Init(Engine& engine) { return engine.InitializeWorldForRun(); }
    static World& W(Engine& engine) { return engine.world; }
    static Graphics& G(Engine& engine) { return engine.graphics; }
    static int Player(Engine& engine) { return engine.playerID; }
    static bool Enqueue(Engine& engine) { return engine.EnqueuePendingShopCommand(); }
    static std::uint64_t Pending(const Engine& engine) { return engine.pendingShopCommandID; }
    static bool PendingBuy(const Engine& engine)
    {
        return engine.pendingShopCommandType == Engine::PendingShopCommandType::BUY;
    }
    static bool PendingNone(const Engine& engine)
    {
        return engine.pendingShopCommandType == Engine::PendingShopCommandType::NONE;
    }
    static void Sync(Engine& engine) { engine.SynchronizeShopPresentation(); }
};

int main()
{
    TestContext test;
    Engine engine;
    test.Expect(EngineTestAccess::Init(engine), "Engine initializes");
    World& world = EngineTestAccess::W(engine);
    Graphics& graphics = EngineTestAccess::G(engine);
    const int playerID = EngineTestAccess::Player(engine);
    auto* player = dynamic_cast<Player*>(world.GetEntityByID(playerID));
    player->GetPosition().SetPosition(2, 3);
    player->GetInventory().AddItem(ItemType::COINS, 8);

    world.EnqueueCommand(NpcInteractionCommand{
        playerID, 1, NpcInteractionType::TRADE});
    world.Update();
    test.Expect(!world.GetShopOpenedEvents().empty(),
                "Trade session opens after World tick");
    graphics.SynchronizeShop(
        playerID, world.GetShopOpenedEvents(),
        world.GetActiveShopSession(playerID));

    const SDL_FRect row = graphics.GetShopRowRectangle(0);
    GraphicsTestAccess::Click(
        graphics, row.x + row.w - 250, row.y + row.h / 2);
    test.Expect(EngineTestAccess::Enqueue(engine),
                "Log Buy queues from SDL hitbox");
    world.Update();
    test.Expect(player->GetInventory().GetItemAmount(ItemType::COINS) == 7 &&
                    player->GetInventory().GetItemAmount(ItemType::LOG) == 1,
                "Accepted one-Coin Log buy applies on World tick");
    EngineTestAccess::Sync(engine);

    GraphicsTestAccess::Click(
        graphics, row.x + row.w - 100, row.y + row.h / 2);
    test.Expect(EngineTestAccess::Enqueue(engine),
                "Log Sell queues from SDL hitbox");
    world.Update();
    test.Expect(player->GetInventory().GetItemAmount(ItemType::LOG) == 0 &&
                    player->GetInventory().GetItemAmount(ItemType::COINS) == 8,
                "Accepted Log sell applies on World tick");
    EngineTestAccess::Sync(engine);

    test.Expect(player->GetInventory().RemoveItem(ItemType::COINS, 8),
                "Rejected-buy fixture removes spending currency");
    const int coins = player->GetInventory().GetItemAmount(ItemType::COINS);
    const int logs = player->GetInventory().GetItemAmount(ItemType::LOG);
    test.Expect(coins == 0,
                "Rejected-buy fixture has exactly zero spendable Coins");
    const auto* shopEvent = graphics.GetShopPresentationState().GetEvent();
    test.Expect(shopEvent != nullptr && !shopEvent->entries.empty() &&
                    shopEvent->entries[0].itemType == ItemType::LOG &&
                    shopEvent->entries[0].buyPrice.has_value() &&
                    *shopEvent->entries[0].buyPrice == 1,
                "First rendered shop row is the authoritative one-Coin Log offer");
    const SDL_FRect buyButton = graphics.GetShopBuyButtonRectangle(0);
    test.Expect(GraphicsTestAccess::Click(
                    graphics,
                    buyButton.x + buyButton.w / 2,
                    buyButton.y + buyButton.h / 2),
                "Click targets the rendered Log Buy control");
    test.Expect(EngineTestAccess::Enqueue(engine),
                "Rejected Log buy queues from SDL hitbox");
    const auto commandID = EngineTestAccess::Pending(engine);
    test.Expect(commandID != 0 && EngineTestAccess::PendingBuy(engine),
                "Exactly one pending command is identified as Buy");
    world.Update();
    std::size_t matchingResults = 0;
    CommandResultCode resultCode = CommandResultCode::ACCEPTED;
    for (const auto& result : world.GetCommandProcessingResults()) {
        if (result.commandID == commandID && result.actorEntityID == playerID) {
            ++matchingResults;
            resultCode = result.resultCode;
        }
    }
    test.Expect(matchingResults == 1,
                "World publishes exactly one result for the Buy command");
    test.Expect(resultCode == CommandResultCode::GAMEPLAY_REJECTED,
                "Server rejects the Buy command for insufficient currency");
    EngineTestAccess::Sync(engine);
    test.Expect(EngineTestAccess::Pending(engine) == 0 &&
                    EngineTestAccess::PendingNone(engine),
                "Rejected Buy result is consumed exactly once");
    test.Expect(player->GetInventory().GetItemAmount(ItemType::COINS) == coins,
                "Rejected Buy leaves Coins unchanged");
    test.Expect(player->GetInventory().GetItemAmount(ItemType::LOG) == logs,
                "Rejected Buy leaves Logs unchanged");
    test.Expect(graphics.GetShopPresentationState().IsOpen(),
                "Rejected Buy keeps the shop open");
    test.Expect(graphics.GetShopPresentationState().HasRejection(),
                "Rejected Buy exposes transaction feedback");
    return test.Finish();
}

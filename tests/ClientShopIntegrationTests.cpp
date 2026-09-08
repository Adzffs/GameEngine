#include "TestSupport.h"
#include "../src/Core/Engine.h"

struct GraphicsTestAccess
{
    static bool HasPending(const Graphics &graphics)
    {
        return graphics.pendingShopCommand.has_value();
    }
    static void Dismiss(Graphics &graphics)
    {
        graphics.shopPresentationState.Dismiss();
    }
};

struct WorldTestAccess
{
    static void PublishResults(
        World &world, std::vector<CommandProcessingResult> results)
    {
        world.publishedCommandProcessingResults = std::move(results);
    }
    static void ReplaceSession(World &world, int actor, int npc)
    {
        world.shopSystem.Start(actor, npc, NpcType::DEVELOPMENT_GUIDE,
            ShopId::DEVELOPMENT_GUIDE_SUPPLIES, world.currentTick);
    }
};

struct EngineTestAccess
{
    static bool Init(Engine &engine) { return engine.InitializeWorldForRun(); }
    static World &W(Engine &engine) { return engine.world; }
    static Graphics &G(Engine &engine) { return engine.graphics; }
    static int Player(const Engine &engine) { return engine.playerID; }
    static bool Enqueue(Engine &engine) { return engine.EnqueuePendingShopCommand(); }
    static void Sync(Engine &engine) { engine.SynchronizeShopPresentation(); }
    static bool HasPending(const Engine &engine)
    {
        return engine.pendingShopCommand.has_value();
    }
    static std::uint64_t ID(const Engine &engine)
    {
        return HasPending(engine) ? engine.pendingShopCommand->commandID : 0;
    }
    static int Actor(const Engine &engine)
    {
        return HasPending(engine) ? engine.pendingShopCommand->actorEntityID : 0;
    }
    static ShopSessionId Session(const Engine &engine)
    {
        return HasPending(engine) ? engine.pendingShopCommand->sessionID
                                  : InvalidShopSessionId;
    }
    static bool IsType(
        const Engine &engine, Engine::PendingShopCommandType type)
    {
        return HasPending(engine) && engine.pendingShopCommand->type == type;
    }
    static auto Buy() { return Engine::PendingShopCommandType::BUY; }
    static auto Sell() { return Engine::PendingShopCommandType::SELL; }
    static auto Close() { return Engine::PendingShopCommandType::CLOSE; }
};

namespace
{
    bool Click(Graphics &graphics, const SDL_FRect &rectangle)
    {
        if ((SDL_WasInit(SDL_INIT_EVENTS) & SDL_INIT_EVENTS) == 0 &&
            !SDL_InitSubSystem(SDL_INIT_EVENTS))
            return false;
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = rectangle.x + rectangle.w / 2.0f;
        event.button.y = rectangle.y + rectangle.h / 2.0f;
        if (!SDL_PushEvent(&event)) return false;
        bool running = true;
        graphics.ProcessEvents(running);
        return running;
    }

    Player *OpenShop(Engine &engine)
    {
        World &world = EngineTestAccess::W(engine);
        Graphics &graphics = EngineTestAccess::G(engine);
        const int actor = EngineTestAccess::Player(engine);
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(actor));
        if (player == nullptr) return nullptr;
        player->GetPosition().SetPosition(2, 3);
        world.EnqueueCommand(
            NpcInteractionCommand{actor, 1, NpcInteractionType::TRADE});
        world.Update();
        graphics.SynchronizeShop(actor, world.GetShopOpenedEvents(),
            world.GetActiveShopSession(actor));
        return player;
    }

    std::size_t CountResult(
        const World &world, std::uint64_t commandID, int actor)
    {
        std::size_t count = 0;
        for (const auto &result : world.GetCommandProcessingResults())
            if (result.commandID == commandID &&
                result.actorEntityID == actor) ++count;
        return count;
    }

    void TickAndSync(Engine &engine)
    {
        EngineTestAccess::W(engine).Update();
        EngineTestAccess::Sync(engine);
    }
}

int main()
{
    TestContext test;
    Engine engine;
    Graphics &graphics = EngineTestAccess::G(engine);
    test.Expect(EngineTestAccess::Init(engine), "Engine initializes");
    World &world = EngineTestAccess::W(engine);
    const int actor = EngineTestAccess::Player(engine);
    Player *player = OpenShop(engine);
    test.Expect(player != nullptr &&
                    graphics.GetShopPresentationState().IsOpen(),
                "Authoritative shop opens in client presentation");
    player->GetInventory().AddItem(ItemType::COINS, 100);
    player->GetInventory().AddItem(ItemType::LOG, 2);

    const SDL_FRect buy = graphics.GetShopBuyButtonRectangle(0);
    const SDL_FRect sell = graphics.GetShopSellButtonRectangle(0);
    const SDL_FRect close = graphics.GetShopCloseButtonRectangle();

    // The critical real SDL sequence: first click -> Engine enqueue -> second
    // click before World tick.
    test.Expect(Click(graphics, buy) && EngineTestAccess::Enqueue(engine),
                "First SDL Buy event reaches Engine through ProcessEvents");
    const std::uint64_t firstBuyID = EngineTestAccess::ID(engine);
    test.Expect(firstBuyID != 0 && EngineTestAccess::Actor(engine) == actor &&
                    EngineTestAccess::Session(engine) ==
                        graphics.GetShopPresentationState().GetEvent()->sessionId &&
                    EngineTestAccess::IsType(engine, EngineTestAccess::Buy()) &&
                    graphics.IsShopCommandInFlight(),
                "Lock records exact Buy ID, actor, session and type");
    Click(graphics, buy);
    test.Expect(!GraphicsTestAccess::HasPending(graphics) &&
                    !EngineTestAccess::Enqueue(engine) &&
                    EngineTestAccess::ID(engine) == firstBuyID,
                "Buy to Buy rejects second click without overwriting first");
    world.Update();
    test.Expect(CountResult(world, firstBuyID, actor) == 1,
                "Only first rapid Buy reaches World");
    EngineTestAccess::Sync(engine);
    test.Expect(!EngineTestAccess::HasPending(engine) &&
                    !graphics.IsShopCommandInFlight(),
                "Matching result releases Buy lock exactly once");
    test.Expect(player->GetInventory().GetItemAmount(ItemType::COINS) == 99 &&
                    player->GetInventory().GetItemAmount(ItemType::LOG) == 3,
                "Accepted Buy still updates economy on authoritative tick");
    const int acceptedCoins = player->GetInventory().GetItemAmount(ItemType::COINS);
    const int acceptedLogs = player->GetInventory().GetItemAmount(ItemType::LOG);
    EngineTestAccess::Sync(engine);
    test.Expect(!EngineTestAccess::HasPending(engine) &&
                    player->GetInventory().GetItemAmount(ItemType::COINS) == acceptedCoins &&
                    player->GetInventory().GetItemAmount(ItemType::LOG) == acceptedLogs,
                "Duplicate result observation cannot reconcile twice");

    Click(graphics, buy);
    test.Expect(EngineTestAccess::Enqueue(engine),
                "Buy cross-control fixture enters flight");
    const std::uint64_t buyCrossID = EngineTestAccess::ID(engine);
    Click(graphics, sell);
    test.Expect(!EngineTestAccess::Enqueue(engine) &&
                    EngineTestAccess::ID(engine) == buyCrossID &&
                    EngineTestAccess::IsType(engine, EngineTestAccess::Buy()),
                "Buy to Sell rejects Sell without overwriting Buy");
    Click(graphics, close);
    test.Expect(!EngineTestAccess::Enqueue(engine) &&
                    EngineTestAccess::ID(engine) == buyCrossID &&
                    EngineTestAccess::IsType(engine, EngineTestAccess::Buy()),
                "Buy to Close rejects Close without overwriting Buy");

    WorldTestAccess::PublishResults(world, {
        {buyCrossID + 100, actor, CommandResultCode::GAMEPLAY_REJECTED}});
    EngineTestAccess::Sync(engine);
    test.Expect(EngineTestAccess::ID(engine) == buyCrossID &&
                    graphics.IsShopCommandInFlight() &&
                    !graphics.GetShopPresentationState().HasRejection(),
                "Wrong-ID result cannot reconcile or unlock Buy");
    WorldTestAccess::PublishResults(world, {
        {buyCrossID, actor + 100, CommandResultCode::GAMEPLAY_REJECTED}});
    EngineTestAccess::Sync(engine);
    test.Expect(EngineTestAccess::ID(engine) == buyCrossID &&
                    graphics.IsShopCommandInFlight() &&
                    !graphics.GetShopPresentationState().HasRejection(),
                "Wrong-actor result cannot reconcile or unlock Buy");
    TickAndSync(engine);

    Click(graphics, close);
    test.Expect(EngineTestAccess::Enqueue(engine) &&
                    EngineTestAccess::IsType(engine, EngineTestAccess::Close()),
                "Close enters flight with explicit Close type");
    const std::uint64_t closeID = EngineTestAccess::ID(engine);
    Click(graphics, buy);
    Click(graphics, sell);
    test.Expect(!EngineTestAccess::Enqueue(engine) &&
                    EngineTestAccess::ID(engine) == closeID &&
                    EngineTestAccess::IsType(engine, EngineTestAccess::Close()),
                "Close to Buy or Sell rejects both without overwriting Close");
    world.Update();
    test.Expect(CountResult(world, closeID, actor) == 1,
                "Only first rapid Close reaches World");
    EngineTestAccess::Sync(engine);
    test.Expect(!graphics.GetShopPresentationState().IsOpen() &&
                    !EngineTestAccess::HasPending(engine) &&
                    !graphics.IsShopCommandInFlight(),
                "Accepted Close closes panel and releases lock");

    player = OpenShop(engine);
    Click(graphics, sell);
    test.Expect(EngineTestAccess::Enqueue(engine) &&
                    EngineTestAccess::IsType(engine, EngineTestAccess::Sell()),
                "Sell enters flight with explicit Sell type");
    const std::uint64_t sellID = EngineTestAccess::ID(engine);
    WorldTestAccess::PublishResults(world, {
        {sellID - 1, actor, CommandResultCode::GAMEPLAY_REJECTED}});
    EngineTestAccess::Sync(engine);
    test.Expect(EngineTestAccess::ID(engine) == sellID &&
                    EngineTestAccess::IsType(engine, EngineTestAccess::Sell()),
                "Result for another command cannot reconcile pending Sell");
    TickAndSync(engine);
    test.Expect(player->GetInventory().GetItemAmount(ItemType::COINS) == 99 &&
                    player->GetInventory().GetItemAmount(ItemType::LOG) == 3,
                "Accepted Sell still updates economy on authoritative tick");

    player->GetInventory().RemoveItem(ItemType::COINS,
        player->GetInventory().GetItemAmount(ItemType::COINS));
    const int rejectedBuyLogs = player->GetInventory().GetItemAmount(ItemType::LOG);
    Click(graphics, buy);
    test.Expect(EngineTestAccess::Enqueue(engine),
                "Insufficient-currency Buy enters flight");
    TickAndSync(engine);
    test.Expect(graphics.GetShopPresentationState().IsOpen() &&
                    graphics.GetShopPresentationState().HasRejection() &&
                    player->GetInventory().GetItemAmount(ItemType::COINS) == 0 &&
                    player->GetInventory().GetItemAmount(ItemType::LOG) == rejectedBuyLogs &&
                    !EngineTestAccess::HasPending(engine),
                "Rejected Buy stays open, preserves economy and shows feedback");

    player->GetInventory().RemoveItem(ItemType::LOG,
        player->GetInventory().GetItemAmount(ItemType::LOG));
    Click(graphics, sell);
    test.Expect(EngineTestAccess::Enqueue(engine),
                "Missing-item Sell enters flight");
    TickAndSync(engine);
    test.Expect(graphics.GetShopPresentationState().IsOpen() &&
                    graphics.GetShopPresentationState().HasRejection() &&
                    player->GetInventory().GetItemAmount(ItemType::COINS) == 0 &&
                    player->GetInventory().GetItemAmount(ItemType::LOG) == 0 &&
                    !EngineTestAccess::HasPending(engine),
                "Rejected Sell stays open, preserves economy and shows feedback");

    Click(graphics, close);
    test.Expect(EngineTestAccess::Enqueue(engine),
                "Rejected Close fixture enters flight");
    const std::uint64_t rejectedCloseID = EngineTestAccess::ID(engine);
    WorldTestAccess::PublishResults(world, {
        {rejectedCloseID, actor, CommandResultCode::GAMEPLAY_REJECTED}});
    EngineTestAccess::Sync(engine);
    test.Expect(!graphics.GetShopPresentationState().IsOpen() &&
                    !EngineTestAccess::HasPending(engine) &&
                    !graphics.IsShopCommandInFlight(),
                "Rejected Close dismisses panel and releases lock");

    player = OpenShop(engine);
    Click(graphics, buy);
    test.Expect(EngineTestAccess::Enqueue(engine),
                "Dismissed-session fixture enters flight");
    const std::uint64_t dismissedCommandID = EngineTestAccess::ID(engine);
    GraphicsTestAccess::Dismiss(graphics);
    WorldTestAccess::PublishResults(world, {});
    EngineTestAccess::Sync(engine);
    test.Expect(!EngineTestAccess::HasPending(engine) &&
                    !graphics.IsShopCommandInFlight(),
                "Dismissed originating presentation clears obsolete lock");

    player = OpenShop(engine);
    Click(graphics, buy);
    test.Expect(EngineTestAccess::Enqueue(engine),
                "New command enters flight after obsolete lock clears");
    const std::uint64_t commandAfterDismiss = EngineTestAccess::ID(engine);
    WorldTestAccess::PublishResults(world, {
        {dismissedCommandID, actor, CommandResultCode::GAMEPLAY_REJECTED}});
    EngineTestAccess::Sync(engine);
    test.Expect(EngineTestAccess::ID(engine) == commandAfterDismiss &&
                    graphics.IsShopCommandInFlight(),
                "Stale result cannot unlock a newer command");
    TickAndSync(engine);

    Click(graphics, buy);
    test.Expect(EngineTestAccess::Enqueue(engine),
                "Replacement-session fixture enters flight");
    WorldTestAccess::ReplaceSession(world, actor, 1);
    WorldTestAccess::PublishResults(world, {});
    EngineTestAccess::Sync(engine);
    test.Expect(!EngineTestAccess::HasPending(engine) &&
                    !graphics.IsShopCommandInFlight(),
                "Replacement authoritative session clears obsolete lock");

    player = OpenShop(engine);
    Click(graphics, buy);
    test.Expect(EngineTestAccess::Enqueue(engine),
                "Invalidated-session fixture enters flight");
    world.EnqueueCommand(ShopCloseCommand{
        actor, EngineTestAccess::Session(engine)});
    world.Update();
    EngineTestAccess::Sync(engine);
    test.Expect(!EngineTestAccess::HasPending(engine) &&
                    !graphics.IsShopCommandInFlight() &&
                    !graphics.GetShopPresentationState().IsOpen(),
                "Invalidated authoritative session clears obsolete lock");

    return test.Finish();
}

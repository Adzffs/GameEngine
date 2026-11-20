#include "TestSupport.h"
#include "../src/Core/Engine.h"
#include "../src/Player/Player.h"
#include <variant>

struct GraphicsTestAccess
{
    static void InventoryView(Graphics &g, const Inventory &inventory)
    { g.displayedInventorySlots = inventory.GetSlots(); }
    static bool PendingDeposit(const Graphics &g)
    { return g.pendingBankCommand && std::holds_alternative<BankDepositCommand>(*g.pendingBankCommand); }
    static bool PendingWithdraw(const Graphics &g)
    { return g.pendingBankCommand && std::holds_alternative<BankWithdrawCommand>(*g.pendingBankCommand); }
    static const std::string &Search(const Graphics &g) { return g.bankSearch; }
    static bool Guide(const Graphics &g) { return g.gameGuideVisible; }
};

struct EngineTestAccess
{
    static bool Initialize(Engine &e) { return e.InitializeWorldForRun() && e.graphics.Initialize(); }
    static World &WorldOf(Engine &e) { return e.world; }
    static Graphics &GraphicsOf(Engine &e) { return e.graphics; }
    static int Actor(const Engine &e) { return e.playerID; }
    static bool Enqueue(Engine &e) { return e.EnqueuePendingBankCommand(); }
    static void Refresh(Engine &e)
    {
        e.SynchronizeBankPresentation();
        auto *player = dynamic_cast<Player *>(e.world.GetEntityByID(e.playerID));
        GraphicsTestAccess::InventoryView(e.graphics, player->GetInventory());
    }
    static void Tick(Engine &e) { e.Update(); Refresh(e); }
};

namespace
{
    void Push(Graphics &g, SDL_Event event)
    {
        if (!SDL_PushEvent(&event)) throw std::runtime_error(SDL_GetError());
        bool running = true;
        g.ProcessEvents(running);
    }
    void Key(Graphics &g, SDL_Scancode key)
    { SDL_Event event{}; event.type = SDL_EVENT_KEY_DOWN; event.key.scancode = key; Push(g, event); }
    void Click(Graphics &g, float x, float y)
    {
        SDL_Event event{}; event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        event.button.button = SDL_BUTTON_LEFT; event.button.x = x; event.button.y = y;
        Push(g, event);
    }
}

int main()
{
    TestContext test;
    EngineConfiguration configuration;
    configuration.newPlayerMode = PlayerInitializationMode::FRESH_START;
    Engine engine(configuration);
    if (!EngineTestAccess::Initialize(engine))
    { test.Expect(false, "Fresh bank client initializes with real SDL"); return test.Finish(); }
    auto &world = EngineTestAccess::WorldOf(engine);
    auto &graphics = EngineTestAccess::GraphicsOf(engine);
    auto &player = *dynamic_cast<Player *>(world.GetEntityByID(EngineTestAccess::Actor(engine)));
    const CraftingStation *bank = nullptr;
    for (const auto &station : world.GetStations())
        if (station.GetStationType() == StationType::BANK) { bank = &station; break; }
    if (!bank) { test.Expect(false, "Bank exists in the authored fixture"); return test.Finish(); }
    player.GetPosition().SetPosition(bank->GetX() - 1, bank->GetY());
    world.QueueStationInteraction(player.GetID(), bank->GetID());
    world.Update();
    StationType opened = StationType::NONE;
    test.Expect(world.ConsumeOpenedStation(player.GetID(), opened) && opened == StationType::BANK &&
                    world.GetBankSession(player.GetID()), "Production station interaction opens a bank session");
    graphics.OpenStationMenu(opened);
    EngineTestAccess::Refresh(engine);

    int axeSlot = -1;
    for (int i = 0; i < Inventory::SlotCount; ++i)
        if (player.GetInventory().GetSlots()[i].GetItemType() == ItemType::BRONZE_AXE) axeSlot = i;
    if (axeSlot < 0) { test.Expect(false, "Fresh player has a bronze axe"); return test.Finish(); }
    Click(graphics, 220.0f + 490.0f + (axeSlot % 7) * 46.0f,
        54.0f + 164.0f + (axeSlot / 7) * 50.0f);
    Key(graphics, SDL_SCANCODE_ESCAPE);
    const bool retained = graphics.IsStationMenuOpen() && GraphicsTestAccess::PendingDeposit(graphics);
    test.Expect(retained, "Escape cannot replace a queued deposit with BankClose in the same frame");
    if (!retained) return test.Finish();
    test.Expect(EngineTestAccess::Enqueue(engine) && !EngineTestAccess::Enqueue(engine),
        "The normal engine queue consumes exactly one deposit request");
    test.Expect(player.GetInventory().GetItemAmount(ItemType::BRONZE_AXE) == 1 &&
        player.GetBank().GetItemAmount(ItemType::BRONZE_AXE) == 0,
        "Input does not predictively mutate authoritative inventory or bank");
    EngineTestAccess::Tick(engine);
    test.Expect(player.GetInventory().GetItemAmount(ItemType::BRONZE_AXE) == 0 &&
        player.GetBank().GetItemAmount(ItemType::BRONZE_AXE) == 1 &&
        world.GetCommandProcessingResults().back().resultCode == CommandResultCode::ACCEPTED,
        "The committed deposit moves the axe to the bank");

    Key(graphics, SDL_SCANCODE_H);
    SDL_Event text{}; text.type = SDL_EVENT_TEXT_INPUT; text.text.text = "h"; Push(graphics, text);
    Click(graphics, 262.0f, 218.0f);
    test.Expect(GraphicsTestAccess::Search(graphics) == "h" && !GraphicsTestAccess::Guide(graphics) &&
        !EngineTestAccess::Enqueue(engine), "Bank h search filters the axe without opening help or withdrawing hidden items");
    Key(graphics, SDL_SCANCODE_BACKSPACE);
    Click(graphics, 262.0f, 218.0f);
    test.Expect(GraphicsTestAccess::Search(graphics).empty() && GraphicsTestAccess::PendingWithdraw(graphics),
        "Clearing the filter exposes the real withdrawal hitbox");
    Key(graphics, SDL_SCANCODE_ESCAPE);
    test.Expect(graphics.IsStationMenuOpen() && GraphicsTestAccess::PendingWithdraw(graphics),
        "Escape preserves the queued withdrawal until it reaches the engine");
    test.Expect(EngineTestAccess::Enqueue(engine), "Withdrawal enters the normal command queue");
    EngineTestAccess::Tick(engine);
    test.Expect(player.GetInventory().GetItemAmount(ItemType::BRONZE_AXE) == 1 &&
        player.GetBank().GetItemAmount(ItemType::BRONZE_AXE) == 0 &&
        world.GetCommandProcessingResults().back().resultCode == CommandResultCode::ACCEPTED,
        "The committed withdrawal restores exactly one axe to the fresh player's bag");
    Key(graphics, SDL_SCANCODE_ESCAPE);
    test.Expect(!graphics.IsStationMenuOpen() && world.GetBankSession(player.GetID()) &&
        EngineTestAccess::Enqueue(engine), "After transfers, Escape queues an authoritative bank close");
    EngineTestAccess::Tick(engine);
    test.Expect(!world.GetBankSession(player.GetID()), "The server closes the bank session on its next tick");
    return test.Finish();
}

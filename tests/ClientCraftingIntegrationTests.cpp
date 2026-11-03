#include "TestSupport.h"

#include "../src/Core/Engine.h"
#include "../src/Player/Player.h"

#include <algorithm>

struct GraphicsTestAccess
{
    static void SetDisplayedInventory(
        Graphics &graphics,
        const Inventory &inventory)
    {
        graphics.displayedLogCount =
            inventory.GetItemAmount(ItemType::LOG);
        graphics.displayedPlankCount =
            inventory.GetItemAmount(ItemType::WOODEN_PLANKS);
    }

    static bool HasGridRequest(const Graphics &graphics)
    {
        return graphics.craftingGridRequestPending;
    }

    static bool HasStationClose(const Graphics &graphics)
    {
        return graphics.stationMenuClosePending;
    }

    static bool HasWorldClick(const Graphics &graphics)
    {
        return graphics.clickPending;
    }

    static const std::string &Feedback(const Graphics &graphics)
    {
        return graphics.stationFeedback;
    }
};

struct WorldTestAccess
{
    static void PublishResults(
        World &world,
        std::vector<CommandProcessingResult> results)
    {
        world.publishedCommandProcessingResults =
            std::move(results);
    }

    static std::size_t QueuedCommandCount(const World &world)
    {
        return world.serverCommandQueue.GetCount();
    }
};

struct EngineTestAccess
{
    static bool Initialize(Engine &engine)
    {
        return engine.InitializeWorldForRun();
    }

    static World &WorldOf(Engine &engine)
    {
        return engine.world;
    }

    static Graphics &GraphicsOf(Engine &engine)
    {
        return engine.graphics;
    }

    static int PlayerID(const Engine &engine)
    {
        return engine.playerID;
    }

    static bool EnqueueRecipe(Engine &engine)
    {
        return engine.EnqueuePendingRecipeCommand();
    }

    static void SynchronizeRecipe(Engine &engine)
    {
        engine.SynchronizeRecipeCommand();
    }

    static bool HasPendingRecipe(const Engine &engine)
    {
        return engine.pendingRecipeCommand.has_value();
    }

    static std::uint64_t RecipeCommandID(const Engine &engine)
    {
        return HasPendingRecipe(engine)
            ? engine.pendingRecipeCommand->commandID
            : 0;
    }

    static int RecipeActor(const Engine &engine)
    {
        return HasPendingRecipe(engine)
            ? engine.pendingRecipeCommand->actorEntityID
            : 0;
    }

    static StationType RecipeStation(const Engine &engine)
    {
        return HasPendingRecipe(engine)
            ? engine.pendingRecipeCommand->stationType
            : StationType::NONE;
    }

    static bool IsCraftingGridCommand(const Engine &engine)
    {
        return HasPendingRecipe(engine) &&
               engine.pendingRecipeCommand->type ==
                   Engine::PendingRecipeCommandType::CRAFTING_GRID;
    }

    static bool IsFurnaceCommand(const Engine &engine)
    {
        return HasPendingRecipe(engine) &&
               engine.pendingRecipeCommand->type ==
                   Engine::PendingRecipeCommandType::FURNACE_RECIPE;
    }
};

namespace
{
    bool PushLeftClick(
        Graphics &graphics,
        const SDL_FRect &rectangle)
    {
        if ((SDL_WasInit(SDL_INIT_EVENTS) & SDL_INIT_EVENTS) == 0 &&
            !SDL_InitSubSystem(SDL_INIT_EVENTS))
        {
            return false;
        }

        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = rectangle.x + rectangle.w * 0.5f;
        event.button.y = rectangle.y + rectangle.h * 0.5f;

        if (!SDL_PushEvent(&event))
        {
            return false;
        }

        bool running = true;
        graphics.ProcessEvents(running);
        return running;
    }

    bool PushEscape(Graphics &graphics)
    {
        if ((SDL_WasInit(SDL_INIT_EVENTS) & SDL_INIT_EVENTS) == 0 &&
            !SDL_InitSubSystem(SDL_INIT_EVENTS))
        {
            return false;
        }

        SDL_Event event{};
        event.type = SDL_EVENT_KEY_DOWN;
        event.key.scancode = SDL_SCANCODE_ESCAPE;
        event.key.repeat = false;
        if (!SDL_PushEvent(&event))
        {
            return false;
        }

        bool running = true;
        graphics.ProcessEvents(running);
        return running;
    }

    bool IsInside(
        const SDL_FRect &outer,
        const SDL_FRect &inner)
    {
        return inner.x >= outer.x &&
               inner.y >= outer.y &&
               inner.x + inner.w <= outer.x + outer.w &&
               inner.y + inner.h <= outer.y + outer.h;
    }

    bool Overlaps(
        const SDL_FRect &left,
        const SDL_FRect &right)
    {
        return left.x < right.x + right.w &&
               left.x + left.w > right.x &&
               left.y < right.y + right.h &&
               left.y + left.h > right.y;
    }

    Player *OpenStation(
        Engine &engine,
        int stationX,
        int stationY,
        StationType expectedType)
    {
        World &world = EngineTestAccess::WorldOf(engine);
        Graphics &graphics = EngineTestAccess::GraphicsOf(engine);
        Player *player = dynamic_cast<Player *>(
            world.GetEntityByID(EngineTestAccess::PlayerID(engine)));
        CraftingStation *station =
            world.GetStationAt(stationX, stationY);

        if (player == nullptr || station == nullptr)
        {
            return nullptr;
        }

        player->GetPosition().SetPosition(stationX - 1, stationY);
        world.QueueStationInteraction(
            player->GetID(),
            station->GetID());
        world.Update();

        StationType openedType = StationType::NONE;
        if (!world.ConsumeOpenedStation(
                player->GetID(),
                openedType) ||
            openedType != expectedType)
        {
            return nullptr;
        }

        graphics.OpenStationMenu(openedType);
        GraphicsTestAccess::SetDisplayedInventory(
            graphics,
            player->GetInventory());
        return player;
    }

    bool PlaceLogAndRequestCraft(Graphics &graphics)
    {
        const Graphics::CraftingTableLayout layout =
            graphics.GetCraftingTableLayout();

        return PushLeftClick(
                   graphics,
                   layout.ingredientButtons[0]) &&
               PushLeftClick(
                   graphics,
                   layout.gridSlots[4]) &&
               PushLeftClick(
                   graphics,
                   layout.craftButton);
    }

    std::size_t CountResult(
        const World &world,
        std::uint64_t commandID,
        int actorEntityID)
    {
        return static_cast<std::size_t>(std::count_if(
            world.GetCommandProcessingResults().begin(),
            world.GetCommandProcessingResults().end(),
            [commandID, actorEntityID](
                const CommandProcessingResult &result)
            {
                return result.commandID == commandID &&
                       result.actorEntityID == actorEntityID;
            }));
    }
}

int main()
{
    TestContext test;
    Engine engine;

    test.Expect(
        EngineTestAccess::Initialize(engine),
        "Engine crafting fixture initializes");

    World &world = EngineTestAccess::WorldOf(engine);
    Graphics &graphics = EngineTestAccess::GraphicsOf(engine);
    const int actorEntityID =
        EngineTestAccess::PlayerID(engine);
    Player *player = OpenStation(
        engine,
        17,
        9,
        StationType::CRAFTING_TABLE);

    test.Expect(
        player != nullptr &&
            graphics.IsStationMenuOpen() &&
            graphics.GetOpenStationType() ==
                StationType::CRAFTING_TABLE,
        "Authoritative Crafting Table interaction opens the client modal");

    const Graphics::CraftingTableLayout layout =
        graphics.GetCraftingTableLayout();
    bool layoutInsidePanel =
        IsInside(layout.panelBounds, layout.closeButton) &&
        IsInside(layout.panelBounds, layout.clearButton) &&
        IsInside(layout.panelBounds, layout.craftButton) &&
        IsInside(layout.panelBounds, layout.previewBounds);
    bool gridDisjoint = true;
    for (std::size_t left = 0;
         left < layout.gridSlots.size();
         ++left)
    {
        layoutInsidePanel = layoutInsidePanel &&
            IsInside(layout.panelBounds, layout.gridSlots[left]);
        for (std::size_t right = left + 1;
             right < layout.gridSlots.size();
             ++right)
        {
            gridDisjoint = gridDisjoint &&
                !Overlaps(
                    layout.gridSlots[left],
                    layout.gridSlots[right]);
        }
    }
    test.Expect(
        layoutInsidePanel &&
            gridDisjoint &&
            !Overlaps(layout.clearButton, layout.craftButton) &&
            !Overlaps(layout.craftButton, layout.closeButton),
        "Crafting controls are distinct and remain inside the modal");

    SDL_FRect worldRectangle{16.0f, 16.0f, 8.0f, 8.0f};
    test.Expect(
        PushLeftClick(graphics, worldRectangle) &&
            !GraphicsTestAccess::HasWorldClick(graphics),
        "Crafting modal consumes outside world clicks");

    player->GetInventory().AddItem(ItemType::LOG, 2);
    GraphicsTestAccess::SetDisplayedInventory(
        graphics,
        player->GetInventory());
    test.Expect(
        PlaceLogAndRequestCraft(graphics) &&
            graphics.GetCraftingGrid()[4] == ItemType::LOG &&
            GraphicsTestAccess::HasGridRequest(graphics),
        "Real SDL events select a Log, place it, and request its recipe");

    test.Expect(
        EngineTestAccess::EnqueueRecipe(engine),
        "Engine consumes and enqueues the grid request");
    const std::uint64_t acceptedCommandID =
        EngineTestAccess::RecipeCommandID(engine);
    test.Expect(
        acceptedCommandID != 0 &&
            EngineTestAccess::RecipeActor(engine) == actorEntityID &&
            EngineTestAccess::RecipeStation(engine) ==
                StationType::CRAFTING_TABLE &&
            EngineTestAccess::IsCraftingGridCommand(engine) &&
            graphics.IsRecipeCommandInFlight() &&
            WorldTestAccess::QueuedCommandCount(world) == 1,
        "In-flight lock records the exact command, actor, station and type");

    PushLeftClick(graphics, layout.clearButton);
    PushLeftClick(graphics, layout.closeButton);
    PushLeftClick(graphics, layout.craftButton);
    PushEscape(graphics);
    test.Expect(
        graphics.GetCraftingGrid()[4] == ItemType::LOG &&
            graphics.IsStationMenuOpen() &&
            !GraphicsTestAccess::HasStationClose(graphics) &&
            !GraphicsTestAccess::HasGridRequest(graphics) &&
            !EngineTestAccess::EnqueueRecipe(engine) &&
            EngineTestAccess::RecipeCommandID(engine) ==
                acceptedCommandID &&
            WorldTestAccess::QueuedCommandCount(world) == 1,
        "Rapid Clear, Close, Craft and Escape cannot overwrite the command");

    WorldTestAccess::PublishResults(
        world,
        {{acceptedCommandID + 1,
          actorEntityID,
          CommandResultCode::GAMEPLAY_REJECTED}});
    EngineTestAccess::SynchronizeRecipe(engine);
    test.Expect(
        EngineTestAccess::RecipeCommandID(engine) ==
                acceptedCommandID &&
            graphics.IsRecipeCommandInFlight(),
        "Wrong-ID result cannot reconcile or unlock crafting");

    WorldTestAccess::PublishResults(
        world,
        {{acceptedCommandID,
          actorEntityID + 1,
          CommandResultCode::GAMEPLAY_REJECTED}});
    EngineTestAccess::SynchronizeRecipe(engine);
    test.Expect(
        EngineTestAccess::RecipeCommandID(engine) ==
                acceptedCommandID &&
            graphics.IsRecipeCommandInFlight(),
        "Wrong-actor result cannot reconcile or unlock crafting");

    world.Update();
    test.Expect(
        CountResult(
            world,
            acceptedCommandID,
            actorEntityID) == 1,
        "World processes exactly the first rapid crafting request");
    EngineTestAccess::SynchronizeRecipe(engine);
    test.Expect(
        !EngineTestAccess::HasPendingRecipe(engine) &&
            !graphics.IsRecipeCommandInFlight() &&
            graphics.GetCraftingGrid()[4] == ItemType::NONE &&
            player->GetInventory().GetItemAmount(ItemType::LOG) == 1 &&
            player->GetInventory().GetItemAmount(
                ItemType::WOODEN_PLANKS) == 4 &&
            GraphicsTestAccess::Feedback(graphics) ==
                "Crafting started",
        "Accepted result unlocks once and redraws from authoritative inventory");

    WorldTestAccess::PublishResults(
        world,
        {{acceptedCommandID,
          actorEntityID,
          CommandResultCode::GAMEPLAY_REJECTED}});
    EngineTestAccess::SynchronizeRecipe(engine);
    test.Expect(
        !EngineTestAccess::HasPendingRecipe(engine) &&
            GraphicsTestAccess::Feedback(graphics) ==
                "Crafting started",
        "Duplicate stale result cannot reconcile a consumed command");

    GraphicsTestAccess::SetDisplayedInventory(
        graphics,
        player->GetInventory());
    test.Expect(
        PlaceLogAndRequestCraft(graphics),
        "Rejected transaction fixture creates a visible valid request");
    player->GetInventory().RemoveItem(
        ItemType::LOG,
        player->GetInventory().GetItemAmount(ItemType::LOG));
    test.Expect(
        EngineTestAccess::EnqueueRecipe(engine),
        "Request is queued before server inventory validation");
    const std::uint64_t rejectedCommandID =
        EngineTestAccess::RecipeCommandID(engine);
    world.Update();
    test.Expect(
        CountResult(
            world,
            rejectedCommandID,
            actorEntityID) == 1,
        "Rejected crafting request publishes one real World result");
    EngineTestAccess::SynchronizeRecipe(engine);
    test.Expect(
        graphics.IsStationMenuOpen() &&
            !graphics.IsRecipeCommandInFlight() &&
            !EngineTestAccess::HasPendingRecipe(engine) &&
            graphics.GetCraftingGrid()[4] == ItemType::LOG &&
            player->GetInventory().GetItemAmount(ItemType::LOG) == 0 &&
            player->GetInventory().GetItemAmount(
                ItemType::WOODEN_PLANKS) == 4 &&
            GraphicsTestAccess::Feedback(graphics) ==
                "Crafting rejected by server",
        "Rejected result keeps the grid, panel and economy safely unchanged");

    player->GetInventory().AddItem(ItemType::LOG, 1);
    test.Expect(
        PushLeftClick(graphics, layout.craftButton) &&
            EngineTestAccess::EnqueueRecipe(engine),
        "A new command can enter flight after rejection");
    const std::uint64_t invalidatedCommandID =
        EngineTestAccess::RecipeCommandID(engine);
    world.CloseStationInteraction(actorEntityID);
    graphics.OpenStationMenu(StationType::NONE);
    WorldTestAccess::PublishResults(world, {});
    EngineTestAccess::SynchronizeRecipe(engine);
    test.Expect(
        !EngineTestAccess::HasPendingRecipe(engine) &&
            !graphics.IsRecipeCommandInFlight() &&
            !graphics.IsStationMenuOpen(),
        "Invalidated originating station clears its obsolete client lock");
    world.Update();
    test.Expect(
        CountResult(
            world,
            invalidatedCommandID,
            actorEntityID) == 1 &&
            player->GetInventory().GetItemAmount(ItemType::LOG) == 1,
        "Already submitted invalidated request is rejected without mutation");

    player = OpenStation(
        engine,
        14,
        9,
        StationType::FURNACE);
    const Graphics::FurnaceMenuLayout furnaceLayout =
        graphics.GetFurnaceMenuLayout();
    test.Expect(
        player != nullptr &&
            PushLeftClick(
                graphics,
                furnaceLayout.recipeButtons[0]) &&
            EngineTestAccess::EnqueueRecipe(engine) &&
            EngineTestAccess::IsFurnaceCommand(engine),
        "Furnace recipe requests use the same one-command lock");
    const std::uint64_t furnaceCommandID =
        EngineTestAccess::RecipeCommandID(engine);
    PushLeftClick(graphics, furnaceLayout.closeButton);
    test.Expect(
        graphics.IsStationMenuOpen() &&
            !GraphicsTestAccess::HasStationClose(graphics) &&
            !EngineTestAccess::EnqueueRecipe(engine) &&
            EngineTestAccess::RecipeCommandID(engine) ==
                furnaceCommandID,
        "Furnace Close cannot overwrite an in-flight recipe command");
    world.Update();
    EngineTestAccess::SynchronizeRecipe(engine);
    test.Expect(
        !EngineTestAccess::HasPendingRecipe(engine) &&
            !graphics.IsRecipeCommandInFlight() &&
            graphics.IsStationMenuOpen() &&
            GraphicsTestAccess::Feedback(graphics) ==
                "Crafting rejected by server",
        "Rejected furnace request unlocks safely without closing its panel");

    test.Expect(
        PushEscape(graphics) &&
            !graphics.IsStationMenuOpen() &&
            graphics.ConsumeStationMenuClose() &&
            !graphics.ConsumeStationMenuClose(),
        "Escape queues exactly one authoritative station close after unlock");

    return test.Finish();
}

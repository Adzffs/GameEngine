#include "Engine.h"
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include "Core/Logger.h"
#include "../Command/ServerCommand.h"
#include "../Equipment/EquipmentSlotType.h"
#include "../Inventory/ItemType.h"
#include "../Player/Player.h"
#include "../Skills/SkillType.h"
#include "../Recipe/RecipeType.h"
#include "EngineClickRouting.h"

namespace
{
    int FindInventorySlot(
        const Inventory &inventory,
        ItemType itemType)
    {
        const auto &slots = inventory.GetSlots();

        for (int slotIndex = 0;
             slotIndex < static_cast<int>(slots.size());
             ++slotIndex)
        {
            if (!slots[slotIndex].IsEmpty() &&
                slots[slotIndex].GetItemType() == itemType)
            {
                return slotIndex;
            }
        }

        return -1;
    }
}

Engine::Engine()
    : Engine(EngineConfiguration{})
{
}

Engine::Engine(EngineConfiguration configuration)
    : playerID(-1)
{
    if (configuration.developmentPlayerSavePath.has_value())
    {
        playerPersistenceLifecycle =
            std::make_unique<WorldPlayerPersistenceLifecycle>(
                *configuration.developmentPlayerSavePath);
    }

}

bool Engine::InitializeWorldForRun()
{
    if (initializationSucceeded) return true;

    bool createdNew = true;
    if (playerPersistenceLifecycle != nullptr)
    {
        WorldPlayerPersistenceStartupResult result =
            playerPersistenceLifecycle->Start(world);
        if (!result.IsSuccess())
        {
            Logger::Error(
                "Development Player persistence startup failed"
                " | path=" + playerPersistenceLifecycle->GetSavePath().string() +
                " | lifecycleIssues=" + std::to_string(result.GetIssues().size()) +
                " | worldIssues=" +
                std::to_string(result.GetWorldLoadResult().GetIssues().size()));
            return false;
        }
        playerID = result.GetPlayerEntityID();
        createdNew = result.WasCreatedNew();
        Logger::Info(
            std::string("Development Player persistence startup ") +
            (result.WasLoadedFromFile() ? "loaded existing Player" : "created new Player") +
            " | runtimePlayerID=" + std::to_string(playerID) +
            " | path=" + playerPersistenceLifecycle->GetSavePath().string());
    }
    else
    {
        playerID = world.CreatePlayer();
    }

    if (playerID <= 0) return false;
    if (createdNew) AddDevelopmentEquipment();
    initializationSucceeded = true;
    return true;
}

void Engine::AddDevelopmentEquipment()
{

    Player *player =
        dynamic_cast<Player *>(
            world.GetEntityByID(playerID));

    if (player != nullptr &&
        player->GetInventory().AddItem(
            ItemType::DEVELOPER_GODSWORD,
            1))
    {
        int weaponSlotIndex = FindInventorySlot(
            player->GetInventory(),
            ItemType::DEVELOPER_GODSWORD);

        if (weaponSlotIndex >= 0)
        {
            world.EnqueueCommand(
                UseInventoryItemCommand{
                    playerID,
                    weaponSlotIndex});
        }
    }
}

bool Engine::Run()
{
    if (!graphics.Initialize()) return false;
    if (!InitializeWorldForRun()) return false;

    Logger::Info("Server Started");

    while (running)
    {
        SynchronizeDialoguePresentation();
        SynchronizeShopPresentation();
        graphics.ProcessEvents(running);

        if (!running)
        {
            break;
        }

        EnqueuePendingDialogueCommand();
        EnqueuePendingShopCommand();

        RecipeType requestedRecipe =
            RecipeType::NONE;

        if (graphics.ConsumeRecipeRequest(
                requestedRecipe))
        {
            world.EnqueueCommand(
                StartRecipeCommand{
                    playerID,
                    requestedRecipe});
        }

        int clickedInventorySlot = -1;

        if (graphics.ConsumeInventorySlotClick(
                clickedInventorySlot))
        {
            world.EnqueueCommand(
                UseInventoryItemCommand{
                    playerID,
                    clickedInventorySlot});
        }

        if (graphics.ConsumeWeaponSlotClick())
        {
            world.EnqueueCommand(
                UnequipItemCommand{
                    playerID,
                    EquipmentSlotType::WEAPON});
        }

        int clickedTileX;
        int clickedTileY;
        int clickedMouseX;
        int clickedMouseY;

        if (graphics.ConsumeClickedTile(
                clickedTileX,
                clickedTileY,
                clickedMouseX,
                clickedMouseY))
        {
            EngineClickRouting::HandleWorldClick(
                graphics,
                world,
                playerID,
                clickedTileX,
                clickedTileY,
                clickedMouseX,
                clickedMouseY);
        }

        if (graphics.ConsumeStationMenuClose())
        {
            world.EnqueueCommand(
                CloseStationCommand{playerID});
        }

        const Clock::TimePoint now =
            Clock::ClockType::now();

        const int dueTickCount =
            clock.GetDueTickCount(
                now,
                MaxCatchUpTicks);

        if (dueTickCount > 0)
        {
            for (int index = 0;
                 index < dueTickCount;
                 ++index)
            {
                clock.AdvanceTickDeadline();
                Update();
            }

            const Clock::TimePoint afterCatchUp =
                Clock::ClockType::now();

            if (clock.IsTickDue(afterCatchUp))
            {
                const long long backlogMs =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        clock.GetBacklogDuration(afterCatchUp))
                        .count();

                Logger::Warn(
                    "Tick backlog exceeded catch-up limit; "
                    "clock resynchronised"
                    " | tick=" +
                    std::to_string(
                        world.GetCurrentTick()) +
                    " | processed=" +
                    std::to_string(dueTickCount) +
                    " | backlogMs=" +
                    std::to_string(backlogMs));

                clock.Resynchronise(afterCatchUp);
            }
        }
        else
        {
            const Clock::TimePoint sleepDeadline =
                clock.GetNextTickDeadline();

            const Clock::TimePoint sleepStart =
                Clock::ClockType::now();

            if (sleepDeadline > sleepStart)
            {
                constexpr auto MaxIdleSleep =
                    std::chrono::milliseconds(5);

                const auto remaining =
                    sleepDeadline - sleepStart;

                if (remaining > MaxIdleSleep)
                {
                    std::this_thread::sleep_for(
                        MaxIdleSleep);
                }
                else
                {
                    std::this_thread::sleep_until(
                        sleepDeadline);
                }
            }
        }

        SynchronizeDialoguePresentation();
        SynchronizeShopPresentation();

        Player *player =
            dynamic_cast<Player *>(
                world.GetEntityByID(playerID));

        StationType openedStationType =
            StationType::NONE;

        if (world.ConsumeOpenedStation(
                playerID,
                openedStationType))
        {
            graphics.OpenStationMenu(
                openedStationType);
        }

        if (player != nullptr)
        {
            const Skill &woodcutting =
                player->GetSkills().GetSkill(
                    SkillType::WOODCUTTING);

            const Skill &mining =
                player->GetSkills().GetSkill(
                    SkillType::MINING);

            graphics.Render(
                world.GetMap(),
                world.GetEntities(),
                world.GetResources(),
                world.GetStations(),
                world.GetMeleeCombatFeedbacks(),
                player->GetPosition().GetX(),
                player->GetPosition().GetY(),
                player->GetInventory(),
                player->GetEquipment(),
                *player,
                world.GetActionForEntity(
                    playerID));
        }
    }

    const bool finalized = FinalizeWorldAfterRun();
    Logger::Info("Server stopped");
    return finalized;
}

void Engine::SynchronizeDialoguePresentation()
{
    graphics.SynchronizeDialogue(
        playerID,
        world.GetNpcTalkEvents(),
        world.GetActiveDialogueSession(playerID));
}

bool Engine::EnqueuePendingDialogueCommand()
{
    std::optional<ServerCommandData> command = graphics.ConsumeDialogueCommand();
    if (!command.has_value()) return false;
    world.EnqueueCommand(std::move(*command));
    return true;
}

void Engine::SynchronizeShopPresentation()
{
    for (const auto &result : world.GetCommandProcessingResults()) {
        if (pendingShopCommandID == 0 || result.commandID != pendingShopCommandID || result.actorEntityID != playerID) continue;
        const bool close = pendingShopCommandType == PendingShopCommandType::CLOSE;
        graphics.ReconcileShopCommandResult(result.resultCode, close);
        pendingShopCommandID = 0;
        pendingShopCommandType = PendingShopCommandType::NONE;
    }
    graphics.SynchronizeShop(playerID, world.GetShopOpenedEvents(), world.GetActiveShopSession(playerID));
}

bool Engine::EnqueuePendingShopCommand()
{
    std::optional<ServerCommandData> command = graphics.ConsumeShopCommand();
    if (!command) return false;
    pendingShopCommandType = std::visit([](const auto &data) {
        using T = std::decay_t<decltype(data)>;
        if constexpr (std::is_same_v<T, ShopBuyCommand>) return PendingShopCommandType::BUY;
        if constexpr (std::is_same_v<T, ShopSellCommand>) return PendingShopCommandType::SELL;
        if constexpr (std::is_same_v<T, ShopCloseCommand>) return PendingShopCommandType::CLOSE;
        return PendingShopCommandType::NONE;
    }, *command);
    pendingShopCommandID = world.EnqueueCommand(std::move(*command));
    return true;
}

bool Engine::FinalizeWorldAfterRun()
{
    if (!initializationSucceeded) return true;
    if (playerPersistenceLifecycle != nullptr)
    {
        WorldPlayerPersistenceOperationResult result =
            playerPersistenceLifecycle->Shutdown(world);
        if (!result.IsSuccess())
        {
            Logger::Error(
                "Development Player persistence shutdown failed"
                " | runtimePlayerID=" + std::to_string(result.GetPlayerEntityID()) +
                " | path=" + playerPersistenceLifecycle->GetSavePath().string() +
                " | lifecycleIssues=" + std::to_string(result.GetIssues().size()) +
                " | worldIssues=" +
                std::to_string(result.GetWorldSaveResult().GetIssues().size()));
            return false;
        }
        if (result.DidSave())
        {
            Logger::Info(
                "Development Player persistence shutdown saved Player"
                " | runtimePlayerID=" + std::to_string(result.GetPlayerEntityID()) +
                " | path=" + playerPersistenceLifecycle->GetSavePath().string());
        }
    }
    return true;
}

void Engine::Update()
{
    const Clock::TimePoint updateStart =
        Clock::ClockType::now();

    world.Update();

    const Clock::TimePoint updateEnd =
        Clock::ClockType::now();

    const auto updateDuration =
        std::chrono::duration_cast<std::chrono::microseconds>(
            updateEnd - updateStart);

    const bool overrun =
        RecordTickDuration(
            tickPerformanceStats,
            updateDuration,
            std::chrono::duration_cast<std::chrono::milliseconds>(
                clock.GetTickInterval()));

    if (overrun)
    {
        const long long durationMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                updateDuration)
                .count();

        const long long budgetMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                clock.GetTickInterval())
                .count();

        Logger::Warn(
            "Tick " +
            std::to_string(
                world.GetCurrentTick()) +
            " exceeded budget: " +
            std::to_string(durationMs) +
            " ms / " +
            std::to_string(budgetMs) +
            " ms");
    }
}

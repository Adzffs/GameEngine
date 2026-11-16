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
#include "../Recipe/CraftingInteractionModel.h"
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
    graphics.ShowGameGuide();
    if (!InitializeWorldForRun()) return false;

    Logger::Info("Server Started");

    while (running)
    {
        SynchronizeDialoguePresentation();
        SynchronizeShopPresentation();
        SynchronizeBankPresentation();
        SynchronizeRecipeCommand();
        graphics.SynchronizeCraftingSession(
            world.GetCraftingSession(playerID));
        SynchronizeCraftingInteraction();
        graphics.ProcessEvents(running);

        if (!running)
        {
            break;
        }

        EnqueuePendingDialogueCommand();
        EnqueuePendingShopCommand();
        EnqueuePendingBankCommand();
        EnqueuePendingRecipeCommand();
        EnqueuePendingCraftingInteraction();

        int clickedInventorySlot = -1;

        if (graphics.ConsumeInventorySlotClick(
                clickedInventorySlot))
        {
            world.EnqueueCommand(
                UseInventoryItemCommand{
                    playerID,
                    clickedInventorySlot});
        }

        int dropAmount = 0;
        if (graphics.ConsumeInventoryDropRequest(
                clickedInventorySlot, dropAmount))
        {
            world.EnqueueCommand(DropInventoryItemCommand{
                playerID, clickedInventorySlot, dropAmount});
        }

        if (graphics.ConsumeTutorialSkipRequest())
        {
            world.EnqueueCommand(SkipTutorialCommand{playerID});
        }

        if (const std::optional<CombatStyle> style =
                graphics.ConsumeCombatStyleRequest();
            style.has_value())
        {
            world.EnqueueCommand(SetCombatStyleCommand{playerID, *style});
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

        if (graphics.ConsumeWalkTile(clickedTileX, clickedTileY))
        {
            world.EnqueueCommand(MoveCommand{
                playerID, Position(clickedTileX, clickedTileY)});
        }

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
        SynchronizeBankPresentation();
        SynchronizeRecipeCommand();

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
            const World::PlayerDeathRecovery *deathRecovery =
                world.GetPlayerDeathRecovery(playerID);
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
                world.GetGroundItems(),
                world.GetMeleeCombatFeedbacks(),
                player->GetPosition().GetX(),
                player->GetPosition().GetY(),
                player->GetInventory(),
                player->GetEquipment(),
                *player,
                world.GetActionForEntity(
                    playerID),
                world.GetMeleeTargetEntityID(playerID).value_or(-1),
                deathRecovery == nullptr
                    ? -1
                    : std::max(0, deathRecovery->respawnTick -
                        world.GetCurrentTick()),
                deathRecovery == nullptr
                    ? -1
                    : deathRecovery->killerEntityID,
                deathRecovery == nullptr ? 0 : deathRecovery->deathX,
                deathRecovery == nullptr ? 0 : deathRecovery->deathY);
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
        if (!pendingShopCommand.has_value() ||
            result.commandID != pendingShopCommand->commandID ||
            result.actorEntityID != pendingShopCommand->actorEntityID) continue;
        const bool close = pendingShopCommand->type == PendingShopCommandType::CLOSE;
        graphics.ReconcileShopCommandResult(result.resultCode, close);
        ClearPendingShopCommand();
        break;
    }
    graphics.SynchronizeShop(playerID, world.GetShopOpenedEvents(), world.GetActiveShopSession(playerID));

    if (pendingShopCommand.has_value())
    {
        const ActiveShopSession *active =
            world.GetActiveShopSession(pendingShopCommand->actorEntityID);
        const ShopOpenedEvent *displayed =
            graphics.GetShopPresentationState().GetEvent();
        const bool originatingSessionStillActive =
            active != nullptr && displayed != nullptr &&
            active->actorEntityID == pendingShopCommand->actorEntityID &&
            active->sessionId == pendingShopCommand->sessionID &&
            displayed->actorEntityID == pendingShopCommand->actorEntityID &&
            displayed->sessionId == pendingShopCommand->sessionID;
        if (!originatingSessionStillActive)
        {
            ClearPendingShopCommand();
        }
    }
}

void Engine::SynchronizeBankPresentation()
{
    Player *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
    if (player == nullptr) return;
    graphics.SynchronizeBank(
        playerID, world.GetBankSession(playerID), player->GetBank());
}

bool Engine::EnqueuePendingBankCommand()
{
    std::optional<ServerCommandData> command = graphics.ConsumeBankCommand();
    if (!command.has_value()) return false;
    world.EnqueueCommand(std::move(*command));
    return true;
}

bool Engine::EnqueuePendingShopCommand()
{
    if (pendingShopCommand.has_value()) return false;
    std::optional<ServerCommandData> command = graphics.ConsumeShopCommand();
    if (!command) return false;
    const PendingShopCommandType commandType = std::visit([](const auto &data) {
        using T = std::decay_t<decltype(data)>;
        if constexpr (std::is_same_v<T, ShopBuyCommand>) return PendingShopCommandType::BUY;
        if constexpr (std::is_same_v<T, ShopSellCommand>) return PendingShopCommandType::SELL;
        if constexpr (std::is_same_v<T, ShopCloseCommand>) return PendingShopCommandType::CLOSE;
        return PendingShopCommandType::NONE;
    }, *command);
    const auto [actorEntityID, sessionID] = std::visit([](const auto &data) {
        using T = std::decay_t<decltype(data)>;
        if constexpr (std::is_same_v<T, ShopBuyCommand> ||
                      std::is_same_v<T, ShopSellCommand>)
            return std::pair{data.actorEntityID, data.shopSessionId};
        if constexpr (std::is_same_v<T, ShopCloseCommand>)
            return std::pair{data.actorEntityID, data.shopSessionId};
        return std::pair{0, InvalidShopSessionId};
    }, *command);
    if (commandType == PendingShopCommandType::NONE ||
        actorEntityID <= 0 || sessionID == InvalidShopSessionId)
    {
        return false;
    }
    pendingShopCommand = PendingShopCommand{
        world.EnqueueCommand(std::move(*command)),
        actorEntityID,
        sessionID,
        commandType};
    graphics.SetShopCommandInFlight(true);
    return true;
}

void Engine::ClearPendingShopCommand()
{
    pendingShopCommand.reset();
    graphics.SetShopCommandInFlight(false);
}

void Engine::SynchronizeRecipeCommand()
{
    for (const CommandProcessingResult &result :
         world.GetCommandProcessingResults())
    {
        if (!pendingRecipeCommand.has_value() ||
            result.commandID !=
                pendingRecipeCommand->commandID ||
            result.actorEntityID !=
                pendingRecipeCommand->actorEntityID)
        {
            continue;
        }

        graphics.ReconcileRecipeCommandResult(
            result.resultCode);
        ClearPendingRecipeCommand();
        break;
    }

    if (pendingRecipeCommand.has_value() &&
        (!graphics.IsStationMenuOpen() ||
         graphics.GetOpenStationType() !=
             pendingRecipeCommand->stationType))
    {
        ClearPendingRecipeCommand();
    }
}

bool Engine::EnqueuePendingRecipeCommand()
{
    if (pendingRecipeCommand.has_value() || pendingCraftingInteraction.has_value() ||
        !graphics.IsStationMenuOpen())
    {
        return false;
    }

    std::optional<ServerCommandData> command;
    PendingRecipeCommandType commandType =
        PendingRecipeCommandType::NONE;

    CraftingRecipeMatcher::Grid grid{};
    RecipeType recipeType = RecipeType::NONE;

    if (graphics.ConsumeCraftingGridRequest(grid))
    {
        Player *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
        const CraftingSessionSnapshot *session = world.GetCraftingSession(playerID);
        if (player != nullptr && session != nullptr)
        {
            CraftingInteractionModel model(player->GetInventory(), *session);
            bool moved = false;
            for (int index = 0; index < CraftingInteractionModel::CraftingSlotCount; ++index)
                moved = model.TryShiftTransfer(Inventory::SlotCount + index) || moved;
            if (moved)
            {
                const ReplaceCraftingInteractionStateCommand staged = model.BuildCommand();
                pendingCraftingInteraction = PendingCraftingInteraction{
                    world.EnqueueCommand(staged), staged.actorEntityID,
                    staged.sessionId, staged.expectedRevision, grid};
                graphics.SetCraftingInteractionInFlight(true);
                graphics.SetRecipeCommandInFlight(true);
                return true;
            }
        }
        command = CraftRecipeCommand{playerID, grid};
        commandType =
            PendingRecipeCommandType::CRAFTING_GRID;
    }
    else if (graphics.ConsumeRecipeRequest(recipeType))
    {
        command = StartRecipeCommand{playerID, recipeType};
        commandType =
            PendingRecipeCommandType::FURNACE_RECIPE;
    }
    else
    {
        return false;
    }

    pendingRecipeCommand = PendingRecipeCommand{
        world.EnqueueCommand(std::move(*command)),
        playerID,
        graphics.GetOpenStationType(),
        commandType};
    graphics.SetRecipeCommandInFlight(true);
    return true;
}

bool Engine::EnqueuePendingCraftingInteraction()
{
    if (pendingCraftingInteraction.has_value()) return false;
    std::optional<Graphics::CraftingInteractionRequest> request =
        graphics.ConsumeCraftingInteractionRequest();
    if (!request.has_value()) return false;
    Player *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
    const CraftingSessionSnapshot *session = world.GetCraftingSession(playerID);
    if (player == nullptr || session == nullptr) return false;

    CraftingInteractionModel model(player->GetInventory(), *session);
    int slotIndex = request->unifiedSlotIndex;
    if (slotIndex < 0 && request->inventoryItemType != ItemType::NONE)
        slotIndex = FindInventorySlot(player->GetInventory(), request->inventoryItemType);
    if (slotIndex < 0 && !request->clearCrafting) return false;

    bool changed = false;
    if (request->clearCrafting)
    {
        for (int index = 0; index < CraftingInteractionModel::CraftingSlotCount; ++index)
            changed = model.TryShiftTransfer(Inventory::SlotCount + index) || changed;
    }
    else if (!request->dragSlotIndices.empty())
        changed = request->rightClick
            ? model.TryRightDrag(request->dragSlotIndices)
            : model.TryLeftDrag(request->dragSlotIndices);
    else if (request->collectMatching)
    {
        if (model.GetCursor().IsEmpty())
            changed = model.TryLeftClick(slotIndex);
        changed = model.TryCollectMatching() || changed;
    }
    else if (request->shiftHeld)
        changed = model.TryShiftTransfer(slotIndex);
    else
        changed = request->rightClick
            ? model.TryRightClick(slotIndex)
            : model.TryLeftClick(slotIndex);
    if (!changed) return false;
    const ReplaceCraftingInteractionStateCommand command = model.BuildCommand();
    pendingCraftingInteraction = PendingCraftingInteraction{
        world.EnqueueCommand(command), command.actorEntityID,
        command.sessionId, command.expectedRevision, std::nullopt};
    graphics.SetCraftingInteractionInFlight(true);
    return true;
}

void Engine::SynchronizeCraftingInteraction()
{
    if (!pendingCraftingInteraction.has_value()) return;
    for (const CommandProcessingResult &result : world.GetCommandProcessingResults())
    {
        if (result.commandID != pendingCraftingInteraction->commandID ||
            result.actorEntityID != pendingCraftingInteraction->actorEntityID) continue;
        graphics.ReconcileCraftingInteractionResult(result.resultCode);
        if (pendingCraftingInteraction->deferredCraftingGrid.has_value())
        {
            if (result.resultCode == CommandResultCode::ACCEPTED)
            {
                const CraftingRecipeMatcher::Grid grid =
                    *pendingCraftingInteraction->deferredCraftingGrid;
                pendingRecipeCommand = PendingRecipeCommand{
                    world.EnqueueCommand(CraftRecipeCommand{playerID, grid}),
                    playerID, StationType::CRAFTING_TABLE,
                    PendingRecipeCommandType::CRAFTING_GRID};
            }
            else
                graphics.SetRecipeCommandInFlight(false);
        }
        pendingCraftingInteraction.reset();
        graphics.SetCraftingInteractionInFlight(false);
        return;
    }
    const CraftingSessionSnapshot *session =
        world.GetCraftingSession(pendingCraftingInteraction->actorEntityID);
    if (session == nullptr ||
        session->sessionId != pendingCraftingInteraction->sessionID)
    {
        if (pendingCraftingInteraction->deferredCraftingGrid.has_value())
            graphics.SetRecipeCommandInFlight(false);
        pendingCraftingInteraction.reset();
        graphics.SetCraftingInteractionInFlight(false);
    }
}

void Engine::ClearPendingRecipeCommand()
{
    pendingRecipeCommand.reset();
    graphics.SetRecipeCommandInFlight(false);
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

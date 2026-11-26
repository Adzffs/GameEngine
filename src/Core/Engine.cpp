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
#include "../Item/ItemDatabase.h"
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
    : world(configuration.starterWorldSnapshot), playerID(-1), newPlayerMode(configuration.newPlayerMode),
      characterPlaytest(configuration.characterPlaytest)
{
    // The old area's fixed pixel labels do not describe an imported map.
    graphics.SetCompiledMapLabelsVisible(!configuration.starterWorldSnapshot.has_value());
    if (configuration.developmentPlayerSavePath.has_value())
    {
        playerPersistenceLifecycle =
            std::make_unique<WorldPlayerPersistenceLifecycle>(
                *configuration.developmentPlayerSavePath, newPlayerMode);
    }

}

bool Engine::InitializeWorldForRun(Clock::TimePoint now)
{
    if (initializationSucceeded) return true;

    bool createdNew = true;
    if (playerPersistenceLifecycle != nullptr)
    {
        WorldPlayerPersistenceStartupResult result =
            playerPersistenceLifecycle->Start(world, now);
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
        playerID = world.CreatePlayer(newPlayerMode);
    }

    if (playerID <= 0) return false;
    if (createdNew && newPlayerMode == PlayerInitializationMode::DEVELOPMENT_DEFAULTS)
        AddDevelopmentEquipment();
    if (characterPlaytest && !playerPersistenceLifecycle && newPlayerMode == PlayerInitializationMode::FRESH_START)
    {
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
        if (!player || !player->GetInventory().AddItem(ItemType::WOODEN_SHIELD, 1))
            return false;
    }
    Logger::Info(std::string("Player start: ") +
        (!createdNew ? "saved progress restored" :
         newPlayerMode == PlayerInitializationMode::FRESH_START ? "fresh player" : "development player"));
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
    if (characterPlaytest) graphics.EnableCharacterPlaytest();
    else graphics.ShowGameGuide();
    if (!InitializeWorldForRun()) return false;
    audioFeedback.Reset(playerID);
    audio.Initialize(); // Optional presentation: device failure never blocks play.
    audio.ApplySettings(graphics.GetAudioSettings());

    Logger::Info("Server Started");

    while (true)
    {
        if (!running)
        {
            const bool finalized = FinalizeWorldAfterRun();
            if (finalized || graphics.ConfirmExitWithoutSaving())
            {
                Logger::Info("Server stopped");
                return finalized;
            }
            running = true;
        }
        SynchronizeDialoguePresentation();
        SynchronizeShopPresentation();
        SynchronizeBankPresentation();
        SynchronizeRecipeCommand();
        graphics.SynchronizeCraftingSession(
            world.GetCraftingSession(playerID));
        SynchronizeCraftingInteraction();
        graphics.ProcessEvents(running, &world);
        audio.ApplySettings(graphics.GetAudioSettings());

        if (!running)
        {
            continue;
        }

        EnqueuePendingDialogueCommand();
        EnqueuePendingShopCommand();
        EnqueuePendingBankCommand();
        EnqueuePendingRecipeCommand();
        EnqueuePendingCraftingInteraction();

        if (graphics.ConsumeRunToggle())
        {
            Player *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
            if (player != nullptr) player->ToggleRunning();
        }

        int clickedInventorySlot = -1;

        if (graphics.ConsumeInventorySlotClick(
                clickedInventorySlot))
        {
            const auto commandID = world.EnqueueCommand(
                UseInventoryItemCommand{
                    playerID,
                    clickedInventorySlot});
            const Player *actor = dynamic_cast<Player *>(world.GetEntityByID(playerID));
            if (actor && clickedInventorySlot >= 0 &&
                clickedInventorySlot < static_cast<int>(actor->GetInventory().GetSlots().size()))
            {
                const auto &definition = ItemDatabase::Get(
                    actor->GetInventory().GetSlots()[clickedInventorySlot].GetItemType());
                if (definition.IsEquippable())
                    audioFeedback.TrackCommand(commandID, AudioCue::EQUIP);
                else if (definition.IsFood())
                    audioFeedback.TrackCommand(commandID, AudioCue::EAT);
            }
        }

        int dropAmount = 0;
        if (graphics.ConsumeInventoryDropRequest(
                clickedInventorySlot, dropAmount))
        {
            audioFeedback.TrackCommand(world.EnqueueCommand(DropInventoryItemCommand{
                playerID, clickedInventorySlot, dropAmount}), AudioCue::INVENTORY);
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

        if (const auto slot = graphics.ConsumeEquipmentSlotClick())
        {
            audioFeedback.TrackCommand(world.EnqueueCommand(
                UnequipItemCommand{
                    playerID,
                    *slot}), AudioCue::EQUIP);
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
            const PlayerRecoveryPile *recoveryPile =
                world.GetPlayerRecoveryPile(playerID);
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
                deathRecovery == nullptr ? 0 : deathRecovery->deathY,
                recoveryPile == nullptr
                    ? -1
                    : recoveryPile->position.GetX(),
                recoveryPile == nullptr
                    ? -1
                    : recoveryPile->position.GetY(),
                &world.GetMeleeAttackPresentations());
            SkillType contactSkill = SkillType::NONE;
            if (graphics.ConsumeGatheringContact(contactSkill))
                audio.Play(contactSkill == SkillType::MINING
                    ? AudioCue::STONE_IMPACT : AudioCue::WOOD_IMPACT);
            if (const auto weapon = graphics.ConsumeMeleeSwing())
                audio.Play(*weapon == ItemType::DEVELOPER_GODSWORD
                    ? AudioCue::HEAVY_SWING : AudioCue::SWING);
        }
    }
}

void Engine::SynchronizeDialoguePresentation()
{
    // Results are consumed after every committed tick, including catch-up ticks.
    for (const auto &result : world.GetCommandProcessingResults())
        if (pendingDialogueCommand &&
            result.commandID == pendingDialogueCommand->commandID &&
            result.actorEntityID == pendingDialogueCommand->actorEntityID)
        {
            pendingDialogueCommand.reset();
            graphics.SetDialogueCommandInFlight(false);
            break;
        }
    graphics.SynchronizeDialogue(
        playerID, world.GetNpcTalkEvents(), world.GetActiveDialogueSession(playerID));
    const auto *displayed = graphics.GetDialoguePresentationState().GetEvent();
    const auto *active = world.GetActiveDialogueSession(playerID);
    if (pendingDialogueCommand &&
        (!displayed || !active ||
         displayed->sessionId != pendingDialogueCommand->sessionID ||
         displayed->nodeId != pendingDialogueCommand->nodeID ||
         active->sessionId != pendingDialogueCommand->sessionID ||
         active->currentNodeId != pendingDialogueCommand->nodeID))
    {
        pendingDialogueCommand.reset();
        graphics.SetDialogueCommandInFlight(false);
    }
}

bool Engine::EnqueuePendingDialogueCommand()
{
    if (pendingDialogueCommand) return false;
    auto command = graphics.ConsumeDialogueCommand();
    const auto *displayed = graphics.GetDialoguePresentationState().GetEvent();
    if (!command || !displayed) return false;
    pendingDialogueCommand = PendingDialogueCommand{
        world.EnqueueCommand(std::move(*command)), displayed->actorEntityID,
        displayed->sessionId, displayed->nodeId};
    graphics.SetDialogueCommandInFlight(true);
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
    audioFeedback.TrackCommand(pendingCraftingInteraction->commandID, AudioCue::INVENTORY);
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

void Engine::Update(Clock::TimePoint now)
{
    const Clock::TimePoint updateStart =
        Clock::ClockType::now();

    world.Update();
    SynchronizeDialoguePresentation();
    for (const auto cue : audioFeedback.ConsumeTick(playerID, world.GetCurrentTick(),
             world.GetActionLifecycleEvents(), world.GetGatheringRewardEvents(),
             world.GetMeleeCombatEvents(), world.GetCommandProcessingResults()))
        audio.Play(cue);

    // Consume before another catch-up tick can replace the published events.
    graphics.SynchronizeGatheringRewards(playerID, world.GetGatheringRewardEvents());
    graphics.SynchronizeMeleeResults(playerID, world.GetMeleeCombatEvents());

    if (playerPersistenceLifecycle)
    {
        const auto checkpoint = playerPersistenceLifecycle->PollAutosave(world, now);
        if (checkpoint.status == PlayerAutosaveStatus::SAVED)
        {
            graphics.SetPlayerSaveNotice(Graphics::PlayerSaveNotice::SAVED);
            if (autosaveWarningActive) Logger::Info("Player autosave recovered; progress saved");
            autosaveWarningActive = false;
        }
        else if (checkpoint.status == PlayerAutosaveStatus::FAILED)
        {
            const bool craftingBlocked = checkpoint.operation->GetWorldSaveResult().Contains(
                WorldPlayerPersistenceIssueCode::CRAFTING_ITEMS_NOT_SAVED);
            graphics.SetPlayerSaveNotice(craftingBlocked ? Graphics::PlayerSaveNotice::CRAFTING_BLOCKED
                                                        : Graphics::PlayerSaveNotice::RETRYING);
            if (!autosaveWarningActive)
                Logger::Warn("Player autosave failed; retrying in 10 seconds | path=" +
                    playerPersistenceLifecycle->GetSavePath().string() +
                    (craftingBlocked ? " | crafting items need bag space" : " | check save path/storage"));
            autosaveWarningActive = true;
        }
    }

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

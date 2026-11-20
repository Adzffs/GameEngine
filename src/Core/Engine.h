#pragma once

#include "../Time/Clock.h"
#include "../Time/TickPerformance.h"
#include "../World/World.h"
#include "../Input/InputManager.h"
#include "../Graphics/Graphics.h"
#include "../Audio/AudioFeedbackState.h"
#include "../Audio/SdlAudioPlayer.h"
#include "EngineConfiguration.h"
#include "../Persistence/WorldPlayerPersistenceLifecycle.h"

#include <cstdint>
#include <memory>
#include <optional>

struct EngineTestAccess;

class Engine
{
public:
    Engine();
    explicit Engine(EngineConfiguration configuration);
    bool Run();

private:
    friend struct EngineTestAccess;

    bool running = true;

    Clock clock;
    World world;
    InputManager inputManager;
    Graphics graphics;
    // Destroy audio before Graphics shuts SDL down.
    SdlAudioPlayer audio;
    AudioFeedbackState audioFeedback;
    TickPerformanceStats tickPerformanceStats;

    int playerID = -1;
    PlayerInitializationMode newPlayerMode = PlayerInitializationMode::DEVELOPMENT_DEFAULTS;
    bool initializationSucceeded = false;
    std::unique_ptr<WorldPlayerPersistenceLifecycle> playerPersistenceLifecycle;

    static constexpr int MaxCatchUpTicks = 3;

    void Update();
    bool InitializeWorldForRun();
    bool FinalizeWorldAfterRun();
    void AddDevelopmentEquipment();
    void SynchronizeDialoguePresentation();
    bool EnqueuePendingDialogueCommand();
    void SynchronizeShopPresentation();
    bool EnqueuePendingShopCommand();
    void SynchronizeBankPresentation();
    bool EnqueuePendingBankCommand();
    void SynchronizeRecipeCommand();
    bool EnqueuePendingRecipeCommand();
    bool EnqueuePendingCraftingInteraction();
    void SynchronizeCraftingInteraction();
    enum class PendingShopCommandType { NONE, BUY, SELL, CLOSE };
    struct PendingShopCommand
    {
        std::uint64_t commandID = 0;
        int actorEntityID = 0;
        ShopSessionId sessionID = InvalidShopSessionId;
        PendingShopCommandType type = PendingShopCommandType::NONE;
    };
    std::optional<PendingShopCommand> pendingShopCommand;
    void ClearPendingShopCommand();
    enum class PendingRecipeCommandType
    {
        NONE,
        FURNACE_RECIPE,
        CRAFTING_GRID
    };
    struct PendingRecipeCommand
    {
        std::uint64_t commandID = 0;
        int actorEntityID = 0;
        StationType stationType = StationType::NONE;
        PendingRecipeCommandType type =
            PendingRecipeCommandType::NONE;
    };
    std::optional<PendingRecipeCommand> pendingRecipeCommand;
    void ClearPendingRecipeCommand();
    struct PendingCraftingInteraction
    {
        std::uint64_t commandID = 0;
        int actorEntityID = 0;
        CraftingSessionId sessionID = InvalidCraftingSessionId;
        std::uint64_t expectedRevision = 0;
        std::optional<CraftingRecipeMatcher::Grid> deferredCraftingGrid;
    };
    std::optional<PendingCraftingInteraction> pendingCraftingInteraction;
};

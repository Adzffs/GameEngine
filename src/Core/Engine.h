#pragma once

#include "../Time/Clock.h"
#include "../Time/TickPerformance.h"
#include "../World/World.h"
#include "../Input/InputManager.h"
#include "../Graphics/Graphics.h"
#include "EngineConfiguration.h"
#include "../Persistence/WorldPlayerPersistenceLifecycle.h"

#include <memory>

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
    TickPerformanceStats tickPerformanceStats;

    int playerID = -1;
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
};

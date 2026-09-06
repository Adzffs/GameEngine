#pragma once

#include "WorldPlayerPersistenceLifecycleResult.h"

#include <filesystem>
#include <optional>

class World;

enum class WorldPlayerPersistenceLifecycleState
{
    NOT_STARTED,
    ACTIVE,
    SHUT_DOWN,
    FAILED
};

class WorldPlayerPersistenceLifecycle
{
public:
    explicit WorldPlayerPersistenceLifecycle(std::filesystem::path savePath);

    WorldPlayerPersistenceStartupResult Start(World &world);
    WorldPlayerPersistenceOperationResult SaveNow(const World &world);
    WorldPlayerPersistenceOperationResult Shutdown(const World &world);

    WorldPlayerPersistenceLifecycleState GetState() const { return state; }
    std::optional<int> GetTrackedPlayerEntityID() const { return playerEntityID; }
    const std::filesystem::path &GetSavePath() const { return savePath; }

private:
    WorldPlayerPersistenceOperationResult SaveActivePlayer(const World &world);

    std::filesystem::path savePath;
    WorldPlayerPersistenceLifecycleState state =
        WorldPlayerPersistenceLifecycleState::NOT_STARTED;
    std::optional<int> playerEntityID;
};

#pragma once

#include "WorldPlayerPersistenceLifecycleResult.h"
#include "../Player/PlayerInitializationMode.h"

#include <filesystem>
#include <optional>
#include <chrono>

class World;

enum class WorldPlayerPersistenceLifecycleState
{
    NOT_STARTED,
    ACTIVE,
    SHUT_DOWN,
    FAILED
};

enum class PlayerAutosaveStatus { INACTIVE, NOT_DUE, WAITING_FOR_RESPAWN, SAVED, FAILED };

struct PlayerAutosaveResult
{
    PlayerAutosaveStatus status = PlayerAutosaveStatus::INACTIVE;
    std::optional<WorldPlayerPersistenceOperationResult> operation;
};

class WorldPlayerPersistenceLifecycle
{
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    static constexpr std::chrono::seconds AutosaveInterval{60};
    static constexpr std::chrono::seconds RetryInterval{10};

    explicit WorldPlayerPersistenceLifecycle(std::filesystem::path savePath,
        PlayerInitializationMode newPlayerMode = PlayerInitializationMode::DEVELOPMENT_DEFAULTS);

    WorldPlayerPersistenceStartupResult Start(World &world, TimePoint now = Clock::now());
    WorldPlayerPersistenceOperationResult SaveNow(const World &world, TimePoint now = Clock::now());
    WorldPlayerPersistenceOperationResult Shutdown(const World &world, TimePoint now = Clock::now());
    // Call after a committed World tick; never mutates live gameplay state.
    PlayerAutosaveResult PollAutosave(const World &world, TimePoint now = Clock::now());

    WorldPlayerPersistenceLifecycleState GetState() const { return state; }
    std::optional<int> GetTrackedPlayerEntityID() const { return playerEntityID; }
    const std::filesystem::path &GetSavePath() const { return savePath; }

private:
    WorldPlayerPersistenceOperationResult SaveActivePlayer(const World &world);

    std::filesystem::path savePath;
    PlayerInitializationMode newPlayerMode;
    WorldPlayerPersistenceLifecycleState state =
        WorldPlayerPersistenceLifecycleState::NOT_STARTED;
    std::optional<int> playerEntityID;
    std::optional<TimePoint> nextAutosave;
};

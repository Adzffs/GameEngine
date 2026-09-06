#include "WorldPlayerPersistenceLifecycle.h"

#include "../World/World.h"

#include <utility>

WorldPlayerPersistenceLifecycle::WorldPlayerPersistenceLifecycle(
    std::filesystem::path savePath)
    : savePath(std::move(savePath))
{
}

WorldPlayerPersistenceStartupResult WorldPlayerPersistenceLifecycle::Start(World &world)
{
    WorldPlayerPersistenceStartupResult result;
    if (state != WorldPlayerPersistenceLifecycleState::NOT_STARTED)
    {
        result.AddIssue(WorldPlayerPersistenceLifecycleIssueSeverity::ERROR,
                        WorldPlayerPersistenceLifecycleIssueCode::ALREADY_STARTED,
                        "Player persistence lifecycle has already been started");
        return result;
    }
    if (savePath.empty())
    {
        state = WorldPlayerPersistenceLifecycleState::FAILED;
        result.AddIssue(WorldPlayerPersistenceLifecycleIssueSeverity::ERROR,
                        WorldPlayerPersistenceLifecycleIssueCode::INVALID_SAVE_PATH,
                        "Player persistence save path must not be empty");
        return result;
    }

    WorldPlayerLoadResult worldResult = world.LoadOrCreatePlayerFromFile(savePath);
    const bool loaded = worldResult.WasLoadedFromFile();
    const bool created = worldResult.WasCreatedNew();
    const int runtimeID = worldResult.GetPlayerEntityID();
    const bool worldSucceeded = worldResult.IsSuccess();
    result.SetWorldLoadResult(std::move(worldResult));
    if (!worldSucceeded)
    {
        state = WorldPlayerPersistenceLifecycleState::FAILED;
        result.AddIssue(WorldPlayerPersistenceLifecycleIssueSeverity::ERROR,
                        WorldPlayerPersistenceLifecycleIssueCode::STARTUP_LOAD_OR_CREATE_FAILED,
                        "World could not load or create the development Player");
        return result;
    }
    if (runtimeID <= 0)
    {
        state = WorldPlayerPersistenceLifecycleState::FAILED;
        result.AddIssue(WorldPlayerPersistenceLifecycleIssueSeverity::ERROR,
                        WorldPlayerPersistenceLifecycleIssueCode::TRACKED_PLAYER_INVALID,
                        "World returned an invalid runtime Player ID");
        return result;
    }
    if (!loaded && !created)
    {
        state = WorldPlayerPersistenceLifecycleState::FAILED;
        result.AddIssue(WorldPlayerPersistenceLifecycleIssueSeverity::ERROR,
                        WorldPlayerPersistenceLifecycleIssueCode::STARTUP_LOAD_OR_CREATE_FAILED,
                        "World returned an unexpected Player load status");
        return result;
    }

    result.SetPlayerEntityID(runtimeID);
    result.SetStatus(loaded ? WorldPlayerPersistenceStartupStatus::LOADED_EXISTING
                            : WorldPlayerPersistenceStartupStatus::CREATED_NEW);
    playerEntityID = runtimeID;
    state = WorldPlayerPersistenceLifecycleState::ACTIVE;
    return result;
}

WorldPlayerPersistenceOperationResult WorldPlayerPersistenceLifecycle::SaveActivePlayer(
    const World &world)
{
    WorldPlayerPersistenceOperationResult result;
    if (state != WorldPlayerPersistenceLifecycleState::ACTIVE)
    {
        result.AddIssue(WorldPlayerPersistenceLifecycleIssueSeverity::ERROR,
                        WorldPlayerPersistenceLifecycleIssueCode::NOT_ACTIVE,
                        "Player persistence lifecycle is not active");
        return result;
    }
    if (!playerEntityID.has_value() || *playerEntityID <= 0)
    {
        result.AddIssue(WorldPlayerPersistenceLifecycleIssueSeverity::ERROR,
                        WorldPlayerPersistenceLifecycleIssueCode::TRACKED_PLAYER_INVALID,
                        "Tracked runtime Player ID is invalid");
        return result;
    }

    const int runtimeID = *playerEntityID;
    WorldPlayerSaveResult worldResult = world.SavePlayerToFile(runtimeID, savePath);
    const bool saved = worldResult.IsSuccess();
    result.SetWorldSaveResult(std::move(worldResult));
    result.SetPlayerEntityID(runtimeID);
    if (!saved)
    {
        result.AddIssue(WorldPlayerPersistenceLifecycleIssueSeverity::ERROR,
                        WorldPlayerPersistenceLifecycleIssueCode::SAVE_FAILED,
                        "World could not save the tracked development Player");
        return result;
    }
    result.SetStatus(WorldPlayerPersistenceOperationStatus::SAVED);
    return result;
}

WorldPlayerPersistenceOperationResult WorldPlayerPersistenceLifecycle::SaveNow(const World &world)
{
    return SaveActivePlayer(world);
}

WorldPlayerPersistenceOperationResult WorldPlayerPersistenceLifecycle::Shutdown(const World &world)
{
    if (state == WorldPlayerPersistenceLifecycleState::SHUT_DOWN)
    {
        WorldPlayerPersistenceOperationResult result;
        result.SetStatus(WorldPlayerPersistenceOperationStatus::ALREADY_SHUT_DOWN);
        if (playerEntityID.has_value()) result.SetPlayerEntityID(*playerEntityID);
        return result;
    }

    WorldPlayerPersistenceOperationResult result = SaveActivePlayer(world);
    if (result.IsSuccess()) state = WorldPlayerPersistenceLifecycleState::SHUT_DOWN;
    return result;
}

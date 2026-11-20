#include "WorldPlayerPersistenceLifecycle.h"

#include "../World/World.h"
#include "../Player/Player.h"

#include <utility>

WorldPlayerPersistenceLifecycle::WorldPlayerPersistenceLifecycle(
    std::filesystem::path savePath, PlayerInitializationMode newPlayerMode)
    : savePath(std::move(savePath)), newPlayerMode(newPlayerMode)
{
}

WorldPlayerPersistenceStartupResult WorldPlayerPersistenceLifecycle::Start(World &world, TimePoint now)
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

    WorldPlayerLoadResult worldResult = world.LoadOrCreatePlayerFromFile(savePath, newPlayerMode);
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
    nextAutosave = now + AutosaveInterval;
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

WorldPlayerPersistenceOperationResult WorldPlayerPersistenceLifecycle::SaveNow(const World &world, TimePoint now)
{
    auto result = SaveActivePlayer(world);
    if (state == WorldPlayerPersistenceLifecycleState::ACTIVE)
        nextAutosave = now + (result.IsSuccess() ? AutosaveInterval : RetryInterval);
    return result;
}

PlayerAutosaveResult WorldPlayerPersistenceLifecycle::PollAutosave(const World &world, TimePoint now)
{
    if (state != WorldPlayerPersistenceLifecycleState::ACTIVE || !nextAutosave)
        return {};
    if (now < *nextAutosave)
        return {PlayerAutosaveStatus::NOT_DUE, std::nullopt};

    // Death is a normal transient state, not a failed file write. Leave the
    // deadline due so the first completed respawn tick can checkpoint the pile.
    for (const auto &entity : world.GetEntities())
        if (playerEntityID && entity->GetID() == *playerEntityID)
            if (const auto *player = dynamic_cast<const Player *>(entity.get());
                player != nullptr && !player->IsAlive())
                return {PlayerAutosaveStatus::WAITING_FOR_RESPAWN, std::nullopt};

    auto result = SaveNow(world, now);
    const auto status = result.IsSuccess() ? PlayerAutosaveStatus::SAVED : PlayerAutosaveStatus::FAILED;
    return {status, std::move(result)};
}

WorldPlayerPersistenceOperationResult WorldPlayerPersistenceLifecycle::Shutdown(const World &world, TimePoint now)
{
    if (state == WorldPlayerPersistenceLifecycleState::SHUT_DOWN)
    {
        WorldPlayerPersistenceOperationResult result;
        result.SetStatus(WorldPlayerPersistenceOperationStatus::ALREADY_SHUT_DOWN);
        if (playerEntityID.has_value()) result.SetPlayerEntityID(*playerEntityID);
        return result;
    }

    WorldPlayerPersistenceOperationResult result = SaveNow(world, now);
    if (result.IsSuccess())
    {
        state = WorldPlayerPersistenceLifecycleState::SHUT_DOWN;
        nextAutosave.reset();
    }
    return result;
}

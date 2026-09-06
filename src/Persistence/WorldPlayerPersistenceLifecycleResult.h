#pragma once

#include "WorldPlayerPersistenceResult.h"

#include <string>
#include <utility>
#include <vector>

enum class WorldPlayerPersistenceLifecycleIssueSeverity
{
    WARNING,
    ERROR
};

enum class WorldPlayerPersistenceLifecycleIssueCode
{
    INVALID_SAVE_PATH,
    ALREADY_STARTED,
    NOT_ACTIVE,
    STARTUP_LOAD_OR_CREATE_FAILED,
    TRACKED_PLAYER_INVALID,
    SAVE_FAILED
};

struct WorldPlayerPersistenceLifecycleIssue
{
    WorldPlayerPersistenceLifecycleIssueSeverity severity;
    WorldPlayerPersistenceLifecycleIssueCode code;
    std::string message;
};

enum class WorldPlayerPersistenceStartupStatus
{
    LOADED_EXISTING,
    CREATED_NEW,
    FAILURE
};

class WorldPlayerPersistenceStartupResult
{
public:
    WorldPlayerPersistenceStartupStatus GetStatus() const { return status; }
    bool IsSuccess() const
    {
        return (status == WorldPlayerPersistenceStartupStatus::LOADED_EXISTING ||
                status == WorldPlayerPersistenceStartupStatus::CREATED_NEW) &&
               playerEntityID > 0 && !HasErrors();
    }
    bool WasLoadedFromFile() const
    {
        return status == WorldPlayerPersistenceStartupStatus::LOADED_EXISTING;
    }
    bool WasCreatedNew() const
    {
        return status == WorldPlayerPersistenceStartupStatus::CREATED_NEW;
    }
    int GetPlayerEntityID() const { return playerEntityID; }
    const std::vector<WorldPlayerPersistenceLifecycleIssue> &GetIssues() const { return issues; }
    bool Contains(WorldPlayerPersistenceLifecycleIssueCode code) const
    {
        for (const auto &issue : issues)
            if (issue.code == code) return true;
        return false;
    }
    const WorldPlayerLoadResult &GetWorldLoadResult() const { return worldLoadResult; }

private:
    friend class WorldPlayerPersistenceLifecycle;

    void SetStatus(WorldPlayerPersistenceStartupStatus value) { status = value; }
    void SetPlayerEntityID(int value) { playerEntityID = value; }
    void SetWorldLoadResult(WorldPlayerLoadResult value) { worldLoadResult = std::move(value); }
    void AddIssue(WorldPlayerPersistenceLifecycleIssueSeverity severity,
                  WorldPlayerPersistenceLifecycleIssueCode code, std::string message)
    {
        issues.push_back({severity, code, std::move(message)});
    }
    bool HasErrors() const
    {
        for (const auto &issue : issues)
            if (issue.severity == WorldPlayerPersistenceLifecycleIssueSeverity::ERROR) return true;
        return false;
    }

    WorldPlayerPersistenceStartupStatus status = WorldPlayerPersistenceStartupStatus::FAILURE;
    int playerEntityID = 0;
    std::vector<WorldPlayerPersistenceLifecycleIssue> issues;
    WorldPlayerLoadResult worldLoadResult;
};

enum class WorldPlayerPersistenceOperationStatus
{
    SAVED,
    ALREADY_SHUT_DOWN,
    FAILURE
};

class WorldPlayerPersistenceOperationResult
{
public:
    WorldPlayerPersistenceOperationStatus GetStatus() const { return status; }
    bool IsSuccess() const
    {
        return (status == WorldPlayerPersistenceOperationStatus::SAVED ||
                status == WorldPlayerPersistenceOperationStatus::ALREADY_SHUT_DOWN) &&
               !HasErrors();
    }
    bool DidSave() const { return status == WorldPlayerPersistenceOperationStatus::SAVED; }
    int GetPlayerEntityID() const { return playerEntityID; }
    const std::vector<WorldPlayerPersistenceLifecycleIssue> &GetIssues() const { return issues; }
    bool Contains(WorldPlayerPersistenceLifecycleIssueCode code) const
    {
        for (const auto &issue : issues)
            if (issue.code == code) return true;
        return false;
    }
    const WorldPlayerSaveResult &GetWorldSaveResult() const { return worldSaveResult; }

private:
    friend class WorldPlayerPersistenceLifecycle;

    void SetStatus(WorldPlayerPersistenceOperationStatus value) { status = value; }
    void SetPlayerEntityID(int value) { playerEntityID = value; }
    void SetWorldSaveResult(WorldPlayerSaveResult value) { worldSaveResult = std::move(value); }
    void AddIssue(WorldPlayerPersistenceLifecycleIssueSeverity severity,
                  WorldPlayerPersistenceLifecycleIssueCode code, std::string message)
    {
        issues.push_back({severity, code, std::move(message)});
    }
    bool HasErrors() const
    {
        for (const auto &issue : issues)
            if (issue.severity == WorldPlayerPersistenceLifecycleIssueSeverity::ERROR) return true;
        return false;
    }

    WorldPlayerPersistenceOperationStatus status = WorldPlayerPersistenceOperationStatus::FAILURE;
    int playerEntityID = 0;
    std::vector<WorldPlayerPersistenceLifecycleIssue> issues;
    WorldPlayerSaveResult worldSaveResult;
};

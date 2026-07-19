#pragma once

#include "PlayerSaveFileResult.h"
#include "PlayerSaveValidation.h"

#include <string>
#include <utility>
#include <vector>

class World;

enum class WorldPlayerPersistenceIssueSeverity
{
    WARNING,
    ERROR
};

enum class WorldPlayerPersistenceIssueCode
{
    INVALID_RUNTIME_ENTITY_ID,
    ENTITY_NOT_FOUND,
    ENTITY_NOT_PLAYER,
    PLAYER_NOT_ALIVE,
    FILE_LOAD_FAILED,
    FILE_SAVE_FAILED,
    PLAYER_RECONSTRUCTION_FAILED,
    SAVED_POSITION_OUT_OF_BOUNDS,
    SAVED_POSITION_BLOCKED,
    DEVELOPMENT_SPAWN_INVALID,
    ENTITY_REGISTRATION_FAILED
};

struct WorldPlayerPersistenceIssue
{
    WorldPlayerPersistenceIssueSeverity severity;
    WorldPlayerPersistenceIssueCode code;
    int entityID = 0;
    std::string message;
};

enum class WorldPlayerLoadStatus
{
    LOADED_EXISTING,
    CREATED_NEW,
    FAILURE
};

class WorldPlayerLoadResult
{
public:
    WorldPlayerLoadStatus GetStatus() const { return status; }
    bool IsSuccess() const
    {
        return (status == WorldPlayerLoadStatus::LOADED_EXISTING ||
                status == WorldPlayerLoadStatus::CREATED_NEW) &&
               playerEntityID > 0 && !HasErrors();
    }
    bool WasLoadedFromFile() const { return status == WorldPlayerLoadStatus::LOADED_EXISTING; }
    bool WasCreatedNew() const { return status == WorldPlayerLoadStatus::CREATED_NEW; }
    int GetPlayerEntityID() const { return playerEntityID; }
    bool UsedSpawnFallback() const { return usedSpawnFallback; }
    const std::vector<WorldPlayerPersistenceIssue> &GetIssues() const { return issues; }
    bool Contains(WorldPlayerPersistenceIssueCode code) const
    {
        for (const auto &issue : issues)
            if (issue.code == code) return true;
        return false;
    }
    const PlayerSaveFileLoadResult &GetFileLoadResult() const { return fileLoadResult; }
    const PlayerSaveValidationReport &GetReconstructionValidationReport() const
    {
        return reconstructionValidationReport;
    }

private:
    friend class World;

    void SetFileLoadResult(PlayerSaveFileLoadResult result) { fileLoadResult = std::move(result); }
    void SetStatus(WorldPlayerLoadStatus value) { status = value; }
    void SetPlayerEntityID(int value) { playerEntityID = value; }
    void SetUsedSpawnFallback() { usedSpawnFallback = true; }
    void SetReconstructionValidationReport(PlayerSaveValidationReport report)
    {
        reconstructionValidationReport = std::move(report);
    }
    void AddIssue(WorldPlayerPersistenceIssueSeverity severity,
                  WorldPlayerPersistenceIssueCode code, int entityID,
                  std::string message)
    {
        issues.push_back({severity, code, entityID, std::move(message)});
    }

    bool HasErrors() const
    {
        for (const auto &issue : issues)
            if (issue.severity == WorldPlayerPersistenceIssueSeverity::ERROR) return true;
        return false;
    }

    WorldPlayerLoadStatus status = WorldPlayerLoadStatus::FAILURE;
    int playerEntityID = 0;
    bool usedSpawnFallback = false;
    std::vector<WorldPlayerPersistenceIssue> issues;
    PlayerSaveFileLoadResult fileLoadResult;
    PlayerSaveValidationReport reconstructionValidationReport;
};

class WorldPlayerSaveResult
{
public:
    bool IsSuccess() const { return playerEntityID > 0 && fileSaveResult.IsSuccess() && !HasErrors(); }
    int GetPlayerEntityID() const { return playerEntityID; }
    const std::vector<WorldPlayerPersistenceIssue> &GetIssues() const { return issues; }
    bool Contains(WorldPlayerPersistenceIssueCode code) const
    {
        for (const auto &issue : issues)
            if (issue.code == code) return true;
        return false;
    }
    const PlayerSaveFileSaveResult &GetFileSaveResult() const { return fileSaveResult; }

private:
    friend class World;

    void SetPlayerEntityID(int value) { playerEntityID = value; }
    void SetFileSaveResult(PlayerSaveFileSaveResult result) { fileSaveResult = std::move(result); }
    void AddIssue(WorldPlayerPersistenceIssueSeverity severity,
                  WorldPlayerPersistenceIssueCode code, int entityID,
                  std::string message)
    {
        issues.push_back({severity, code, entityID, std::move(message)});
    }

    bool HasErrors() const
    {
        for (const auto &issue : issues)
            if (issue.severity == WorldPlayerPersistenceIssueSeverity::ERROR) return true;
        return false;
    }

    int playerEntityID = 0;
    std::vector<WorldPlayerPersistenceIssue> issues;
    PlayerSaveFileSaveResult fileSaveResult;
};

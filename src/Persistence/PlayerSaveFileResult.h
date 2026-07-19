#pragma once

#include "PlayerSaveData.h"
#include "PlayerSaveTextCodec.h"
#include "PlayerSaveValidation.h"

#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

enum class PlayerSaveFileIssueSeverity { WARNING, ERROR };

enum class PlayerSaveFileIssueCode
{
    INVALID_PATH,
    PARENT_PATH_IS_NOT_DIRECTORY, CREATE_DIRECTORY_FAILED, TARGET_IS_DIRECTORY,
    ENCODE_FAILED, ENCODED_TEXT_TOO_LARGE,
    TEMP_REMOVE_FAILED, TEMP_OPEN_FAILED, TEMP_WRITE_FAILED, TEMP_FLUSH_FAILED,
    TEMP_CLOSE_FAILED, TEMP_VERIFY_OPEN_FAILED, TEMP_VERIFY_READ_FAILED, TEMP_VERIFY_MISMATCH,
    BACKUP_REMOVE_FAILED, TARGET_TO_BACKUP_FAILED, TEMP_TO_TARGET_FAILED, ROLLBACK_FAILED,
    FILE_NOT_FOUND, FILE_OPEN_FAILED, FILE_TOO_LARGE, FILE_READ_FAILED, DECODE_FAILED,
    RECOVERY_BACKUP_RESTORE_FAILED, RECOVERY_TEMP_PROMOTION_FAILED,
    RECOVERY_INVALID_TEMP_DISCARDED, STALE_TEMP_CLEANUP_FAILED, STALE_BACKUP_CLEANUP_FAILED
};

struct PlayerSaveFileIssue
{
    PlayerSaveFileIssueSeverity severity;
    PlayerSaveFileIssueCode code;
    std::filesystem::path path;
    std::error_code systemError;
    std::string message;
};

class PlayerSaveFileSaveResult
{
public:
    bool IsSuccess() const;
    bool WasCommitted() const;
    const std::vector<PlayerSaveFileIssue> &GetIssues() const;
    bool Contains(PlayerSaveFileIssueCode code) const;
    const PlayerSaveValidationReport &GetValidationReport() const;

    void AddIssue(PlayerSaveFileIssueSeverity severity, PlayerSaveFileIssueCode code,
                  std::filesystem::path path, std::error_code systemError, std::string message);
    void SetCommitted();
    void SetValidationReport(PlayerSaveValidationReport report);

private:
    bool committed = false;
    std::vector<PlayerSaveFileIssue> issues;
    PlayerSaveValidationReport validationReport;
};

enum class PlayerSaveFileLoadStatus { SUCCESS, NOT_FOUND, FAILURE };

class PlayerSaveFileLoadResult
{
public:
    bool IsSuccess() const;
    bool IsNotFound() const;
    PlayerSaveFileLoadStatus GetStatus() const;
    const std::optional<PlayerSaveData> &GetSaveData() const;
    const std::vector<PlayerSaveFileIssue> &GetIssues() const;
    bool Contains(PlayerSaveFileIssueCode code) const;
    const std::optional<PlayerSaveTextDecodeResult> &GetDecodeResult() const;

    void AddIssue(PlayerSaveFileIssueSeverity severity, PlayerSaveFileIssueCode code,
                  std::filesystem::path path, std::error_code systemError, std::string message);
    void SetNotFound();
    void SetFailure();
    void SetDecodeFailure(PlayerSaveTextDecodeResult decodeResult);
    void SetLoaded(PlayerSaveData saveData, PlayerSaveTextDecodeResult decodeResult);

private:
    PlayerSaveFileLoadStatus status = PlayerSaveFileLoadStatus::FAILURE;
    std::optional<PlayerSaveData> saveData;
    std::vector<PlayerSaveFileIssue> issues;
    std::optional<PlayerSaveTextDecodeResult> decodeResult;
};

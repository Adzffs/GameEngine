#include "PlayerSaveFileStore.h"

#include "PlayerSaveTextCodec.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <utility>

namespace
{
    constexpr std::size_t READ_CHUNK_BYTES = 4096;

    std::filesystem::path Sidecar(const std::filesystem::path &target, const char *suffix)
    {
        std::filesystem::path result = target;
        result += suffix;
        return result;
    }

    enum class ReadStatus { SUCCESS, OPEN_FAILED, TOO_LARGE, READ_FAILED };
    struct ReadResult { ReadStatus status; std::string bytes; std::error_code systemError; };

    ReadResult ReadBounded(const std::filesystem::path &path)
    {
        std::ifstream input(path, std::ios::binary | std::ios::in);
        if (!input.is_open())
            return {ReadStatus::OPEN_FAILED, {}, std::make_error_code(std::errc::io_error)};
        std::string bytes;
        bytes.reserve(READ_CHUNK_BYTES);
        std::array<char, READ_CHUNK_BYTES> chunk{};
        while (input)
        {
            input.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
            const std::streamsize count = input.gcount();
            if (count > 0)
            {
                if (bytes.size() + static_cast<std::size_t>(count) > MAX_PLAYER_SAVE_TEXT_BYTES)
                    return {ReadStatus::TOO_LARGE, {}, {}};
                bytes.append(chunk.data(), static_cast<std::size_t>(count));
            }
        }
        if (input.bad())
            return {ReadStatus::READ_FAILED, {}, std::make_error_code(std::errc::io_error)};
        return {ReadStatus::SUCCESS, std::move(bytes), {}};
    }

    bool Exists(const std::filesystem::path &path, std::error_code &error)
    {
        error.clear();
        return std::filesystem::exists(path, error);
    }

    bool ValidPath(const std::filesystem::path &target, PlayerSaveFileSaveResult &result)
    {
        const auto filename = target.filename();
        if (target.empty() || filename.empty() || filename == "." || filename == "..")
        {
            result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::INVALID_PATH,
                            target, {}, "Target path must name a file");
            return false;
        }
        std::error_code error;
        const bool targetExists = Exists(target, error);
        if (error)
        {
            result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::INVALID_PATH,
                            target, error, "Could not inspect target path");
            return false;
        }
        if (targetExists && std::filesystem::is_directory(target, error))
        {
            result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::TARGET_IS_DIRECTORY,
                            target, error, "Target path is a directory");
            return false;
        }
        const auto parent = target.parent_path();
        const bool parentExists = !parent.empty() && Exists(parent, error);
        if (error)
        {
            result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::INVALID_PATH,
                            parent, error, "Could not inspect target parent");
            return false;
        }
        const bool parentIsDirectory = !parentExists || std::filesystem::is_directory(parent, error);
        if (error)
        {
            result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::INVALID_PATH,
                            parent, error, "Could not inspect target parent type");
            return false;
        }
        if (parentExists && !parentIsDirectory)
        {
            result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::PARENT_PATH_IS_NOT_DIRECTORY,
                            parent, error, "Target parent is not a directory");
            return false;
        }
        return true;
    }

    bool ValidPath(const std::filesystem::path &target, PlayerSaveFileLoadResult &result)
    {
        const auto filename = target.filename();
        if (target.empty() || filename.empty() || filename == "." || filename == "..")
        {
            result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::INVALID_PATH,
                            target, {}, "Target path must name a file");
            return false;
        }
        std::error_code error;
        const bool targetExists = Exists(target, error);
        if (error)
        {
            result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::INVALID_PATH,
                            target, error, "Could not inspect target path");
            return false;
        }
        if (targetExists && std::filesystem::is_directory(target, error))
        {
            result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::TARGET_IS_DIRECTORY,
                            target, error, "Target path is a directory");
            return false;
        }
        const auto parent = target.parent_path();
        const bool parentExists = !parent.empty() && Exists(parent, error);
        if (error)
        {
            result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::INVALID_PATH,
                            parent, error, "Could not inspect target parent");
            return false;
        }
        const bool parentIsDirectory = !parentExists || std::filesystem::is_directory(parent, error);
        if (error)
        {
            result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::INVALID_PATH,
                            parent, error, "Could not inspect target parent type");
            return false;
        }
        if (parentExists && !parentIsDirectory)
        {
            result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::PARENT_PATH_IS_NOT_DIRECTORY,
                            parent, error, "Target parent is not a directory");
            return false;
        }
        return true;
    }

    template <typename Result>
    bool RemoveArtifact(const std::filesystem::path &path, Result &result,
                        PlayerSaveFileIssueCode code, PlayerSaveFileIssueSeverity severity,
                        const char *message)
    {
        std::error_code error;
        if (!Exists(path, error))
        {
            if (!error) return true;
            result.AddIssue(severity, code, path, error, message);
            return severity == PlayerSaveFileIssueSeverity::WARNING;
        }
        const bool regularFile = std::filesystem::is_regular_file(path, error);
        if (error || !regularFile)
        {
            result.AddIssue(severity, code, path, error,
                            "Transaction sidecar is not a removable regular file");
            return severity == PlayerSaveFileIssueSeverity::WARNING;
        }
        if (!std::filesystem::remove(path, error))
        {
            result.AddIssue(severity, code, path, error, message);
            return severity == PlayerSaveFileIssueSeverity::WARNING;
        }
        return true;
    }

    template <typename Result>
    bool Reconcile(const std::filesystem::path &target, Result &result, bool forSave)
    {
        const auto temporary = Sidecar(target, ".tmp");
        const auto backup = Sidecar(target, ".bak");
        std::error_code error;
        const bool targetExists = Exists(target, error);
        if (error)
        {
            result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::INVALID_PATH,
                            target, error, "Could not inspect transaction target");
            return false;
        }
        const bool temporaryExists = Exists(temporary, error);
        if (error)
        {
            result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::INVALID_PATH,
                            temporary, error, "Could not inspect transaction temporary");
            return false;
        }
        const bool backupExists = Exists(backup, error);
        if (error)
        {
            result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::INVALID_PATH,
                            backup, error, "Could not inspect transaction artifacts");
            return false;
        }

        if (forSave && temporaryExists)
        {
            const bool temporaryIsRegular = std::filesystem::is_regular_file(temporary, error);
            if (error || !temporaryIsRegular)
            {
                result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::TEMP_REMOVE_FAILED,
                                temporary, error, "Temporary save path is not a reusable file");
                return false;
            }
        }

        if (targetExists)
        {
            const auto tempSeverity = forSave ? PlayerSaveFileIssueSeverity::ERROR : PlayerSaveFileIssueSeverity::WARNING;
            if (temporaryExists && !RemoveArtifact(temporary, result,
                    PlayerSaveFileIssueCode::STALE_TEMP_CLEANUP_FAILED, tempSeverity,
                    "Could not remove stale temporary save")) return false;
            if (backupExists)
            {
                const auto backupSeverity = forSave ? PlayerSaveFileIssueSeverity::ERROR : PlayerSaveFileIssueSeverity::WARNING;
                if (!RemoveArtifact(backup, result,
                        PlayerSaveFileIssueCode::STALE_BACKUP_CLEANUP_FAILED, backupSeverity,
                        "Could not remove stale backup save")) return false;
            }
            return true;
        }

        if (backupExists)
        {
            if (temporaryExists && !RemoveArtifact(temporary, result,
                    PlayerSaveFileIssueCode::STALE_TEMP_CLEANUP_FAILED, PlayerSaveFileIssueSeverity::ERROR,
                    "Could not remove stale temporary before backup recovery")) return false;
            const bool backupIsRegular = std::filesystem::is_regular_file(backup, error);
            if (error || !backupIsRegular)
            {
                result.AddIssue(PlayerSaveFileIssueSeverity::ERROR,
                    PlayerSaveFileIssueCode::RECOVERY_BACKUP_RESTORE_FAILED, backup, error,
                    "Recovery backup is not a regular transaction file");
                return false;
            }
            std::filesystem::rename(backup, target, error);
            if (error)
            {
                result.AddIssue(PlayerSaveFileIssueSeverity::ERROR,
                    PlayerSaveFileIssueCode::RECOVERY_BACKUP_RESTORE_FAILED, backup, error,
                    "Could not restore committed backup");
                return false;
            }
            return true;
        }

        if (temporaryExists)
        {
            const ReadResult read = ReadBounded(temporary);
            bool valid = false;
            if (read.status == ReadStatus::SUCCESS)
                valid = PlayerSaveTextCodec::Decode(read.bytes).IsSuccess();
            if (valid)
            {
                std::filesystem::rename(temporary, target, error);
                if (error)
                {
                    result.AddIssue(PlayerSaveFileIssueSeverity::ERROR,
                        PlayerSaveFileIssueCode::RECOVERY_TEMP_PROMOTION_FAILED, temporary, error,
                        "Could not promote recovered temporary save");
                    return false;
                }
                return true;
            }
            if (!RemoveArtifact(temporary, result, PlayerSaveFileIssueCode::STALE_TEMP_CLEANUP_FAILED,
                                PlayerSaveFileIssueSeverity::ERROR,
                                "Could not discard invalid recovered temporary save")) return false;
            result.AddIssue(PlayerSaveFileIssueSeverity::WARNING,
                PlayerSaveFileIssueCode::RECOVERY_INVALID_TEMP_DISCARDED, temporary, {},
                "Discarded invalid interrupted first save");
        }
        return true;
    }

    void CleanupTemporary(const std::filesystem::path &temporary, PlayerSaveFileSaveResult &result)
    {
        RemoveArtifact(temporary, result, PlayerSaveFileIssueCode::TEMP_REMOVE_FAILED,
                       PlayerSaveFileIssueSeverity::ERROR, "Could not clean temporary save");
    }
}

bool PlayerSaveFileSaveResult::IsSuccess() const
{
    return committed && std::none_of(issues.begin(), issues.end(), [](const auto &issue)
    { return issue.severity == PlayerSaveFileIssueSeverity::ERROR; });
}
bool PlayerSaveFileSaveResult::WasCommitted() const { return committed; }
const std::vector<PlayerSaveFileIssue> &PlayerSaveFileSaveResult::GetIssues() const { return issues; }
bool PlayerSaveFileSaveResult::Contains(PlayerSaveFileIssueCode code) const
{ return std::any_of(issues.begin(), issues.end(), [code](const auto &issue) { return issue.code == code; }); }
const PlayerSaveValidationReport &PlayerSaveFileSaveResult::GetValidationReport() const { return validationReport; }
void PlayerSaveFileSaveResult::AddIssue(PlayerSaveFileIssueSeverity severity, PlayerSaveFileIssueCode code,
    std::filesystem::path path, std::error_code systemError, std::string message)
{ issues.push_back({severity, code, std::move(path), systemError, std::move(message)}); }
void PlayerSaveFileSaveResult::SetCommitted() { committed = true; }
void PlayerSaveFileSaveResult::SetValidationReport(PlayerSaveValidationReport report) { validationReport = std::move(report); }

bool PlayerSaveFileLoadResult::IsSuccess() const
{
    return status == PlayerSaveFileLoadStatus::SUCCESS &&
        std::none_of(issues.begin(), issues.end(), [](const auto &issue)
        { return issue.severity == PlayerSaveFileIssueSeverity::ERROR; });
}
bool PlayerSaveFileLoadResult::IsNotFound() const { return status == PlayerSaveFileLoadStatus::NOT_FOUND; }
PlayerSaveFileLoadStatus PlayerSaveFileLoadResult::GetStatus() const { return status; }
const std::optional<PlayerSaveData> &PlayerSaveFileLoadResult::GetSaveData() const { return saveData; }
const std::vector<PlayerSaveFileIssue> &PlayerSaveFileLoadResult::GetIssues() const { return issues; }
bool PlayerSaveFileLoadResult::Contains(PlayerSaveFileIssueCode code) const
{ return std::any_of(issues.begin(), issues.end(), [code](const auto &issue) { return issue.code == code; }); }
const std::optional<PlayerSaveTextDecodeResult> &PlayerSaveFileLoadResult::GetDecodeResult() const { return decodeResult; }
void PlayerSaveFileLoadResult::AddIssue(PlayerSaveFileIssueSeverity severity, PlayerSaveFileIssueCode code,
    std::filesystem::path path, std::error_code systemError, std::string message)
{ issues.push_back({severity, code, std::move(path), systemError, std::move(message)}); }
void PlayerSaveFileLoadResult::SetNotFound() { status = PlayerSaveFileLoadStatus::NOT_FOUND; saveData.reset(); }
void PlayerSaveFileLoadResult::SetFailure() { status = PlayerSaveFileLoadStatus::FAILURE; saveData.reset(); }
void PlayerSaveFileLoadResult::SetDecodeFailure(PlayerSaveTextDecodeResult decoded)
{ status = PlayerSaveFileLoadStatus::FAILURE; saveData.reset(); decodeResult = std::move(decoded); }
void PlayerSaveFileLoadResult::SetLoaded(PlayerSaveData data, PlayerSaveTextDecodeResult decoded)
{ status = PlayerSaveFileLoadStatus::SUCCESS; saveData = std::move(data); decodeResult = std::move(decoded); }

PlayerSaveFileSaveResult PlayerSaveFileStore::Save(
    const std::filesystem::path &targetPath, const PlayerSaveData &saveData)
{
    PlayerSaveFileSaveResult result;
    if (!ValidPath(targetPath, result)) return result;

    std::string encoded;
    PlayerSaveValidationReport validation;
    if (!PlayerSaveTextCodec::TryEncode(saveData, encoded, validation))
    {
        result.SetValidationReport(std::move(validation));
        result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::ENCODE_FAILED,
                        targetPath, {}, "Player save data could not be encoded");
        return result;
    }
    if (encoded.size() > MAX_PLAYER_SAVE_TEXT_BYTES)
    {
        result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::ENCODED_TEXT_TOO_LARGE,
                        targetPath, {}, "Encoded player save exceeds file limit");
        return result;
    }

    const auto parent = targetPath.parent_path();
    if (!parent.empty())
    {
        std::error_code error;
        std::filesystem::create_directories(parent, error);
        if (error)
        {
            result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::CREATE_DIRECTORY_FAILED,
                            parent, error, "Could not create save parent directory");
            return result;
        }
    }
    if (!Reconcile(targetPath, result, true)) return result;

    const auto temporary = Sidecar(targetPath, ".tmp");
    const auto backup = Sidecar(targetPath, ".bak");
    if (!RemoveArtifact(temporary, result, PlayerSaveFileIssueCode::TEMP_REMOVE_FAILED,
                        PlayerSaveFileIssueSeverity::ERROR, "Could not prepare temporary save")) return result;

    std::ofstream output(temporary, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!output.is_open())
    {
        result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::TEMP_OPEN_FAILED,
                        temporary, std::make_error_code(std::errc::io_error), "Could not open temporary save");
        CleanupTemporary(temporary, result); return result;
    }
    output.write(encoded.data(), static_cast<std::streamsize>(encoded.size()));
    if (!output)
    {
        result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::TEMP_WRITE_FAILED,
                        temporary, std::make_error_code(std::errc::io_error), "Could not write complete temporary save");
        output.close(); CleanupTemporary(temporary, result); return result;
    }
    output.flush();
    if (!output)
    {
        result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::TEMP_FLUSH_FAILED,
                        temporary, std::make_error_code(std::errc::io_error), "Could not flush temporary save");
        output.close(); CleanupTemporary(temporary, result); return result;
    }
    output.close();
    if (output.fail())
    {
        result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::TEMP_CLOSE_FAILED,
                        temporary, std::make_error_code(std::errc::io_error), "Could not close temporary save");
        CleanupTemporary(temporary, result); return result;
    }

    const ReadResult verification = ReadBounded(temporary);
    if (verification.status != ReadStatus::SUCCESS)
    {
        const auto code = verification.status == ReadStatus::OPEN_FAILED ?
            PlayerSaveFileIssueCode::TEMP_VERIFY_OPEN_FAILED : PlayerSaveFileIssueCode::TEMP_VERIFY_READ_FAILED;
        result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, code, temporary, verification.systemError,
                        "Could not verify temporary save");
        CleanupTemporary(temporary, result); return result;
    }
    if (verification.bytes != encoded)
    {
        result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::TEMP_VERIFY_MISMATCH,
                        temporary, {}, "Temporary save bytes did not match encoded text");
        CleanupTemporary(temporary, result); return result;
    }

    std::error_code error;
    const bool hadTarget = Exists(targetPath, error);
    if (error)
    {
        result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::INVALID_PATH,
                        targetPath, error, "Could not inspect target before replacement");
        CleanupTemporary(temporary, result);
        return result;
    }
    if (hadTarget)
    {
        if (!RemoveArtifact(backup, result, PlayerSaveFileIssueCode::BACKUP_REMOVE_FAILED,
                            PlayerSaveFileIssueSeverity::ERROR,
                            "Could not prepare replacement backup path"))
        {
            CleanupTemporary(temporary, result);
            return result;
        }
        std::filesystem::rename(targetPath, backup, error);
        if (error)
        {
            result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::TARGET_TO_BACKUP_FAILED,
                            targetPath, error, "Could not move committed save to backup");
            CleanupTemporary(temporary, result); return result;
        }
    }
    std::filesystem::rename(temporary, targetPath, error);
    if (error)
    {
        result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::TEMP_TO_TARGET_FAILED,
                        temporary, error, "Could not commit temporary save");
        if (hadTarget)
        {
            std::error_code rollbackError;
            std::filesystem::rename(backup, targetPath, rollbackError);
            if (rollbackError)
                result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::ROLLBACK_FAILED,
                                backup, rollbackError, "Could not restore previous committed save");
        }
        CleanupTemporary(temporary, result); return result;
    }
    result.SetCommitted();
    if (hadTarget)
        RemoveArtifact(backup, result, PlayerSaveFileIssueCode::BACKUP_REMOVE_FAILED,
                       PlayerSaveFileIssueSeverity::WARNING, "Committed save but could not remove backup");
    return result;
}

PlayerSaveFileLoadResult PlayerSaveFileStore::Load(const std::filesystem::path &targetPath)
{
    PlayerSaveFileLoadResult result;
    if (!ValidPath(targetPath, result)) return result;
    if (!Reconcile(targetPath, result, false)) return result;

    std::error_code error;
    const bool targetExists = Exists(targetPath, error);
    if (error)
    {
        result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::FILE_OPEN_FAILED,
                        targetPath, error, "Could not inspect recovered target");
        return result;
    }
    if (!targetExists)
    {
        result.SetNotFound();
        result.AddIssue(PlayerSaveFileIssueSeverity::WARNING, PlayerSaveFileIssueCode::FILE_NOT_FOUND,
                        targetPath, error, "Player save file was not found");
        return result;
    }
    const ReadResult read = ReadBounded(targetPath);
    if (read.status != ReadStatus::SUCCESS)
    {
        const auto code = read.status == ReadStatus::OPEN_FAILED ? PlayerSaveFileIssueCode::FILE_OPEN_FAILED :
            read.status == ReadStatus::TOO_LARGE ? PlayerSaveFileIssueCode::FILE_TOO_LARGE :
            PlayerSaveFileIssueCode::FILE_READ_FAILED;
        result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, code, targetPath, read.systemError,
                        "Could not read bounded player save file");
        return result;
    }
    PlayerSaveTextDecodeResult decoded = PlayerSaveTextCodec::Decode(read.bytes);
    if (!decoded.IsSuccess())
    {
        result.AddIssue(PlayerSaveFileIssueSeverity::ERROR, PlayerSaveFileIssueCode::DECODE_FAILED,
                        targetPath, {}, "Player save file text could not be decoded");
        result.SetDecodeFailure(std::move(decoded));
        return result;
    }
    PlayerSaveData save = *decoded.GetSaveData();
    result.SetLoaded(std::move(save), std::move(decoded));
    return result;
}

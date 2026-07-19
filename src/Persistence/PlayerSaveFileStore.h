#pragma once

#include "PlayerSaveData.h"
#include "PlayerSaveFileResult.h"

#include <filesystem>

// This store provides logical replacement, rollback, and sidecar recovery during
// normal process execution. It does not force file data or directory entries to
// physical storage: portable C++ has no fsync or Windows write-through guarantee.
// Abrupt power loss at rename boundaries, adversarial concurrent mutation, and
// symlink hardening remain outside this trusted local-development boundary.
class PlayerSaveFileStore
{
public:
    static PlayerSaveFileSaveResult Save(
        const std::filesystem::path &targetPath,
        const PlayerSaveData &saveData);

    static PlayerSaveFileLoadResult Load(
        const std::filesystem::path &targetPath);
};

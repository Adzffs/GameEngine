#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string_view>

struct GameLaunchOptions
{
    bool showHelp = false;
    std::optional<std::filesystem::path> worldSnapshotPath;
    std::optional<std::filesystem::path> playerSavePath;
};

// Arguments exclude argv[0]. Imported-world playtests are temporary unless a
// save file is explicitly selected. Normal no-argument startup keeps its save.
// Invalid/ambiguous arguments throw std::invalid_argument before any I/O.
GameLaunchOptions ParseGameLaunchOptions(std::span<const std::string_view> arguments);
// Prevent the player-save transaction (including its recovery sidecars) from
// replacing or deleting the selected world input through aliases or links.
void ValidateGameLaunchPaths(const GameLaunchOptions &options);
std::string_view GameLaunchUsage();

#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string_view>
#include "../Player/PlayerInitializationMode.h"

struct GameLaunchOptions
{
    bool showHelp = false;
    bool characterPlaytest = false;
    std::optional<std::filesystem::path> worldSnapshotPath;
    std::optional<std::filesystem::path> worldVersionManifestPath;
    std::optional<std::filesystem::path> playerSavePath;
    PlayerInitializationMode newPlayerMode = PlayerInitializationMode::DEVELOPMENT_DEFAULTS;
};

// Arguments exclude argv[0]. Imported-world playtests are temporary unless a
// save file is explicitly selected. Fresh starts also have no implicit save.
// Normal no-argument startup keeps its development profile and save.
// Invalid/ambiguous arguments throw std::invalid_argument before any I/O.
GameLaunchOptions ParseGameLaunchOptions(std::span<const std::string_view> arguments);
// Prevent the player-save transaction (including its recovery sidecars) from
// replacing or deleting the selected world input through aliases or links.
void ValidateGameLaunchPaths(const GameLaunchOptions &options);
std::string_view GameLaunchUsage();

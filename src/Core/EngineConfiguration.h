#pragma once

#include <filesystem>
#include <optional>
#include "../Content/Export/StarterWorldSnapshot.h"
#include "../Player/PlayerInitializationMode.h"

struct EngineConfiguration
{
    std::optional<std::filesystem::path> developmentPlayerSavePath;
    // Absent selects the compiled starter world. Present is validated by World.
    std::optional<StarterWorldExport::Snapshot> starterWorldSnapshot;
    // Applies only to newly created characters; loaded saves restore their own state.
    PlayerInitializationMode newPlayerMode = PlayerInitializationMode::DEVELOPMENT_DEFAULTS;
    bool characterPlaytest = false;
};

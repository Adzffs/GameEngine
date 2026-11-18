#pragma once

#include <filesystem>
#include <optional>
#include "../Content/Export/StarterWorldSnapshot.h"

struct EngineConfiguration
{
    std::optional<std::filesystem::path> developmentPlayerSavePath;
    // Absent selects the compiled starter world. Present is validated by World.
    std::optional<StarterWorldExport::Snapshot> starterWorldSnapshot;
};

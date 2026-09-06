#pragma once

#include <filesystem>
#include <optional>

struct EngineConfiguration
{
    std::optional<std::filesystem::path> developmentPlayerSavePath;
};

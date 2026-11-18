#pragma once

#include <filesystem>
#include <iosfwd>

// Interactive, terminal-based first editor surface. The shared typed history
// and guarded draft writer remain the authority for all changes and saves.
namespace EmbervaleStudioSession
{
    int Run(const std::filesystem::path &sourcePath,
        std::istream &input, std::ostream &output, std::ostream &errors);
}

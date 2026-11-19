#include "Content/Export/StarterWorldSnapshot.h"
#include "RiverCrossingBlockout.h"

#include <exception>
#include <iostream>
#include <string>
#include <string_view>

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#include <cstdio>
#endif

namespace
{
    constexpr std::string_view Usage =
        "Usage: EmbervaleWorldExport [--river-crossing | --help | --version]\n"
        "\n"
        "Write the compiled development starter world as canonical JSON to stdout.\n"
        "Example: EmbervaleWorldExport > development-starter.snapshot.json\n"
        "--river-crossing: build the separate validated River Crossing blockout (schema 2).\n"
        "\n"
        "This exports initial content. It does not load or edit maps or player saves.\n"
        "Snapshot format: embervale.starter-world-snapshot; baseline schema 1, River Crossing schema 2.\n";
}

int main(int argc, char **argv)
{
    if (argc == 2 && std::string_view(argv[1]) == "--help")
    {
        std::cout << Usage;
        return std::cout ? 0 : 1;
    }
    if (argc == 2 && std::string_view(argv[1]) == "--version")
    {
        std::cout << "EmbervaleWorldExport snapshot schema 1\n";
        return std::cout ? 0 : 1;
    }
    const bool riverCrossing = argc == 2 && std::string_view(argv[1]) == "--river-crossing";
    if (argc != 1 && !riverCrossing)
    {
        std::cerr << "Unsupported arguments.\n" << Usage;
        return 2;
    }

    try
    {
        // Serialise completely before touching stdout. No save or runtime world
        // lifecycle is entered, and failed capture cannot emit a partial model.
        const std::string json = StarterWorldExport::SerializeCanonical(
            riverCrossing ? RiverCrossingBlockout::Build() : StarterWorldExport::Capture());

#if defined(_WIN32)
        // The snapshot's canonical bytes use LF on every platform, including
        // redirected output from the Windows C runtime.
        if (_setmode(_fileno(stdout), _O_BINARY) == -1)
        {
            std::cerr << "Cannot configure canonical JSON output.\n";
            return 1;
        }
#endif

        std::cout.write(json.data(), static_cast<std::streamsize>(json.size()));
        std::cout.flush();
        if (!std::cout)
        {
            std::cerr << "Cannot write starter-world snapshot.\n";
            return 1;
        }
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Starter-world export failed: " << error.what() << '\n';
        return 1;
    }
}

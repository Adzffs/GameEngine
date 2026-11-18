#include "../Content/Edit/StarterWorldEdit.h"
#include "../Content/Import/StarterWorldRuntimeInput.h"

#include <charconv>
#include <exception>
#include <iostream>
#include <string_view>
#include <system_error>

namespace
{
    bool ParseCoordinate(std::string_view text, int &value)
    {
        if (text.empty()) return false;
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
        return error == std::errc{} && end == text.data() + text.size();
    }

    void Usage(std::ostream &output)
    {
        output << "Usage: EmbervaleWorldMove <snapshot.json> <placement-id> <x> <y>\n"
            "Validate a resource/station move by stable ID (dry run only).\n"
            "Does not change the input, create output files or modify player saves.\n";
    }
}

int main(int argc, char **argv)
{
    if (argc == 2 && std::string_view(argv[1]) == "--help")
    {
        Usage(std::cout);
        return 0;
    }
    int x = 0;
    int y = 0;
    if (argc != 5 || std::string_view(argv[1]).empty() ||
        std::string_view(argv[1]).starts_with("--") ||
        std::string_view(argv[2]).empty() ||
        !ParseCoordinate(argv[3], x) || !ParseCoordinate(argv[4], y))
    {
        Usage(std::cerr);
        return 2;
    }
    try
    {
        const auto source = StarterWorldImport::LoadFile(argv[1]);
        const auto result = StarterWorldEdit::MovePlacement(source, argv[2], Position{x, y});
        std::cout << (result.changed ? "Valid move: " : "Valid unchanged placement: ")
            << result.placementId << " (" << result.previousPosition.GetX() << ", "
            << result.previousPosition.GetY() << ") -> ("
            << result.newPosition.GetX() << ", " << result.newPosition.GetY()
            << "); dry run, no files written.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

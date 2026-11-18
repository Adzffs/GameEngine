#include "../Content/Import/StarterWorldRuntimeInput.h"

#include <exception>
#include <iostream>
#include <string_view>

int main(int argc, char **argv)
{
    if (argc == 2 && std::string_view(argv[1]) == "--help")
    {
        std::cout << "Usage: EmbervaleWorldValidate <snapshot.json>\n"
            "Validate a starter-world snapshot for runtime loading.\n"
            "Reads the selected file only; does not edit content or player saves.\n";
        return 0;
    }
    if (argc != 2 || std::string_view(argv[1]).empty() ||
        std::string_view(argv[1]).starts_with("--"))
    {
        std::cerr << "Usage: EmbervaleWorldValidate <snapshot.json>\n";
        return 2;
    }
    try
    {
        const auto snapshot = StarterWorldImport::LoadFile(argv[1]);
        std::cout << "Valid starter world: " << snapshot.width << " x " << snapshot.height
            << ", " << snapshot.placements.size() << " placements, player spawn ("
            << snapshot.playerSpawn.GetX() << ", " << snapshot.playerSpawn.GetY() << ").\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

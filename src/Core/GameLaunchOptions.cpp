#include "GameLaunchOptions.h"

#include <stdexcept>
#include <string>
#include <system_error>

GameLaunchOptions ParseGameLaunchOptions(std::span<const std::string_view> arguments)
{
    GameLaunchOptions options;
    bool compiledWorld = false;
    bool noSave = false;
    if (arguments.size() == 1 && arguments.front() == "--help")
    {
        options.showHelp = true;
        return options;
    }
    for (std::size_t i = 0; i < arguments.size(); ++i)
    {
        const auto argument = arguments[i];
        if (argument == "--compiled-world")
        {
            if (compiledWorld || options.worldSnapshotPath)
                throw std::invalid_argument("select exactly one world source");
            compiledWorld = true;
        }
        else if (argument == "--no-save")
        {
            if (noSave || options.playerSavePath)
                throw std::invalid_argument("select either --save or --no-save");
            noSave = true;
        }
        else if (argument == "--world" || argument == "--save")
        {
            if (i + 1 == arguments.size() || arguments[i + 1].empty() ||
                arguments[i + 1].starts_with("--"))
                throw std::invalid_argument("missing path after " + std::string(argument));
            const std::filesystem::path path{arguments[++i]};
            if (argument == "--world")
            {
                if (compiledWorld || options.worldSnapshotPath)
                    throw std::invalid_argument("select exactly one world source");
                options.worldSnapshotPath = path;
            }
            else
            {
                if (noSave || options.playerSavePath)
                    throw std::invalid_argument("select either --save or --no-save");
                options.playerSavePath = path;
            }
        }
        else
            throw std::invalid_argument("unknown argument: " + std::string(argument));
    }
    if (!options.playerSavePath && !noSave && !options.worldSnapshotPath)
        options.playerSavePath = std::filesystem::path{"saves"} / "development-player.save";
    return options;
}

void ValidateGameLaunchPaths(const GameLaunchOptions &options)
{
    if (!options.worldSnapshotPath || !options.playerSavePath) return;
    std::error_code error;
    const auto world = std::filesystem::weakly_canonical(*options.worldSnapshotPath, error);
    if (error) throw std::invalid_argument("cannot resolve selected world path");
    for (const char *suffix : {"", ".tmp", ".bak"})
    {
        auto destination = *options.playerSavePath;
        destination += suffix;
        const auto resolved = std::filesystem::weakly_canonical(destination, error);
        if (error) throw std::invalid_argument("cannot resolve selected save path");
        if (resolved == world)
            throw std::invalid_argument("player save and its recovery files must be separate from world content");
        const bool destinationExists = std::filesystem::exists(destination, error);
        if (error) throw std::invalid_argument("cannot inspect selected save path");
        if (destinationExists)
        {
            const bool sameFile = std::filesystem::equivalent(*options.worldSnapshotPath, destination, error);
            if (!error && sameFile)
                throw std::invalid_argument("player save must not alias selected world content");
            // A missing input is diagnosed by the bounded content loader.
            if (error && error != std::errc::no_such_file_or_directory)
                throw std::invalid_argument("cannot compare selected world and save paths");
            error.clear();
        }
    }
}

std::string_view GameLaunchUsage()
{
    return "Usage: GameEngine [--world <snapshot.json> | --compiled-world] "
        "[--save <player.save> | --no-save]\n"
        "       GameEngine --help\n"
        "\n"
        "No arguments: use the compiled starter world and normal development save.\n"
        "--world: validate and load the selected starter snapshot; a bad file fails startup.\n"
        "Imported-world playtests keep progress temporary unless --save is supplied.\n"
        "--compiled-world: explicitly select the original starter content.\n"
        "--save: use the selected player save file.\n"
        "--no-save: do not read or write a player save.\n";
}

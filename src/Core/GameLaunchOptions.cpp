#include "GameLaunchOptions.h"

#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

GameLaunchOptions ParseGameLaunchOptions(std::span<const std::string_view> arguments)
{
    GameLaunchOptions options;
    bool compiledWorld = false;
    bool noSave = false;
    bool profileSelected = false;
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
            if (compiledWorld || options.worldSnapshotPath || options.worldVersionManifestPath)
                throw std::invalid_argument("select exactly one world source");
            compiledWorld = true;
        }
        else if (argument == "--no-save")
        {
            if (noSave || options.playerSavePath)
                throw std::invalid_argument("select either --save or --no-save");
            noSave = true;
        }
        else if (argument == "--profile")
        {
            if (profileSelected)
                throw std::invalid_argument("select exactly one player profile");
            if (i + 1 == arguments.size())
                throw std::invalid_argument("missing profile after --profile (fresh or development)");
            const auto profile = arguments[++i];
            if (profile == "fresh")
                options.newPlayerMode = PlayerInitializationMode::FRESH_START;
            else if (profile == "development")
                options.newPlayerMode = PlayerInitializationMode::DEVELOPMENT_DEFAULTS;
            else
                throw std::invalid_argument("unknown player profile: " + std::string(profile));
            profileSelected = true;
        }
        else if (argument == "--world" || argument == "--world-version" || argument == "--save")
        {
            if (i + 1 == arguments.size() || arguments[i + 1].empty() ||
                arguments[i + 1].starts_with("--"))
                throw std::invalid_argument("missing path after " + std::string(argument));
            const std::filesystem::path path{arguments[++i]};
            if (argument == "--world" || argument == "--world-version")
            {
                if (compiledWorld || options.worldSnapshotPath || options.worldVersionManifestPath)
                    throw std::invalid_argument("select exactly one world source");
                if (argument == "--world") options.worldSnapshotPath = path;
                else options.worldVersionManifestPath = path;
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
    if (!options.playerSavePath && !noSave && !options.worldSnapshotPath && !options.worldVersionManifestPath &&
        options.newPlayerMode == PlayerInitializationMode::DEVELOPMENT_DEFAULTS)
        options.playerSavePath = std::filesystem::path{"saves"} / "development-player.save";
    return options;
}

void ValidateGameLaunchPaths(const GameLaunchOptions &options)
{
    if (!options.playerSavePath) return;
    std::vector<std::filesystem::path> inputs;
    if (options.worldSnapshotPath) inputs.push_back(*options.worldSnapshotPath);
    if (options.worldVersionManifestPath)
    {
        inputs.push_back(*options.worldVersionManifestPath);
        inputs.push_back(options.worldVersionManifestPath->parent_path() / "world.json");
    }
    if (inputs.empty()) return;
    std::error_code error;
    for (const char *suffix : {"", ".tmp", ".bak"})
    {
        auto destination = *options.playerSavePath;
        destination += suffix;
        const auto resolved = std::filesystem::weakly_canonical(destination, error);
        if (error) throw std::invalid_argument("cannot resolve selected save path");
        if (options.worldVersionManifestPath)
        {
            const auto library = std::filesystem::weakly_canonical(
                options.worldVersionManifestPath->parent_path().parent_path().empty() ? "." :
                options.worldVersionManifestPath->parent_path().parent_path(), error);
            if (error) throw std::invalid_argument("cannot resolve version library");
            auto part = resolved.begin();
            bool within = true;
            for (auto directory = library.begin(); directory != library.end(); ++directory, ++part)
                if (part == resolved.end() || *part != *directory) { within = false; break; }
            if (within) throw std::invalid_argument("player saves must stay outside the published version library");
            // Existing directory identities also catch case aliases on Windows
            // and alternate paths to the same library directory.
            auto ancestor = resolved.parent_path();
            while (!ancestor.empty())
            {
                const bool sameDirectory = std::filesystem::equivalent(ancestor, library, error);
                if (!error && sameDirectory)
                    throw std::invalid_argument("player saves must stay outside the published version library");
                if (error && error != std::errc::no_such_file_or_directory)
                    throw std::invalid_argument("cannot compare save and version library directories");
                error.clear();
                const auto next = ancestor.parent_path();
                if (next == ancestor) break;
                ancestor = next;
            }
        }
        const bool destinationExists = std::filesystem::exists(destination, error);
        if (error) throw std::invalid_argument("cannot inspect selected save path");
        for (const auto &input : inputs)
        {
            const auto world = std::filesystem::weakly_canonical(input, error);
            if (error) throw std::invalid_argument("cannot resolve selected world path");
            if (resolved == world)
                throw std::invalid_argument("player save and its recovery files must be separate from world content");
            if (!destinationExists) continue;
            const bool sameFile = std::filesystem::equivalent(input, destination, error);
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
    return "Usage: GameEngine [--world <snapshot.json> | --world-version <manifest.json> | --compiled-world] "
        "[--save <player.save> | --no-save] [--profile fresh|development]\n"
        "       GameEngine --help\n"
        "\n"
        "No arguments: use the compiled starter world and normal development save.\n"
        "--world: validate and load the selected starter snapshot; a bad file fails startup.\n"
        "--world-version: verify and revalidate an immutable published version manifest before startup.\n"
        "Published-version play keeps progress temporary unless --save outside the version library is supplied.\n"
        "Imported-world playtests keep progress temporary unless --save is supplied.\n"
        "--compiled-world: explicitly select the original starter content.\n"
        "--profile fresh: new characters get normal starting skills and bronze tools, with no developer equipment.\n"
        "Fresh starts keep progress temporary unless --save is supplied.\n"
        "--profile development: use the existing boosted development defaults (the default profile).\n"
        "Profiles apply only when creating a character; an existing selected save retains its progress.\n"
        "--save: use the selected player save file.\n"
        "--no-save: do not read or write a player save.\n";
}

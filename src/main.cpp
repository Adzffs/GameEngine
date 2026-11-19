#include <cstdlib>
#include <filesystem>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>
#include "Core/Engine.h"
#include "Core/ContentStartupValidation.h"
#include "Core/GameLaunchOptions.h"
#include "Content/Import/StarterWorldRuntimeInput.h"

int main(int argc, char **argv)
{
    GameLaunchOptions options;
    try
    {
        std::vector<std::string_view> arguments;
        for (int i = 1; i < argc; ++i) arguments.emplace_back(argv[i]);
        options = ParseGameLaunchOptions(arguments);
        ValidateGameLaunchPaths(options);
    }
    catch (const std::invalid_argument &error)
    {
        std::cerr << error.what() << '\n' << GameLaunchUsage();
        return 2;
    }
    if (options.showHelp)
    {
        std::cout << GameLaunchUsage();
        return EXIT_SUCCESS;
    }

    if (!ContentStartupValidation::ValidateAndLog()) return EXIT_FAILURE;

    try
    {
        EngineConfiguration configuration;
        configuration.newPlayerMode = options.newPlayerMode;
        if (options.worldSnapshotPath)
        {
            configuration.starterWorldSnapshot =
                StarterWorldImport::LoadFile(*options.worldSnapshotPath);
            if (!options.playerSavePath)
                std::cout << "World playtest: player progress is temporary. "
                    "Use --save <player.save> to keep it.\n";
        }
        configuration.developmentPlayerSavePath = options.playerSavePath;
        Engine engine(configuration);

        return engine.Run() ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Game startup failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}

#include <cstdlib>
#include <filesystem>
#include "Core/Engine.h"
#include "Core/ContentStartupValidation.h"

int main()
{
    if (!ContentStartupValidation::ValidateAndLog())
    {
        return EXIT_FAILURE;
    }

    // Development application policy. World remains unaware of this path.
    // Command-line and production path configuration are future work.
    EngineConfiguration configuration;
    configuration.developmentPlayerSavePath =
        std::filesystem::path{"saves"} / "development-player.save";
    Engine engine(configuration);

    return engine.Run() ? EXIT_SUCCESS : EXIT_FAILURE;
}

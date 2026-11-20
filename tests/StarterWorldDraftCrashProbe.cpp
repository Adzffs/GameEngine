#include "../src/Content/Edit/StarterWorldDraftFile.h"
#include <iostream>
#include <string>

namespace { std::string pausePhase; }

void GameEngineDraftTestPause(std::string_view phase)
{
    if (phase == pausePhase)
    {
        std::cout << "paused:" << phase << std::endl;
        std::cin.get(); // The test parent kills this process; no cleanup runs.
    }
}

int main(int argc, char **argv)
{
    if (argc != 4) return 2;
    pausePhase = argv[3];
    try
    {
        const auto bytes = StarterWorldDraftFile::Read(argv[1]);
        StarterWorldDraftFile::PublishNew(argv[1], StarterWorldDraftFile::Digest(bytes), bytes, argv[2]);
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

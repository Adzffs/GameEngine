#include "../src/Content/Publication/WorldPublication.h"
#include <iostream>
#include <string>
namespace { std::string stop; }
void GameEnginePublicationTestPause(std::string_view phase)
{
    if (phase == stop)
    {
        std::cout << "paused:" << phase << std::endl;
        std::cin.get();
    }
}
int main(int argc, char **argv)
{
    if (argc != 5) return 2;
    stop = argv[4];
    try
    {
        WorldPublication::Publish(argv[1], WorldPublication::ValidateDraft(argv[1]), argv[2], argv[3]);
        return 0;
    }
    catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}

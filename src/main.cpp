#include <iostream>
#include "Core/Engine.h"

int main()
{
    std::cout << "Starting Game Engine..." << std::endl;

    Engine engine;

    engine.Run();

    return 0;
}
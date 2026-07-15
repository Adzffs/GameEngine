#include "World.h"
#include <iostream>

World::World()
    : player(1)
{
}

void World::Update()
{
    std::cout << "Updating World..." << std::endl;

    player.Update();
}
#include "World.h"
#include <iostream>

World::World()
{

    entityManager.CreateEntity();
    entityManager.CreateEntity();
    entityManager.CreateEntity();
}

void World::Update()
{
    std::cout << "Updating World..." << std::endl;

    entityManager.Update();
}
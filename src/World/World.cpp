#include "World.h"
#include <iostream>

World::World()
    : map(100, 100)
{

    entityManager.CreateEntity(EntityType::PLAYER);
    entityManager.CreateEntity(EntityType::NPC);
    entityManager.CreateEntity(EntityType::RESOURCE);
}

void World::Update()
{
    std::cout << "Updating World..." << std::endl;

    entityManager.Update();
}
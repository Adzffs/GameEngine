#include "World.h"
#include <iostream>

World::World()
    : map(100, 100)
{

    entityManager.CreateEntity(EntityType::PLAYER);
    entityManager.CreateEntity(EntityType::NPC);
    entityManager.CreateEntity(EntityType::RESOURCE);
    objectManager.CreateResource(5, 5);
    objectManager.CreateResource(10, 10);
}

void World::Update()
{
    std::cout << "Updating World..." << std::endl;

    entityManager.Update(map);
    objectManager.Update();
}
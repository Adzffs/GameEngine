#include "EntityManager.h"
#include <iostream>

EntityManager::EntityManager()
{
}

void EntityManager::CreateEntity()
{
    Entity entity(nextID);

    entities.push_back(entity);

    nextID++;
}

void EntityManager::Update()
{
    std::cout
        << "Updating "
        << entities.size()
        << " Entities..."
        << std::endl;

    for (auto &entity : entities)
    {
        entity.Update();
    }
}
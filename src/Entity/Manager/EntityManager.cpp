#include "EntityManager.h"
#include <iostream>

EntityManager::EntityManager()
{
}

void EntityManager::CreateEntity(EntityType type)
{
    Entity entity(nextID, type);

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
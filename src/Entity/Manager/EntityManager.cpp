#include "EntityManager.h"
#include <iostream>
#include "../../Movement/Movement.h"

EntityManager::EntityManager()
{
}

void EntityManager::CreateEntity(EntityType type)
{
    Entity entity(nextID, type);

    entities.push_back(entity);

    nextID++;
}

void EntityManager::Update(Map &map)
{
    std::cout
        << "Updating "
        << entities.size()
        << " Entities..."
        << std::endl;

    for (auto &entity : entities)
    {
        if (entity.GetType() == EntityType::PLAYER)
        {
            bool moved = Movement::Move(entity, map, 1, 0);

            if (!moved)
            {
                std::cout << "Movement blocked!" << std::endl;
            }
        }

        entity.Update();
    }
}
#include "EntityManager.h"
#include <iostream>
#include "../../Movement/Movement.h"
#include "../../Core/Logger.h"
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
    Logger::Debug(
        "Updating " + std::to_string(entities.size()) + " Entities");

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
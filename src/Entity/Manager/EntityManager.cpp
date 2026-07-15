#include "EntityManager.h"
#include <iostream>
#include "../../Movement/Movement.h"
#include "../../Core/Logger.h"
#include "../../Movement/MovementRequest.h"
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
            // temporary input test

            MovementRequest request = input.GetMovementRequest();

            bool moved = Movement::Move(
                entity,
                map,
                request.GetX(),
                request.GetY());

            if (!moved)
            {
                std::cout << "Movement blocked!" << std::endl;
            }
        }

        entity.Update();
    }
}
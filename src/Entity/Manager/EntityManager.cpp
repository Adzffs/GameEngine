#include "EntityManager.h"
#include <iostream>
#include "../../Movement/Movement.h"
#include "../../Core/Logger.h"
#include "../../Movement/MovementRequest.h"
#include "../../Player/Player.h"
#include "../../World/World.h"

EntityManager::EntityManager()
{
}

void EntityManager::CreatePlayer()
{
    entities.push_back(
        std::make_unique<Player>(nextID));

    nextID++;
}

void EntityManager::Update(World &world)
{
    Logger::Debug(
        "Updating " + std::to_string(entities.size()) + " Entities");

    for (auto &entity : entities)
    {
        entity->Update(world);
    }
}
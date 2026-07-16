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
int EntityManager::CreatePlayer()
{
    int playerID = nextID;

    entities.push_back(
        std::make_unique<Player>(playerID));

    nextID++;

    return playerID;
}
int EntityManager::CreateNPC()
{
    int npcID = nextID;

    entities.push_back(
        std::make_unique<NPC>(npcID));

    nextID++;

    return npcID;
}
Entity *EntityManager::GetEntityByID(int id)
{
    for (const auto &entity : entities)
    {
        if (entity->GetID() == id)
        {
            return entity.get();
        }
    }

    return nullptr;
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
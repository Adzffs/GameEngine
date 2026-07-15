#include "World.h"
#include <iostream>
#include "Object/Resource/ResourceNode.h"
#include "../Core/Logger.h"

World::World()
    : map(100, 100)
{

    entityManager.CreateEntity(EntityType::PLAYER);
    entityManager.CreateEntity(EntityType::NPC);
    entityManager.CreateEntity(EntityType::RESOURCE);
}

void World::Update()
{
    Logger::Debug("Updating World");

    entityManager.Update(map);
    objectManager.Update();
}
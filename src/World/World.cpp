#include "World.h"
#include <iostream>
#include "Object/Resource/ResourceNode.h"
#include "../Core/Logger.h"
#include "../Action/Action.h"

World::World()
    : map(100, 100)
{

    entityManager.CreateEntity(EntityType::PLAYER);
    entityManager.CreateEntity(EntityType::NPC);
    entityManager.CreateEntity(EntityType::RESOURCE);
    actionManager.AddAction(
        Action("Chopping Tree", 5));
    entityManager.CreatePlayer();
}

void World::Update()
{
    Logger::Debug("Updating World");
    actionManager.Update();
    entityManager.Update(map);
    objectManager.Update();
}
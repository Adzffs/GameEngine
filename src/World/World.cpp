#include "World.h"
#include <iostream>
#include "Object/Resource/ResourceNode.h"
#include "../Core/Logger.h"
#include "../Action/Action.h"

World::World()
    : map(100, 100)
{
    entityManager.CreatePlayer();
    objectManager.CreateResource(5, 5);

    actionManager.AddAction(
        Action("Chopping Tree", 5));
}

void World::Update()
{
    Logger::Debug("Updating World");
    actionManager.Update();
    entityManager.Update(*this);
    objectManager.Update();
}

Map &World::GetMap()
{
    return map;
}
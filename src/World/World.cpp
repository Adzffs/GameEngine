#include "World.h"
#include <iostream>
#include "Object/Resource/ResourceNode.h"
#include "../Core/Logger.h"
#include "../Action/Action.h"
#include "../Movement/Movement.h"

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
    ProcessMovementRequests();
    objectManager.Update();
}

Map &World::GetMap()
{
    return map;
}
void World::QueueMovementRequest(const MovementRequest &request)
{
    movementRequests.push(request);
}
void World::ProcessMovementRequests()
{
    while (!movementRequests.empty())
    {
        MovementRequest request = movementRequests.front();
        movementRequests.pop();

        Entity *entity =
            entityManager.GetEntityByID(request.GetEntityID());

        if (entity == nullptr)
        {
            std::cout
                << "Entity not found: "
                << request.GetEntityID()
                << std::endl;

            continue;
        }
        Movement::Move(
            *entity,
            map,
            request.GetX(),
            request.GetY());
    }
}
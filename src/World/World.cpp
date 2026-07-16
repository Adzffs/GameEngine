#include "World.h"
#include <iostream>
#include "Object/Resource/ResourceNode.h"
#include "../Core/Logger.h"
#include "../Action/Action.h"
#include "../Movement/Movement.h"

World::World()
    : map(100, 100)
{
    entityManager.CreateNPC(3, 3);
    CreateResource(5, 5);
}
void World::CreateResource(int x, int y)
{
    objectManager.CreateResource(x, y);

    map.SetTileType(
        x,
        y,
        TileType::TREE);
}
int World::CreatePlayer()
{
    return entityManager.CreatePlayer();
}
Entity *World::GetEntityByID(int id)
{
    return entityManager.GetEntityByID(id);
}
const std::vector<std::unique_ptr<Entity>> &
World::GetEntities() const
{
    return entityManager.GetEntities();
}

const std::vector<ResourceNode> &
World::GetResources() const
{
    return objectManager.GetResources();
}
void World::Update()
{
    Logger::Debug("Updating World");
    actionManager.Update();
    ProcessMovementDestinationRequests();
    ProcessActiveMovementPaths();
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
void World::QueueMovementDestination(
    const MovementDestinationRequest &request)
{
    movementDestinationRequests.push(request);
}
void World::ProcessMovementDestinationRequests()
{
    while (!movementDestinationRequests.empty())
    {
        MovementDestinationRequest request =
            movementDestinationRequests.front();

        movementDestinationRequests.pop();

        Entity *entity =
            entityManager.GetEntityByID(
                request.GetEntityID());

        if (entity == nullptr)
        {
            std::cout
                << "Cannot calculate path: Entity "
                << request.GetEntityID()
                << " was not found."
                << std::endl;

            continue;
        }

        int startX =
            entity->GetPosition().GetX();

        int startY =
            entity->GetPosition().GetY();

        std::vector<PathStep> path =
            pathfinder.FindPath(
                map,
                startX,
                startY,
                request.GetDestinationX(),
                request.GetDestinationY());

        if (path.empty())
        {
            if (startX == request.GetDestinationX() &&
                startY == request.GetDestinationY())
            {
                std::cout
                    << "Entity is already at destination."
                    << std::endl;
            }
            else
            {
                std::cout
                    << "No path found to destination: ("
                    << request.GetDestinationX()
                    << ", "
                    << request.GetDestinationY()
                    << ")"
                    << std::endl;
            }

            continue;
        }
        const PathStep &finalStep = path.back();

        if (finalStep.x != request.GetDestinationX() ||
            finalStep.y != request.GetDestinationY())
        {
            std::cout
                << "Destination was blocked or unreachable."
                << " Using nearest reachable tile: ("
                << finalStep.x
                << ", "
                << finalStep.y
                << ")"
                << std::endl;
        }

        std::cout
            << "Path found for Entity ID: "
            << request.GetEntityID()
            << " Steps: "
            << path.size()
            << std::endl;

        std::queue<PathStep> storedPath;

        for (const PathStep &step : path)
        {
            storedPath.push(step);
        }

        activeMovementPaths[request.GetEntityID()] =
            storedPath;

        std::cout
            << "Path stored for Entity ID: "
            << request.GetEntityID()
            << std::endl;
    }
}
void World::ProcessActiveMovementPaths()
{
    auto pathIterator =
        activeMovementPaths.begin();

    while (pathIterator !=
           activeMovementPaths.end())
    {
        int entityID = pathIterator->first;

        std::queue<PathStep> &path =
            pathIterator->second;

        Entity *entity =
            entityManager.GetEntityByID(entityID);

        if (entity == nullptr || path.empty())
        {
            pathIterator =
                activeMovementPaths.erase(
                    pathIterator);

            continue;
        }

        PathStep nextStep = path.front();

        int changeX =
            nextStep.x -
            entity->GetPosition().GetX();

        int changeY =
            nextStep.y -
            entity->GetPosition().GetY();

        Movement::Move(
            *entity,
            map,
            changeX,
            changeY);

        path.pop();

        std::cout
            << "Entity "
            << entityID
            << " moved to: ("
            << entity->GetPosition().GetX()
            << ", "
            << entity->GetPosition().GetY()
            << ")"
            << std::endl;

        if (path.empty())
        {
            std::cout
                << "Entity "
                << entityID
                << " reached its destination."
                << std::endl;

            pathIterator =
                activeMovementPaths.erase(
                    pathIterator);
        }
        else
        {
            ++pathIterator;
        }
    }
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
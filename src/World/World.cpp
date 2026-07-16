#include "World.h"
#include <iostream>
#include "Object/Resource/ResourceNode.h"
#include "../Core/Logger.h"
#include "../Action/Action.h"
#include "../Movement/Movement.h"
#include <cstdlib>
#include "../Player/Player.h"
#include "../Inventory/ItemType.h"
#include "../Skills/SkillType.h"

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
ResourceNode *World::GetResourceAt(
    int x,
    int y)
{
    return objectManager.GetResourceAt(x, y);
}
void World::QueueResourceInteraction(
    int entityID,
    int resourceID)
{
    pendingResourceInteractions[entityID] =
        resourceID;
}

void World::ClearPendingResourceInteraction(
    int entityID)
{
    pendingResourceInteractions.erase(entityID);
}
void World::Update()
{
    Logger::Debug("Updating World");

    ProcessMovementDestinationRequests();
    ProcessActiveMovementPaths();
    ProcessResourceInteractions();

    objectManager.Update();

    std::vector<Action> completedActions =
        actionManager.Update();

    ProcessCompletedActions(completedActions);

    entityManager.Update(*this);
    ProcessMovementRequests();
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
void World::ProcessResourceInteractions()
{
    auto interactionIterator =
        pendingResourceInteractions.begin();

    while (interactionIterator !=
           pendingResourceInteractions.end())
    {
        int entityID =
            interactionIterator->first;

        int resourceID =
            interactionIterator->second;

        Entity *entity =
            entityManager.GetEntityByID(entityID);

        ResourceNode *resource =
            objectManager.GetResourceByID(resourceID);

        if (entity == nullptr ||
            resource == nullptr)
        {
            interactionIterator =
                pendingResourceInteractions.erase(
                    interactionIterator);

            continue;
        }
        if (!resource->IsActive())
        {
            std::cout
                << "Resource "
                << resourceID
                << " is currently depleted."
                << std::endl;

            interactionIterator =
                pendingResourceInteractions.erase(
                    interactionIterator);

            continue;
        }

        int distanceX = std::abs(
            entity->GetPosition().GetX() -
            resource->GetX());

        int distanceY = std::abs(
            entity->GetPosition().GetY() -
            resource->GetY());

        bool isAdjacent =
            distanceX <= 1 &&
            distanceY <= 1;

        if (!isAdjacent)
        {
            ++interactionIterator;
            continue;
        }

        if (!actionManager.HasActionForEntity(entityID))
        {
            actionManager.AddAction(
                Action(
                    "Chopping Tree",
                    5,
                    entityID,
                    resourceID));

            std::cout
                << "Entity "
                << entityID
                << " started chopping Resource "
                << resourceID
                << "."
                << std::endl;
        }

        interactionIterator =
            pendingResourceInteractions.erase(
                interactionIterator);
    }
}
void World::ProcessCompletedActions(
    const std::vector<Action> &completedActions)
{
    for (const Action &action : completedActions)
    {
        ResourceNode *resource =
            objectManager.GetResourceByID(
                action.GetTargetID());

        if (resource == nullptr)
        {
            continue;
        }

        if (!resource->IsActive())
        {
            continue;
        }

        Entity *entity =
            entityManager.GetEntityByID(
                action.GetEntityID());

        Player *player =
            dynamic_cast<Player *>(entity);

        if (player == nullptr)
        {
            continue;
        }

        bool itemAdded =
            player->GetInventory().AddItem(
                ItemType::LOG,
                1);

        if (!itemAdded)
        {
            std::cout
                << "Player "
                << player->GetID()
                << " inventory is full."
                << std::endl;

            continue;
        }

        int previousLevel =
            player->GetSkills()
                .GetSkill(SkillType::WOODCUTTING)
                .GetLevel();

        player->GetSkills().AddXP(
            SkillType::WOODCUTTING,
            25);

        const Skill &woodcutting =
            player->GetSkills()
                .GetSkill(SkillType::WOODCUTTING);

        std::cout
            << "Player "
            << player->GetID()
            << " received 1 log. Total logs: "
            << player->GetInventory().GetItemAmount(
                   ItemType::LOG)
            << std::endl;

        std::cout
            << "Woodcutting XP: "
            << woodcutting.GetXP()
            << " Level: "
            << woodcutting.GetLevel()
            << std::endl;

        if (woodcutting.GetLevel() > previousLevel)
        {
            std::cout
                << "Woodcutting level increased to "
                << woodcutting.GetLevel()
                << "!"
                << std::endl;
        }

        resource->Deplete();
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
#include "MovementSystem.h"

#include "Movement.h"
#include "../Entity/Manager/EntityManager.h"
#include "../Pathfinding/Pathfinder.h"
#include "../World/Map.h"

#include <algorithm>

bool MovementSystem::QueueDestination(
    int entityID,
    const Position &destination,
    const EntityManager &entityManager,
    const Map &map)
{
    if (entityManager.GetEntityByID(entityID) == nullptr ||
        !map.IsInBounds(destination.GetX(), destination.GetY()))
    {
        return false;
    }

    queuedDestinations.push_back(
        MovementDestination{entityID, destination});
    return true;
}

bool MovementSystem::CancelMovement(int entityID)
{
    const std::size_t previousQueuedCount = queuedDestinations.size();
    std::erase_if(
        queuedDestinations,
        [entityID](const MovementDestination &request)
        {
            return request.entityID == entityID;
        });

    return activeMovementPaths.erase(entityID) != 0 ||
           queuedDestinations.size() != previousQueuedCount;
}

bool MovementSystem::HasMovement(int entityID) const
{
    if (activeMovementPaths.contains(entityID))
    {
        return true;
    }

    return std::any_of(
        queuedDestinations.begin(),
        queuedDestinations.end(),
        [entityID](const MovementDestination &request)
        {
            return request.entityID == entityID;
        });
}

std::optional<Position> MovementSystem::GetDestination(int entityID) const
{
    for (auto iterator = queuedDestinations.rbegin();
         iterator != queuedDestinations.rend();
         ++iterator)
    {
        if (iterator->entityID == entityID)
        {
            return iterator->destination;
        }
    }

    auto iterator = activeMovementPaths.find(entityID);
    if (iterator == activeMovementPaths.end())
    {
        return std::nullopt;
    }
    return iterator->second.requestedDestination;
}

std::size_t MovementSystem::GetActiveMovementCount() const
{
    return activeMovementPaths.size();
}

std::vector<MovementOutcome> MovementSystem::Process(
    EntityManager &entityManager,
    Map &map)
{
    Pathfinder pathfinder;

    while (!queuedDestinations.empty())
    {
        MovementDestination request = queuedDestinations.front();
        queuedDestinations.pop_front();

        Entity *entity = entityManager.GetEntityByID(request.entityID);
        if (entity == nullptr)
        {
            continue;
        }

        std::vector<PathStep> path = pathfinder.FindPath(
            map,
            entity->GetPosition().GetX(),
            entity->GetPosition().GetY(),
            request.destination.GetX(),
            request.destination.GetY());

        if (path.empty())
        {
            continue;
        }

        ActiveMovementPath activePath{request.destination, {}};
        for (const PathStep &step : path)
        {
            activePath.remainingSteps.emplace_back(step.x, step.y);
        }
        activeMovementPaths.insert_or_assign(
            request.entityID,
            std::move(activePath));
    }

    std::erase_if(
        activeMovementPaths,
        [&entityManager](const auto &entry)
        {
            return entityManager.GetEntityByID(entry.first) == nullptr;
        });

    std::vector<MovementOutcome> outcomes;
    for (const auto &entityOwner : entityManager.GetEntities())
    {
        Entity &entity = *entityOwner;
        const int entityID = entity.GetID();
        auto pathIterator = activeMovementPaths.find(entityID);
        if (pathIterator == activeMovementPaths.end())
        {
            continue;
        }

        ActiveMovementPath &activePath = pathIterator->second;
        const Position previousPosition = entity.GetPosition();

        if (activePath.remainingSteps.empty())
        {
            outcomes.push_back(MovementOutcome{
                entityID,
                MovementOutcomeType::ARRIVED,
                previousPosition,
                previousPosition,
                activePath.requestedDestination});
            activeMovementPaths.erase(pathIterator);
            continue;
        }

        const Position nextStep = activePath.remainingSteps.front();
        const int changeX = nextStep.GetX() - previousPosition.GetX();
        const int changeY = nextStep.GetY() - previousPosition.GetY();

        if (Movement::Move(entity, map, changeX, changeY))
        {
            activePath.remainingSteps.pop_front();
            const bool arrived = activePath.remainingSteps.empty();
            outcomes.push_back(MovementOutcome{
                entityID,
                arrived ? MovementOutcomeType::ARRIVED
                        : MovementOutcomeType::STEP_SUCCEEDED,
                previousPosition,
                entity.GetPosition(),
                activePath.requestedDestination});
            if (arrived)
            {
                activeMovementPaths.erase(pathIterator);
            }
            continue;
        }

        std::vector<PathStep> recalculatedPath = pathfinder.FindPath(
            map,
            entity.GetPosition().GetX(),
            entity.GetPosition().GetY(),
            activePath.requestedDestination.GetX(),
            activePath.requestedDestination.GetY());

        bool validReplacement = !recalculatedPath.empty();
        if (validReplacement)
        {
            const PathStep &firstStep = recalculatedPath.front();
            const int firstChangeX = firstStep.x - entity.GetPosition().GetX();
            const int firstChangeY = firstStep.y - entity.GetPosition().GetY();
            validReplacement =
                (((firstChangeX == -1 || firstChangeX == 1) && firstChangeY == 0) ||
                 (firstChangeX == 0 && (firstChangeY == -1 || firstChangeY == 1))) &&
                map.IsValidPosition(firstStep.x, firstStep.y);
        }

        if (!validReplacement)
        {
            outcomes.push_back(MovementOutcome{
                entityID,
                MovementOutcomeType::PATH_CANCELLED,
                previousPosition,
                entity.GetPosition(),
                activePath.requestedDestination});
            activeMovementPaths.erase(pathIterator);
            continue;
        }

        activePath.remainingSteps.clear();
        for (const PathStep &step : recalculatedPath)
        {
            activePath.remainingSteps.emplace_back(step.x, step.y);
        }
        outcomes.push_back(MovementOutcome{
            entityID,
            MovementOutcomeType::PATH_RECALCULATED,
            previousPosition,
            entity.GetPosition(),
            activePath.requestedDestination});
    }

    return outcomes;
}

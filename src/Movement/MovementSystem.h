#pragma once

#include "MovementOutcome.h"

#include <cstddef>
#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

class EntityManager;
class Map;
struct MovementSystemTestAccess;

class MovementSystem
{
public:
    bool QueueDestination(
        int entityID,
        const Position &destination,
        const EntityManager &entityManager,
        const Map &map);

    bool CancelMovement(int entityID);
    bool HasMovement(int entityID) const;
    std::optional<Position> GetDestination(int entityID) const;
    std::size_t GetActiveMovementCount() const;

    std::vector<MovementOutcome> Process(
        EntityManager &entityManager,
        Map &map);

private:
    struct MovementDestination
    {
        int entityID;
        Position destination;
    };

    struct ActiveMovementPath
    {
        Position requestedDestination;
        std::deque<Position> remainingSteps;
    };

    std::deque<MovementDestination> queuedDestinations;
    std::unordered_map<int, ActiveMovementPath> activeMovementPaths;

    friend struct MovementSystemTestAccess;
};

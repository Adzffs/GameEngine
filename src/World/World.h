#pragma once
#include <queue>
#include "../Movement/MovementRequest.h"
#include "../Entity/Manager/EntityManager.h"
#include "Map.h"
#include "Object/Manager/ObjectManager.h"
#include "../Action/ActionManager.h"
#include "../Movement/MovementDestinationRequest.h"
#include "../Pathfinding/Pathfinder.h"
#include <map>

class World
{
public:
    World();

    void Update();

    Map &GetMap();

    int CreatePlayer();
    Entity *GetEntityByID(int id);

    void QueueMovementRequest(
        const MovementRequest &request);
    void QueueMovementDestination(
        const MovementDestinationRequest &request);

private:
    EntityManager entityManager;

    ObjectManager objectManager;

    ActionManager actionManager;

    Map map;

    void ProcessMovementRequests();
    void ProcessMovementDestinationRequests();
    void ProcessActiveMovementPaths();

    Pathfinder pathfinder;

    std::queue<MovementRequest> movementRequests;

    std::queue<MovementDestinationRequest>
        movementDestinationRequests;

    std::map<int, std::queue<PathStep>>
        activeMovementPaths;
};
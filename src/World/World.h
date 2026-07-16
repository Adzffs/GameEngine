#pragma once
#include <queue>
#include "../Movement/MovementRequest.h"
#include "../Entity/Manager/EntityManager.h"
#include "Map.h"
#include "Object/Manager/ObjectManager.h"
#include "../Action/ActionManager.h"

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

private:
    EntityManager entityManager;

    ObjectManager objectManager;

    ActionManager actionManager;

    Map map;

    void ProcessMovementRequests();

    std::queue<MovementRequest> movementRequests;
};
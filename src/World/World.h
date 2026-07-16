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
    const std::vector<std::unique_ptr<Entity>> &
    GetEntities() const;

    const std::vector<ResourceNode> &
    GetResources() const;
    ResourceNode *GetResourceAt(int x, int y);
    void QueueMovementRequest(
        const MovementRequest &request);
    void QueueMovementDestination(
        const MovementDestinationRequest &request);
    void QueueResourceInteraction(
        int entityID,
        int resourceID);

    void ClearPendingResourceInteraction(
        int entityID);

    bool TryEquipInventoryItem(
        int entityID,
        int slotIndex);

    bool TryUnequipWeapon(
        int entityID);

private:
    EntityManager entityManager;

    ObjectManager objectManager;

    ActionManager actionManager;

    Map map;

    Pathfinder pathfinder;

    void CreateResource(int x, int y);

    void ProcessMovementRequests();
    void ProcessMovementDestinationRequests();
    void ProcessActiveMovementPaths();
    void ProcessResourceInteractions();
    void ProcessCompletedActions(
        const std::vector<Action> &completedActions);
    std::queue<MovementRequest> movementRequests;

    std::queue<MovementDestinationRequest>
        movementDestinationRequests;

    std::map<int, std::queue<PathStep>>
        activeMovementPaths;

    std::map<int, int> pendingResourceInteractions;
};
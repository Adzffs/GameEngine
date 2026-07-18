#pragma once
#include <queue>
#include "../Movement/MovementRequest.h"
#include "../Entity/Manager/EntityManager.h"
#include "Map.h"
#include "Object/Manager/ObjectManager.h"
#include "../Action/ActionManager.h"
#include "../Action/ActionValidationResult.h"
#include "../Movement/MovementDestinationRequest.h"
#include "../Pathfinding/Pathfinder.h"
#include "../Recipe/RecipeType.h"
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

    const std::vector<CraftingStation> &
    GetStations() const;

    CraftingStation *GetStationAt(
        int x,
        int y);

    void QueueMovementRequest(
        const MovementRequest &request);
    void QueueMovementDestination(
        const MovementDestinationRequest &request);
    void QueueResourceInteraction(
        int entityID,
        int resourceID);
    void QueueStationInteraction(
        int entityID,
        int stationID);
    void CancelActionsForEntity(
        int entityID,
        ActionCancelReason reason =
            ActionCancelReason::NONE);
    void CloseStationInteraction(
        int entityID);
    void ClearPendingResourceInteraction(
        int entityID);
    void ClearPendingStationInteraction(
        int entityID);

    bool ConsumeOpenedStation(
        int entityID,
        StationType &stationType);

    const Action *GetActionForEntity(
        int entityID) const;

    bool TryEquipInventoryItem(
        int entityID,
        int slotIndex);

    bool TryUnequipWeapon(
        int entityID);

    bool TryStartRecipeAction(
        int entityID,
        RecipeType recipeType);

private:
    EntityManager entityManager;

    ObjectManager objectManager;

    ActionManager actionManager;

    Map map;

    Pathfinder pathfinder;

    void CreateResource(
        ResourceType resourceType,
        int x,
        int y);

    void CreateStation(
        StationType stationType,
        int x,
        int y);

    void ProcessMovementRequests();
    void ProcessMovementDestinationRequests();
    void ProcessActiveMovementPaths();
    void ProcessResourceInteractions();
    void ProcessStationInteractions();
    void ProcessCompletedActions(
        const std::vector<Action> &completedActions);
    ActionValidationResult ValidateGatheringAction(
        int entityID,
        int resourceID,
        bool checkInventorySpace);
    bool CanUseStation(
        int entityID,
        StationType requiredStationType);
    std::queue<MovementRequest> movementRequests;

    std::queue<MovementDestinationRequest>
        movementDestinationRequests;

    std::map<int, std::queue<PathStep>>
        activeMovementPaths;

    std::map<int, int> pendingResourceInteractions;
    std::map<int, int> pendingStationInteractions;
    std::map<int, StationType> openedStations;
    std::map<int, int> activeStations;
    std::map<int, RecipeType> activeRecipeLoops;
};
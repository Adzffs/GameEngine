#pragma once
#include <memory>
#include <queue>
#include "../Movement/MovementRequest.h"
#include "../Entity/Manager/EntityManager.h"
#include "Map.h"
#include "Object/Manager/ObjectManager.h"
#include "Event/EntityDiedEvent.h"
#include "../Action/ActionManager.h"
#include "../Action/ActionCancelReason.h"
#include "../Action/ActionValidationResult.h"
#include "../Movement/MovementDestinationRequest.h"
#include "../Pathfinding/Pathfinder.h"
#include "../Recipe/RecipeType.h"
#include <map>
#include "../Combat/CombatService.h"
#include "../Combat/MeleeCombatFeedback.h"
#include "../Stats/CombatRatings.h"
#include <optional>
#include <set>
#include <vector>

class RandomSource;

class World
{
public:
    explicit World(unsigned int combatSeed = 1337U);
    explicit World(std::unique_ptr<RandomSource> combatRandomSource);

    void Update();

    Map &GetMap();

    int CreatePlayer();
    int CreateMonster(
        int x,
        int y,
        const CombatRatings &ratings);
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
    bool QueueMeleeEngagementRequest(
        int attackerEntityID,
        int defenderEntityID,
        int durationTicks);
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

    bool TryStartMeleeAttack(
        int attackerEntityID,
        int defenderEntityID,
        int durationTicks);

    bool TryStartMeleeEngagement(
        int attackerEntityID,
        int defenderEntityID,
        int durationTicks);

    bool HasPendingMeleeEngagement(
        int attackerEntityID) const;

    int GetCurrentTick() const;

    // Temporary global seam used by tests and manual debugging.
    // This is not intended as the long-term per-entity combat event model.
    const std::optional<MeleeAttackResult> &
    GetLastMeleeAttackResult() const;

    const std::map<int, MeleeCombatFeedback> &
    GetMeleeCombatFeedbacks() const;

    const std::vector<EntityDiedEvent> &
    GetEntityDiedEvents() const;

private:
    EntityManager entityManager;

    ObjectManager objectManager;

    ActionManager actionManager;

    CombatService combatService;

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
    void ProcessPendingMeleeInteractions();
    void ProcessCompletedActions(
        const std::vector<Action> &completedActions);
    ActionValidationResult ValidateGatheringAction(
        int entityID,
        int resourceID,
        bool checkInventorySpace);
    ActionValidationResult ValidateRecipeAction(
        int entityID,
        RecipeType recipeType);
    ActionValidationResult ValidateMeleeAttackAction(
        int attackerEntityID,
        int defenderEntityID);
    ActionValidationResult ValidateMeleeStartAction(
        int attackerEntityID,
        int defenderEntityID,
        int durationTicks);
    bool TryStartMeleeAction(
        int attackerEntityID,
        int defenderEntityID,
        int durationTicks,
        bool repeating);
    std::optional<std::pair<int, int>> FindMeleeApproachTile(
        int attackerX,
        int attackerY,
        int defenderX,
        int defenderY);
    void CancelGatheringForToolChange(
        int entityID);
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
    struct PendingMeleeInteraction
    {
        int attackerEntityID;
        int defenderEntityID;
        int durationTicks;
        std::optional<std::pair<int, int>> destination;
    };

    std::map<int, PendingMeleeInteraction>
        pendingMeleeInteractions;
    std::map<int, StationType> openedStations;
    std::map<int, int> activeStations;

    std::optional<MeleeAttackResult>
        lastMeleeAttackResult;

    int currentTick = 0;

    std::map<int, MeleeCombatFeedback>
        meleeCombatFeedbacks;

    static constexpr int MeleeCombatFeedbackLifetimeTicks = 3;

    void UpdateMeleeCombatFeedback();
    void RecordMeleeCombatFeedback(
        int attackerEntityID,
        int defenderEntityID,
        const MeleeAttackResult &result);

    static constexpr int DefaultMonsterAttackDurationTicks = 5;

    void TryStartMonsterRetaliation(
        int monsterEntityID,
        int playerEntityID);

    void CancelMeleeActionsTargetingEntity(
        int targetEntityID,
        ActionCancelReason reason =
            ActionCancelReason::ENTITY_DIED);

    void ClearPendingMeleeInteractionsInvolvingEntity(
        int entityID);

    void ClearPendingMovementForEntity(
        int entityID);

    void HandleCombatantDeath(
        int deadEntityID,
        int killerEntityID =
            EntityDiedEvent::InvalidKillerEntityID);

    void RecordEntityDiedEvent(
        int deadEntityID,
        int killerEntityID);

    void ProcessDeadCombatantCleanup();

    std::set<int> processedDeathEntityIDs;
    std::vector<EntityDiedEvent> entityDiedEvents;
};

#pragma once
#include <memory>
#include <queue>
#include "../Movement/MovementRequest.h"
#include "../Entity/Manager/EntityManager.h"
#include "Map.h"
#include "Object/Manager/ObjectManager.h"
#include "Event/ActionLifecycleEvent.h"
#include "Event/EntityDiedEvent.h"
#include "../Action/ActionManager.h"
#include "../Action/ActionCancelReason.h"
#include "../Action/ActionValidationResult.h"
#include "../Movement/MovementDestinationRequest.h"
#include "../Pathfinding/Pathfinder.h"
#include "../Equipment/EquipmentSlotType.h"
#include "../Recipe/RecipeType.h"
#include <map>
#include "../Combat/CombatService.h"
#include "../Combat/MeleeCombatFeedback.h"
#include "../Reward/ItemReward.h"
#include "../Reward/RewardTableRoller.h"
#include "../Reward/RewardTableType.h"
#include "../Stats/CombatRatings.h"
#include "../Entity/Monster/MonsterAggressionDefinition.h"
#include "../StatusEffect/StatusEffectDefinition.h"
#include "../StatusEffect/StatusEffectType.h"
#include "../Entity/Monster/MonsterRespawnDefinition.h"
#include <optional>
#include <set>
#include <string>
#include <vector>
#include "../Command/CommandProcessingResult.h"
#include "../Command/ServerCommandQueue.h"

class RandomSource;
class Monster;
struct WorldTestAccess;

class World
{
public:
    World();
    explicit World(
        unsigned int combatSeed,
        unsigned int rewardSeed = 7331U,
        unsigned int gatheringSeed = 9001U);
    explicit World(std::unique_ptr<RandomSource> combatRandomSource);
    World(
        std::unique_ptr<RandomSource> combatRandomSource,
        std::unique_ptr<RandomSource> rewardRandomSource);
    World(
        std::unique_ptr<RandomSource> combatRandomSource,
        std::unique_ptr<RandomSource> rewardRandomSource,
        std::unique_ptr<RandomSource> gatheringRandomSource);

    void Update();

    std::uint64_t EnqueueCommand(
        ServerCommandData command);

    const std::vector<CommandProcessingResult> &
    GetCommandProcessingResults() const;

    Map &GetMap();

    int CreatePlayer();
    int CreateMonster(
        int x,
        int y,
        const CombatRatings &ratings,
        RewardTableType rewardTableType =
            RewardTableType::NONE,
        std::optional<MonsterRespawnDefinition> respawnDefinition =
            std::nullopt,
        std::optional<MonsterAggressionDefinition> aggressionDefinition =
            std::nullopt);
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

    bool TryConsumeFood(
        int playerEntityID,
        int inventorySlotIndex);

    bool TryUnequipItem(
        int entityID,
        EquipmentSlotType equipmentSlot);

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

    bool TryApplyStatusEffect(
        int playerEntityID,
        const StatusEffectDefinition &definition);

    bool TryRemoveStatusEffect(
        int playerEntityID,
        StatusEffectType type);

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

    const std::vector<ActionLifecycleEvent> &
    GetActionLifecycleEvents() const;

    int GetScheduledMonsterRespawnCount() const;
    bool HasScheduledMonsterRespawn(
        int monsterEntityID) const;
    std::optional<int> GetScheduledMonsterRespawnTick(
        int monsterEntityID) const;

private:
    friend struct WorldTestAccess;

    EntityManager entityManager;

    ObjectManager objectManager;

    ActionManager actionManager;

    CombatService combatService;

    std::unique_ptr<RandomSource>
        rewardRandomSource;

    std::unique_ptr<RandomSource>
        gatheringRandomSource;

    std::unique_ptr<RewardTableRoller>
        rewardTableRoller;

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
    void ProcessQueuedCommands();
    void ProcessMovementDestinationRequests();
    void ProcessActiveMovementPaths();
    void ProcessResourceInteractions();
    void ProcessStationInteractions();
    void ProcessPendingMeleeInteractions();
    void ProcessCompletedActions(
        const std::vector<Action> &completedActions);
    void ProcessAggressiveMonsters();
    void PublishActionLifecycleEvents();
    void RecordActionStartedEvent(
        const ActionStartedSnapshot &snapshot);
    void RecordActionCompletedEvent(
        const Action &action);
    void RecordActionCancelledEvent(
        const CancelledActionSnapshot &snapshot);
    void StartAction(
        const Action &action);
    void RestartAction(
        const Action &action);
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
    bool IsValidMonsterAggressionTarget(
        const Monster &monster,
        int targetEntityID);
    bool IsMonsterOutsideLeash(
        const Monster &monster,
        const MonsterAggressionDefinition &aggressionDefinition);
    int SelectMonsterAggressionTargetEntityID(
        const Monster &monster,
        const MonsterAggressionDefinition &aggressionDefinition);
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

    struct ActiveMovementPath
    {
        PathStep destination;
        std::queue<PathStep> remainingSteps;
    };

    std::map<int, ActiveMovementPath>
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

    std::vector<ActionLifecycleEvent>
        pendingActionLifecycleEvents;

    std::vector<ActionLifecycleEvent>
        publishedActionLifecycleEvents;

    ServerCommandQueue serverCommandQueue;

    std::vector<CommandProcessingResult>
        pendingCommandProcessingResults;

    std::vector<CommandProcessingResult>
        publishedCommandProcessingResults;

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
    void ProcessEntityDeathRewards();
    void TickPlayerStatusEffects();
    void ScheduleMonsterRespawnsFromDeathEvents();
    void ProcessDueMonsterRespawns();
    bool TryCalculateRespawnTick(
        int delayTicks,
        int &respawnTick) const;
    bool ScheduleMonsterRespawn(
        int monsterEntityID,
        int respawnTick);
    void ExecuteMonsterRespawn(
        Monster &monster);
    void ClearCombatFeedbackInvolvingEntity(
        int entityID);

    static bool TryBuildLootReceiptMessage(
        const std::vector<ItemReward> &rewards,
        std::string &message);

    std::set<int> processedDeathEntityIDs;
    std::vector<EntityDiedEvent> entityDiedEvents;
    std::map<int, int>
        scheduledMonsterRespawnTicksByEntityID;
    std::map<int, std::set<int>>
        scheduledMonsterRespawnEntityIDsByTick;

    std::set<int>
        respawnedMonsterEntityIDsThisTick;
};

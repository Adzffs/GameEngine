#include "World.h"
#include <iostream>
#include "Distance.h"
#include "Object/Resource/ResourceNode.h"
#include "../Core/Logger.h"
#include "../Action/Action.h"
#include "../Movement/Movement.h"
#include <cstdlib>
#include "../Player/Player.h"
#include "../Entity/Monster/Monster.h"
#include "../Inventory/ItemType.h"
#include "../Inventory/ItemAmount.h"
#include "../Skills/SkillType.h"
#include "../Equipment/EquipmentSlotType.h"
#include "../Item/ItemDatabase.h"
#include "../Item/ToolType.h"
#include <string>
#include "Object/Resource/ResourceDatabase.h"
#include "Development/DevelopmentWorldContent.h"
#include "../Core/SeededRandom.h"
#include "../Recipe/RecipeSystem.h"
#include "../Recipe/RecipeDatabase.h"
#include "../Reward/RewardTableRegistry.h"
#include "../Combat/Combatant.h"
#include "../Combat/MeleeCombatFeedback.h"
#include "../Requirement/RequirementEvaluator.h"
#include "../StatusEffect/StatusEffectValidation.h"
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>
#include <set>
#include <algorithm>
#include <random>
#include <type_traits>

namespace
{
    constexpr std::array<std::pair<int, int>, 8>
        MeleeAdjacentOffsets{{{0, -1},
                              {1, 0},
                              {0, 1},
                              {-1, 0},
                              {1, -1},
                              {1, 1},
                              {-1, 1},
                              {-1, -1}}};

    Combatant *TryGetCombatant(Entity *entity)
    {
        if (entity == nullptr)
        {
            return nullptr;
        }

        return dynamic_cast<Combatant *>(entity);
    }

    bool IsValidEquipmentSlot(EquipmentSlotType equipmentSlot)
    {
        switch (equipmentSlot)
        {
        case EquipmentSlotType::HEAD:
        case EquipmentSlotType::BODY:
        case EquipmentSlotType::LEGS:
        case EquipmentSlotType::WEAPON:
        case EquipmentSlotType::SHIELD:
            return true;

        case EquipmentSlotType::NONE:
        case EquipmentSlotType::COUNT:
        default:
            return false;
        }
    }

    std::unique_ptr<RandomSource>
    CreateProductionRandomSource()
    {
        return std::make_unique<SeededRandom>(
            std::random_device{}());
    }
}

World::World()
    : World(
          CreateProductionRandomSource(),
          CreateProductionRandomSource(),
          CreateProductionRandomSource())
{
}

World::World(
    unsigned int combatSeed,
    unsigned int rewardSeed,
    unsigned int gatheringSeed)
    : World(
          std::make_unique<SeededRandom>(combatSeed),
          std::make_unique<SeededRandom>(rewardSeed),
          std::make_unique<SeededRandom>(gatheringSeed))
{
}

World::World(std::unique_ptr<RandomSource> combatRandomSource)
    : World(
          std::move(combatRandomSource),
          CreateProductionRandomSource(),
          CreateProductionRandomSource())
{
}

World::World(
    std::unique_ptr<RandomSource> combatRandomSource,
    std::unique_ptr<RandomSource> rewardRandomSource)
    : World(
          std::move(combatRandomSource),
          std::move(rewardRandomSource),
          CreateProductionRandomSource())
{
}

World::World(
    std::unique_ptr<RandomSource> combatRandomSource,
    std::unique_ptr<RandomSource> rewardRandomSource,
    std::unique_ptr<RandomSource> gatheringRandomSource)
    : combatService(std::move(combatRandomSource)),
      rewardRandomSource(std::move(rewardRandomSource)),
      gatheringRandomSource(std::move(gatheringRandomSource)),
      map(
          DevelopmentWorldContent::MapWidth,
          DevelopmentWorldContent::MapHeight)
{
    if (this->rewardRandomSource == nullptr)
    {
        throw std::invalid_argument("World requires a non-null reward RandomSource");
    }

    if (this->gatheringRandomSource == nullptr)
    {
        throw std::invalid_argument("World requires a non-null gathering RandomSource");
    }

    rewardTableRoller = std::make_unique<RewardTableRoller>(
        *this->rewardRandomSource);

    for (const DevelopmentNpcSpawnDefinition &definition :
         DevelopmentWorldContent::GetStarterNPCSpawns())
    {
        entityManager.CreateNPC(
            definition.spawnX,
            definition.spawnY);
    }

    for (const DevelopmentMonsterSpawnDefinition &definition :
         DevelopmentWorldContent::GetStarterMonsterSpawns())
    {
        CreateMonster(
            definition.spawnX,
            definition.spawnY,
            definition.ratings,
            definition.rewardTableType,
            definition.respawnDefinition,
            definition.aggressionDefinition);
    }

    for (const DevelopmentResourcePlacementDefinition &definition :
         DevelopmentWorldContent::GetStarterResourcePlacements())
    {
        CreateResource(
            definition.resourceType,
            definition.x,
            definition.y);
    }

    for (const DevelopmentStationPlacementDefinition &definition :
         DevelopmentWorldContent::GetStarterStationPlacements())
    {
        CreateStation(
            definition.stationType,
            definition.x,
            definition.y);
    }
}
void World::CreateResource(
    ResourceType resourceType,
    int x,
    int y)
{
    objectManager.CreateResource(
        resourceType,
        x,
        y);

    map.SetTileType(
        x,
        y,
        TileType::TREE);
}

void World::CreateStation(
    StationType stationType,
    int x,
    int y)
{
    objectManager.CreateStation(
        stationType,
        x,
        y);

    map.SetTileType(
        x,
        y,
        TileType::STATION);
}
int World::CreatePlayer()
{
    return entityManager.CreatePlayer();
}

int World::CreateMonster(
    int x,
    int y,
    const CombatRatings &ratings,
    RewardTableType rewardTableType,
    std::optional<MonsterRespawnDefinition> respawnDefinition,
    std::optional<MonsterAggressionDefinition> aggressionDefinition)
{
    return entityManager.CreateMonster(
        x,
        y,
        ratings,
        rewardTableType,
        respawnDefinition,
        aggressionDefinition);
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

const std::vector<CraftingStation> &
World::GetStations() const
{
    return objectManager.GetStations();
}

CraftingStation *World::GetStationAt(
    int x,
    int y)
{
    return objectManager.GetStationAt(
        x,
        y);
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
    Entity *entity =
        entityManager.GetEntityByID(entityID);

    Player *player =
        dynamic_cast<Player *>(entity);

    if (player != nullptr &&
        !player->IsAlive())
    {
        return;
    }

    pendingMeleeInteractions.erase(entityID);

    pendingResourceInteractions[entityID] =
        resourceID;
}

void World::QueueStationInteraction(
    int entityID,
    int stationID)
{
    Entity *entity =
        entityManager.GetEntityByID(entityID);

    Player *player =
        dynamic_cast<Player *>(entity);

    if (player != nullptr &&
        !player->IsAlive())
    {
        return;
    }

    pendingMeleeInteractions.erase(entityID);

    pendingStationInteractions[entityID] =
        stationID;
}

bool World::QueueMeleeEngagementRequest(
    int attackerEntityID,
    int defenderEntityID,
    int durationTicks)
{
    if (durationTicks < 1)
    {
        Logger::Game(
            "Melee attack duration must be at least one tick");

        return false;
    }

    if (attackerEntityID == defenderEntityID)
    {
        Logger::Game(
            "You cannot target yourself");

        return false;
    }

    Entity *attackerEntity =
        entityManager.GetEntityByID(
            attackerEntityID);

    Player *attacker =
        dynamic_cast<Player *>(attackerEntity);

    if (attacker == nullptr)
    {
        Logger::Game(
            "Player could not be found");

        return false;
    }

    Entity *defenderEntity =
        entityManager.GetEntityByID(
            defenderEntityID);

    Combatant *defender =
        TryGetCombatant(defenderEntity);

    if (defender == nullptr)
    {
        Logger::Game(
            "The target cannot be engaged in combat");

        return false;
    }

    if (!attacker->IsAlive())
    {
        Logger::Game(
            "Attacker is not alive");

        return false;
    }

    if (!defender->IsAlive())
    {
        Logger::Game(
            "Defender is not alive");

        return false;
    }

    int attackerX = attackerEntity->GetPosition().GetX();
    int attackerY = attackerEntity->GetPosition().GetY();
    int defenderX = defenderEntity->GetPosition().GetX();
    int defenderY = defenderEntity->GetPosition().GetY();

    std::optional<std::pair<int, int>> destination =
        FindMeleeApproachTile(
            attackerX,
            attackerY,
            defenderX,
            defenderY);

    bool isAdjacent =
        std::abs(attackerX - defenderX) <= 1 &&
        std::abs(attackerY - defenderY) <= 1;

    if (!isAdjacent && !destination.has_value())
    {
        Logger::Game(
            "No reachable adjacent tile exists");

        return false;
    }

    CancelActionsForEntity(
        attackerEntityID,
        ActionCancelReason::NEW_ACTION_STARTED);

    pendingResourceInteractions.erase(attackerEntityID);
    pendingStationInteractions.erase(attackerEntityID);

    pendingMeleeInteractions[attackerEntityID] =
        PendingMeleeInteraction{
            attackerEntityID,
            defenderEntityID,
            durationTicks,
            destination};

    activeMovementPaths.erase(attackerEntityID);

    if (isAdjacent)
    {
        bool started = TryStartMeleeEngagement(
            attackerEntityID,
            defenderEntityID,
            durationTicks);

        pendingMeleeInteractions.erase(
            attackerEntityID);

        return started;
    }

    movementDestinationRequests.push(
        MovementDestinationRequest(
            attackerEntityID,
            destination->first,
            destination->second));

    return true;
}

void World::CancelActionsForEntity(
    int entityID,
    ActionCancelReason reason)
{
    std::optional<CancelledActionSnapshot> cancelledAction =
        actionManager.CancelActionsForEntity(
            entityID,
            reason);

    if (cancelledAction.has_value())
    {
        RecordActionCancelledEvent(
            cancelledAction.value());
    }
}

void World::CancelGatheringForToolChange(
    int entityID)
{
    const Action *action =
        actionManager.GetActionForEntity(
            entityID);

    if (action != nullptr &&
        action->GetType() ==
            ActionType::GATHERING)
    {
        CancelActionsForEntity(
            entityID,
            ActionCancelReason::INVALID_TOOL);
    }

    pendingResourceInteractions.erase(
        entityID);
}

void World::ClearPendingResourceInteraction(
    int entityID)
{
    pendingResourceInteractions.erase(entityID);
}

void World::ClearPendingStationInteraction(
    int entityID)
{
    pendingStationInteractions.erase(entityID);
}

bool World::HasPendingMeleeEngagement(
    int entityID) const
{
    return pendingMeleeInteractions.find(entityID) !=
           pendingMeleeInteractions.end();
}

void World::ProcessPendingMeleeInteractions()
{
    auto interactionIterator =
        pendingMeleeInteractions.begin();

    while (interactionIterator !=
           pendingMeleeInteractions.end())
    {
        PendingMeleeInteraction request =
            interactionIterator->second;

        Entity *attackerEntity =
            entityManager.GetEntityByID(
                request.attackerEntityID);

        Entity *defenderEntity =
            entityManager.GetEntityByID(
                request.defenderEntityID);

        Player *attacker =
            dynamic_cast<Player *>(attackerEntity);

        Combatant *defender =
            TryGetCombatant(defenderEntity);

        if (attacker == nullptr ||
            defender == nullptr ||
            !attacker->IsAlive() ||
            !defender->IsAlive())
        {
            interactionIterator =
                pendingMeleeInteractions.erase(
                    interactionIterator);
            continue;
        }

        if (actionManager.HasActionForEntity(
                request.attackerEntityID))
        {
            interactionIterator =
                pendingMeleeInteractions.erase(
                    interactionIterator);
            continue;
        }

        int attackerX = attackerEntity->GetPosition().GetX();
        int attackerY = attackerEntity->GetPosition().GetY();
        int defenderX = defenderEntity->GetPosition().GetX();
        int defenderY = defenderEntity->GetPosition().GetY();

        bool isAdjacent =
            std::abs(attackerX - defenderX) <= 1 &&
            std::abs(attackerY - defenderY) <= 1;

        if (isAdjacent)
        {
            bool started = TryStartMeleeEngagement(
                request.attackerEntityID,
                request.defenderEntityID,
                request.durationTicks);

            interactionIterator =
                pendingMeleeInteractions.erase(
                    interactionIterator);

            if (!started)
            {
                continue;
            }

            continue;
        }

        if (activeMovementPaths.find(
                request.attackerEntityID) !=
            activeMovementPaths.end())
        {
            ++interactionIterator;
            continue;
        }

        interactionIterator =
            pendingMeleeInteractions.erase(
                interactionIterator);
    }
}

const Action *
World::GetActionForEntity(
    int entityID) const
{
    return actionManager
        .GetActionForEntity(entityID);
}

bool World::ConsumeOpenedStation(
    int entityID,
    StationType &stationType)
{
    auto stationIterator =
        openedStations.find(entityID);

    if (stationIterator ==
        openedStations.end())
    {
        return false;
    }

    stationType = stationIterator->second;
    openedStations.erase(stationIterator);

    return true;
}

bool World::CanUseStation(
    int entityID,
    StationType requiredStationType)
{
    auto activeStationIterator =
        activeStations.find(entityID);

    if (activeStationIterator ==
        activeStations.end())
    {
        return false;
    }

    Entity *entity =
        entityManager.GetEntityByID(
            entityID);

    CraftingStation *station =
        objectManager.GetStationByID(
            activeStationIterator->second);

    if (entity == nullptr ||
        station == nullptr)
    {
        activeStations.erase(
            activeStationIterator);

        return false;
    }

    if (station->GetStationType() !=
        requiredStationType)
    {
        return false;
    }

    int distanceX = std::abs(
        entity->GetPosition().GetX() -
        station->GetX());

    int distanceY = std::abs(
        entity->GetPosition().GetY() -
        station->GetY());

    bool isAdjacent =
        distanceX <= 1 && distanceY <= 1;

    if (!isAdjacent)
    {
        activeStations.erase(
            activeStationIterator);

        return false;
    }

    return true;
}

void World::CloseStationInteraction(
    int entityID)
{
    pendingStationInteractions.erase(
        entityID);

    openedStations.erase(entityID);

    activeStations.erase(entityID);
}

bool World::TryStartRecipeAction(
    int entityID,
    RecipeType recipeType)
{
    Entity *entity =
        entityManager.GetEntityByID(
            entityID);

    Player *player =
        dynamic_cast<Player *>(entity);

    if (player == nullptr)
    {
        Logger::Game(
            "Player could not be found");

        return false;
    }

    if (!player->IsAlive())
    {
        Logger::Game(
            "Attacker is not alive");

        return false;
    }

    if (actionManager.HasActionForEntity(
            entityID))
    {
        Logger::Game(
            "You are already performing an action");

        return false;
    }

    ActionValidationResult validation =
        ValidateRecipeAction(
            entityID,
            recipeType);

    if (!validation.valid)
    {
        if (!validation.message.empty())
        {
            Logger::Game(
                validation.message);
        }

        return false;
    }

    const RecipeDefinition &recipe =
        RecipeDatabase::Get(
            recipeType);

    StartAction(
        Action(
            ActionType::RECIPE,
            recipe.GetName(),
            recipe.GetActionDurationTicks(),
            entityID,
            static_cast<int>(
                recipeType),
            true));

    Logger::Game(
        "You begin " +
        recipe.GetName());

    return true;
}

bool World::TryStartMeleeAttack(
    int attackerEntityID,
    int defenderEntityID,
    int durationTicks)
{
    return TryStartMeleeAction(
        attackerEntityID,
        defenderEntityID,
        durationTicks,
        false);
}

bool World::TryStartMeleeEngagement(
    int attackerEntityID,
    int defenderEntityID,
    int durationTicks)
{
    bool started = TryStartMeleeAction(
        attackerEntityID,
        defenderEntityID,
        durationTicks,
        true);

    if (!started)
    {
        return false;
    }

    Entity *attackerEntity =
        entityManager.GetEntityByID(
            attackerEntityID);

    Entity *defenderEntity =
        entityManager.GetEntityByID(
            defenderEntityID);

    Player *attacker =
        dynamic_cast<Player *>(attackerEntity);

    Monster *defender =
        dynamic_cast<Monster *>(defenderEntity);

    if (attacker != nullptr &&
        defender != nullptr)
    {
        TryStartMonsterRetaliation(
            defenderEntityID,
            attackerEntityID);
    }

    return true;
}

ActionValidationResult World::ValidateMeleeStartAction(
    int attackerEntityID,
    int defenderEntityID,
    int durationTicks)
{
    if (durationTicks < 1)
    {
        return {
            false,
            ActionCancelReason::REQUIREMENTS_FAILED,
            "Melee attack duration must be at least one tick"};
    }

    if (actionManager.HasActionForEntity(
            attackerEntityID))
    {
        return {
            false,
            ActionCancelReason::REQUIREMENTS_FAILED,
            "You are already performing an action"};
    }

    return ValidateMeleeAttackAction(
        attackerEntityID,
        defenderEntityID);
}

bool World::TryStartMeleeAction(
    int attackerEntityID,
    int defenderEntityID,
    int durationTicks,
    bool repeating)
{
    ActionValidationResult validation =
        ValidateMeleeStartAction(
            attackerEntityID,
            defenderEntityID,
            durationTicks);

    if (!validation.valid)
    {
        if (!validation.message.empty())
        {
            Logger::Game(
                validation.message);
        }

        return false;
    }

    lastMeleeAttackResult.reset();

    StartAction(
        Action(
            ActionType::MELEE_ATTACK,
            "Melee attack",
            durationTicks,
            attackerEntityID,
            defenderEntityID,
            repeating));

    return true;
}

std::optional<std::pair<int, int>> World::FindMeleeApproachTile(
    int attackerX,
    int attackerY,
    int defenderX,
    int defenderY)
{
    for (const auto &offset : MeleeAdjacentOffsets)
    {
        int candidateX = defenderX + offset.first;
        int candidateY = defenderY + offset.second;

        if (candidateX == defenderX &&
            candidateY == defenderY)
        {
            continue;
        }

        if (!map.IsValidPosition(candidateX, candidateY))
        {
            continue;
        }

        std::vector<PathStep> path = pathfinder.FindPath(
            map,
            attackerX,
            attackerY,
            candidateX,
            candidateY);

        if (path.empty())
        {
            if (attackerX == candidateX &&
                attackerY == candidateY)
            {
                return std::pair<int, int>{
                    candidateX,
                    candidateY};
            }

            continue;
        }

        const PathStep &finalStep = path.back();

        if (finalStep.x != candidateX ||
            finalStep.y != candidateY)
        {
            continue;
        }

        return std::pair<int, int>{
            candidateX,
            candidateY};
    }

    return std::nullopt;
}

const std::optional<MeleeAttackResult> &
World::GetLastMeleeAttackResult() const
{
    return lastMeleeAttackResult;
}

const std::map<int, MeleeCombatFeedback> &
World::GetMeleeCombatFeedbacks() const
{
    return meleeCombatFeedbacks;
}

const std::vector<EntityDiedEvent> &
World::GetEntityDiedEvents() const
{
    return entityDiedEvents;
}

const std::vector<ActionLifecycleEvent> &
World::GetActionLifecycleEvents() const
{
    return publishedActionLifecycleEvents;
}

int World::GetScheduledMonsterRespawnCount() const
{
    return static_cast<int>(
        scheduledMonsterRespawnTicksByEntityID.size());
}

bool World::HasScheduledMonsterRespawn(
    int monsterEntityID) const
{
    return scheduledMonsterRespawnTicksByEntityID.find(
               monsterEntityID) !=
           scheduledMonsterRespawnTicksByEntityID.end();
}

std::optional<int> World::GetScheduledMonsterRespawnTick(
    int monsterEntityID) const
{
    auto iterator =
        scheduledMonsterRespawnTicksByEntityID.find(
            monsterEntityID);

    if (iterator ==
        scheduledMonsterRespawnTicksByEntityID.end())
    {
        return std::nullopt;
    }

    return iterator->second;
}

int World::GetCurrentTick() const
{
    return currentTick;
}

void World::UpdateMeleeCombatFeedback()
{
    auto iterator = meleeCombatFeedbacks.begin();

    while (iterator != meleeCombatFeedbacks.end())
    {
        if (iterator->second.remainingTicks <= 0)
        {
            iterator = meleeCombatFeedbacks.erase(iterator);
            continue;
        }

        iterator->second.remainingTicks--;

        if (iterator->second.remainingTicks <= 0)
        {
            iterator = meleeCombatFeedbacks.erase(iterator);
            continue;
        }

        ++iterator;
    }
}

void World::RecordMeleeCombatFeedback(
    int attackerEntityID,
    int defenderEntityID,
    const MeleeAttackResult &result)
{
    meleeCombatFeedbacks[defenderEntityID] = MeleeCombatFeedback{
        attackerEntityID,
        defenderEntityID,
        result.didHit,
        result.actualDamageApplied,
        MeleeCombatFeedbackLifetimeTicks};
}

void World::Update()
{
    Logger::Debug("Updating World");

    entityDiedEvents.clear();
    publishedActionLifecycleEvents.clear();
    respawnedMonsterEntityIDsThisTick.clear();

    publishedCommandProcessingResults.clear();
    pendingCommandProcessingResults.clear();
    ProcessQueuedCommands();

    currentTick++;

    ProcessDueMonsterRespawns();

    UpdateMeleeCombatFeedback();
    ProcessDeadCombatantCleanup();

    ProcessMovementDestinationRequests();
    ProcessActiveMovementPaths();
    ProcessResourceInteractions();
    ProcessStationInteractions();

    objectManager.Update();

    std::vector<Action> completedActions =
        actionManager.Update(currentTick);

    ProcessCompletedActions(completedActions);

    entityManager.Update(*this);
    ProcessMovementRequests();
    ProcessAggressiveMonsters();
    ProcessPendingMeleeInteractions();
    ProcessEntityDeathRewards();
    ScheduleMonsterRespawnsFromDeathEvents();
    PublishActionLifecycleEvents();
    TickPlayerStatusEffects();

    publishedCommandProcessingResults =
        std::move(pendingCommandProcessingResults);
}

std::uint64_t World::EnqueueCommand(
    ServerCommandData command)
{
    return serverCommandQueue.Enqueue(
        std::move(command));
}

const std::vector<CommandProcessingResult> &
World::GetCommandProcessingResults() const
{
    return publishedCommandProcessingResults;
}

void World::ProcessQueuedCommands()
{
    // Only the commands present at this boundary belong to this update.
    // Any command enqueued by processing code waits for the next update.
    const std::size_t batchCount =
        serverCommandQueue.GetCount();

    for (std::size_t index = 0;
         index < batchCount;
         ++index)
    {
        std::optional<ServerCommand> nextCommand =
            serverCommandQueue.PopNext();

        if (!nextCommand.has_value())
        {
            break;
        }

        const ServerCommand &command =
            nextCommand.value();

        std::visit(
            [&](const auto &data)
            {
                using CommandType =
                    std::decay_t<decltype(data)>;

                CommandResultCode resultCode =
                    CommandResultCode::GAMEPLAY_REJECTED;

                Entity *actorEntity =
                    entityManager.GetEntityByID(
                        data.actorEntityID);

                Player *actor =
                    dynamic_cast<Player *>(actorEntity);

                if (actor == nullptr)
                {
                    resultCode =
                        CommandResultCode::INVALID_ACTOR;
                }
                else if constexpr (
                    std::is_same_v<CommandType, MoveCommand>)
                {
                    if (!map.IsInBounds(
                            data.destination.GetX(),
                            data.destination.GetY()))
                    {
                        resultCode =
                            CommandResultCode::INVALID_COMMAND_DATA;
                    }
                    else if (actor->IsAlive())
                    {
                        CloseStationInteraction(
                            data.actorEntityID);
                        ClearPendingResourceInteraction(
                            data.actorEntityID);
                        QueueMovementDestination(
                            MovementDestinationRequest(
                                data.actorEntityID,
                                data.destination.GetX(),
                                data.destination.GetY()));
                        resultCode =
                            CommandResultCode::ACCEPTED;
                    }
                }
                else if constexpr (
                    std::is_same_v<CommandType, AttackCommand>)
                {
                    if (entityManager.GetEntityByID(
                            data.targetEntityID) == nullptr)
                    {
                        resultCode =
                            CommandResultCode::INVALID_COMMAND_DATA;
                    }
                    else if (QueueMeleeEngagementRequest(
                                 data.actorEntityID,
                                 data.targetEntityID,
                                 4))
                    {
                        resultCode =
                            CommandResultCode::ACCEPTED;
                    }
                }
                else if constexpr (
                    std::is_same_v<CommandType,
                                   UseInventoryItemCommand>)
                {
                    if (data.inventorySlotIndex < 0 ||
                        data.inventorySlotIndex >=
                            Inventory::SlotCount)
                    {
                        resultCode =
                            CommandResultCode::INVALID_COMMAND_DATA;
                    }
                    else if (TryConsumeFood(
                                 data.actorEntityID,
                                 data.inventorySlotIndex) ||
                             TryEquipInventoryItem(
                                 data.actorEntityID,
                                 data.inventorySlotIndex))
                    {
                        resultCode =
                            CommandResultCode::ACCEPTED;
                    }
                }
                else if constexpr (
                    std::is_same_v<CommandType,
                                   UnequipItemCommand>)
                {
                    const int slotValue =
                        static_cast<int>(data.equipmentSlot);

                    if (slotValue < 0 ||
                        slotValue >= static_cast<int>(
                            EquipmentSlotType::COUNT))
                    {
                        resultCode =
                            CommandResultCode::INVALID_COMMAND_DATA;
                    }
                    else if (TryUnequipItem(
                                 data.actorEntityID,
                                 data.equipmentSlot))
                    {
                        resultCode =
                            CommandResultCode::ACCEPTED;
                    }
                }
                else if constexpr (
                    std::is_same_v<CommandType,
                                   StartRecipeCommand>)
                {
                    const int recipeValue =
                        static_cast<int>(data.recipeType);

                    if (recipeValue <=
                            static_cast<int>(RecipeType::NONE) ||
                        recipeValue >
                            static_cast<int>(RecipeType::STEEL_BAR))
                    {
                        resultCode =
                            CommandResultCode::INVALID_COMMAND_DATA;
                    }
                    else if (TryStartRecipeAction(
                                 data.actorEntityID,
                                 data.recipeType))
                    {
                        resultCode =
                            CommandResultCode::ACCEPTED;
                    }
                }
                else if constexpr (
                    std::is_same_v<CommandType, InteractCommand>)
                {
                    const bool validDestination =
                        map.IsInBounds(
                            data.destination.GetX(),
                            data.destination.GetY());

                    ResourceNode *resource = nullptr;
                    CraftingStation *station = nullptr;

                    if (data.targetType ==
                        InteractionTargetType::RESOURCE)
                    {
                        resource = objectManager.GetResourceByID(
                            data.targetObjectID);
                    }
                    else if (data.targetType ==
                             InteractionTargetType::STATION)
                    {
                        station = objectManager.GetStationByID(
                            data.targetObjectID);
                    }

                    const bool targetMatchesDestination =
                        (resource != nullptr &&
                         resource->GetX() == data.destination.GetX() &&
                         resource->GetY() == data.destination.GetY()) ||
                        (station != nullptr &&
                         station->GetX() == data.destination.GetX() &&
                         station->GetY() == data.destination.GetY());

                    if (!validDestination ||
                        !targetMatchesDestination)
                    {
                        resultCode =
                            CommandResultCode::INVALID_COMMAND_DATA;
                    }
                    else if (actor->IsAlive() &&
                             (resource == nullptr ||
                              resource->IsActive()))
                    {
                        CancelActionsForEntity(
                            data.actorEntityID,
                            ActionCancelReason::PLAYER_MOVED);
                        CloseStationInteraction(
                            data.actorEntityID);
                        ClearPendingResourceInteraction(
                            data.actorEntityID);

                        if (resource != nullptr)
                        {
                            QueueResourceInteraction(
                                data.actorEntityID,
                                data.targetObjectID);
                        }
                        else
                        {
                            QueueStationInteraction(
                                data.actorEntityID,
                                data.targetObjectID);
                        }

                        QueueMovementDestination(
                            MovementDestinationRequest(
                                data.actorEntityID,
                                data.destination.GetX(),
                                data.destination.GetY()));

                        resultCode =
                            CommandResultCode::ACCEPTED;
                    }
                }
                else if constexpr (
                    std::is_same_v<CommandType,
                                   CloseStationCommand>)
                {
                    CancelActionsForEntity(
                        data.actorEntityID,
                        ActionCancelReason::INTERFACE_CLOSED);
                    CloseStationInteraction(
                        data.actorEntityID);
                    resultCode =
                        CommandResultCode::ACCEPTED;
                }

                pendingCommandProcessingResults.push_back(
                    CommandProcessingResult{
                        command.commandID,
                        data.actorEntityID,
                        resultCode});
            },
            command.data);
    }
}

Map &World::GetMap()
{
    return map;
}

bool World::IsMonsterOutsideLeash(
    const Monster &monster,
    const MonsterAggressionDefinition &aggressionDefinition)
{
    const int distanceFromSpawn =
        Distance::Calculate(
            monster.GetPosition().GetX(),
            monster.GetPosition().GetY(),
            monster.GetOriginalSpawnX(),
            monster.GetOriginalSpawnY());

    return distanceFromSpawn >
           aggressionDefinition.leashRadius;
}

bool World::IsValidMonsterAggressionTarget(
    const Monster &monster,
    int targetEntityID)
{
    if (!monster.HasAggressionDefinition() ||
        targetEntityID == Monster::InvalidAggressionTargetEntityID)
    {
        return false;
    }

    Entity *targetEntity =
        entityManager.GetEntityByID(
            targetEntityID);

    Player *targetPlayer =
        dynamic_cast<Player *>(targetEntity);

    if (targetPlayer == nullptr ||
        !targetPlayer->IsAlive())
    {
        return false;
    }

    std::optional<MonsterAggressionDefinition> aggressionDefinition =
        monster.GetAggressionDefinition();

    if (!aggressionDefinition.has_value())
    {
        return false;
    }

    if (IsMonsterOutsideLeash(
            monster,
            aggressionDefinition.value()))
    {
        return false;
    }

    const int targetDistanceFromSpawn =
        Distance::Calculate(
            targetPlayer->GetPosition().GetX(),
            targetPlayer->GetPosition().GetY(),
            monster.GetOriginalSpawnX(),
            monster.GetOriginalSpawnY());

    return targetDistanceFromSpawn <=
           aggressionDefinition->leashRadius;
}

int World::SelectMonsterAggressionTargetEntityID(
    const Monster &monster,
    const MonsterAggressionDefinition &aggressionDefinition)
{
    struct CandidateTarget
    {
        int entityID;
        int distance;
    };

    std::optional<CandidateTarget> selectedTarget;

    for (const std::unique_ptr<Entity> &entity :
         entityManager.GetEntities())
    {
        Player *player =
            dynamic_cast<Player *>(entity.get());

        if (player == nullptr ||
            !player->IsAlive())
        {
            continue;
        }

        const int distanceToMonster =
            Distance::Calculate(
                monster.GetPosition().GetX(),
                monster.GetPosition().GetY(),
                player->GetPosition().GetX(),
                player->GetPosition().GetY());

        if (distanceToMonster >
            aggressionDefinition.detectionRadius)
        {
            continue;
        }

        const int distanceFromSpawn =
            Distance::Calculate(
                monster.GetOriginalSpawnX(),
                monster.GetOriginalSpawnY(),
                player->GetPosition().GetX(),
                player->GetPosition().GetY());

        if (distanceFromSpawn >
            aggressionDefinition.leashRadius)
        {
            continue;
        }

        const CandidateTarget candidate{
            player->GetID(),
            distanceToMonster};

        if (!selectedTarget.has_value() ||
            candidate.distance < selectedTarget->distance ||
            (candidate.distance == selectedTarget->distance &&
             candidate.entityID < selectedTarget->entityID))
        {
            selectedTarget = candidate;
        }
    }

    if (!selectedTarget.has_value())
    {
        return Monster::InvalidAggressionTargetEntityID;
    }

    return selectedTarget->entityID;
}

void World::ProcessAggressiveMonsters()
{
    for (const std::unique_ptr<Entity> &entity :
         entityManager.GetEntities())
    {
        Monster *monster =
            dynamic_cast<Monster *>(entity.get());

        if (monster == nullptr ||
            !monster->IsAlive() ||
            !monster->HasAggressionDefinition())
        {
            continue;
        }

        if (HasScheduledMonsterRespawn(monster->GetID()))
        {
            continue;
        }

        if (respawnedMonsterEntityIDsThisTick.find(
                monster->GetID()) !=
            respawnedMonsterEntityIDsThisTick.end())
        {
            continue;
        }

        const std::optional<MonsterAggressionDefinition>
            aggressionDefinition =
                monster->GetAggressionDefinition();

        if (!aggressionDefinition.has_value() ||
            aggressionDefinition->detectionRadius <= 0 ||
            aggressionDefinition->leashRadius <
                aggressionDefinition->detectionRadius)
        {
            continue;
        }

        const int monsterEntityID = monster->GetID();
        bool acquiredTargetThisTick = false;

        const auto clearAggressionState =
            [&](ActionCancelReason reason)
        {
            monster->ClearAggressionTargetEntityID();

            CancelActionsForEntity(
                monsterEntityID,
                reason);

            pendingMeleeInteractions.erase(
                monsterEntityID);

            ClearPendingMovementForEntity(
                monsterEntityID);
        };

        if (IsMonsterOutsideLeash(
                *monster,
                aggressionDefinition.value()))
        {
            if (monster->GetAggressionTargetEntityID() !=
                    Monster::InvalidAggressionTargetEntityID ||
                actionManager.HasActionForEntity(
                    monsterEntityID) ||
                HasPendingMeleeEngagement(
                    monsterEntityID))
            {
                clearAggressionState(
                    ActionCancelReason::OUT_OF_RANGE);
            }
        }

        const int currentTargetEntityID =
            monster->GetAggressionTargetEntityID();

        if (currentTargetEntityID !=
                Monster::InvalidAggressionTargetEntityID &&
            !IsValidMonsterAggressionTarget(
                *monster,
                currentTargetEntityID))
        {
            clearAggressionState(
                ActionCancelReason::TARGET_MISSING);
        }

        if (monster->GetAggressionTargetEntityID() ==
            Monster::InvalidAggressionTargetEntityID)
        {
            const int selectedTargetID =
                SelectMonsterAggressionTargetEntityID(
                    *monster,
                    aggressionDefinition.value());

            if (selectedTargetID !=
                Monster::InvalidAggressionTargetEntityID)
            {
                monster->SetAggressionTargetEntityID(
                    selectedTargetID);

                acquiredTargetThisTick = true;
            }
        }

        const int targetEntityID =
            monster->GetAggressionTargetEntityID();

        if (targetEntityID ==
            Monster::InvalidAggressionTargetEntityID)
        {
            if (actionManager.HasActionForEntity(
                    monsterEntityID) ||
                HasPendingMeleeEngagement(
                    monsterEntityID) ||
                activeMovementPaths.find(
                    monsterEntityID) !=
                    activeMovementPaths.end())
            {
                continue;
            }

            if (monster->GetPosition().GetX() ==
                    monster->GetOriginalSpawnX() &&
                monster->GetPosition().GetY() ==
                    monster->GetOriginalSpawnY())
            {
                continue;
            }

            QueueMovementDestination(
                MovementDestinationRequest(
                    monsterEntityID,
                    monster->GetOriginalSpawnX(),
                    monster->GetOriginalSpawnY()));

            continue;
        }

        Entity *targetEntity =
            entityManager.GetEntityByID(targetEntityID);

        Player *targetPlayer =
            dynamic_cast<Player *>(targetEntity);

        if (targetPlayer == nullptr)
        {
            clearAggressionState(
                ActionCancelReason::TARGET_MISSING);
            continue;
        }

        const int distanceX =
            std::abs(
                monster->GetPosition().GetX() -
                targetPlayer->GetPosition().GetX());

        const int distanceY =
            std::abs(
                monster->GetPosition().GetY() -
                targetPlayer->GetPosition().GetY());

        const bool isAdjacent =
            distanceX <= 1 &&
            distanceY <= 1;

        if (isAdjacent)
        {
            ClearPendingMovementForEntity(
                monsterEntityID);

            if (actionManager.HasActionForEntity(
                    monsterEntityID) ||
                HasPendingMeleeEngagement(
                    monsterEntityID))
            {
                continue;
            }

            TryStartMeleeEngagement(
                monsterEntityID,
                targetEntityID,
                DefaultMonsterAttackDurationTicks);

            continue;
        }

        bool hasAction =
            actionManager.HasActionForEntity(
                monsterEntityID);

        bool hasPendingMelee =
            HasPendingMeleeEngagement(
                monsterEntityID);

        auto activePathIterator =
            activeMovementPaths.find(
                monsterEntityID);

        if (hasAction ||
            hasPendingMelee ||
            activePathIterator !=
                activeMovementPaths.end())
        {
            if (acquiredTargetThisTick &&
                activePathIterator !=
                    activeMovementPaths.end())
            {
                ClearPendingMovementForEntity(
                    monsterEntityID);

                activePathIterator =
                    activeMovementPaths.find(
                        monsterEntityID);
            }

            if (hasAction ||
                hasPendingMelee ||
                activePathIterator !=
                    activeMovementPaths.end())
            {
                continue;
            }
        }

        std::optional<std::pair<int, int>> destination =
            FindMeleeApproachTile(
                monster->GetPosition().GetX(),
                monster->GetPosition().GetY(),
                targetPlayer->GetPosition().GetX(),
                targetPlayer->GetPosition().GetY());

        if (!destination.has_value())
        {
            continue;
        }

        QueueMovementDestination(
            MovementDestinationRequest(
                monsterEntityID,
                destination->first,
                destination->second));
    }
}

void World::QueueMovementRequest(const MovementRequest &request)
{
    Entity *entity =
        entityManager.GetEntityByID(
            request.GetEntityID());

    Player *player =
        dynamic_cast<Player *>(entity);

    if (player != nullptr &&
        !player->IsAlive())
    {
        return;
    }

    CancelActionsForEntity(
        request.GetEntityID(),
        ActionCancelReason::PLAYER_MOVED);

    pendingMeleeInteractions.erase(
        request.GetEntityID());

    movementRequests.push(request);
}
void World::QueueMovementDestination(
    const MovementDestinationRequest &request)
{
    Entity *entity =
        entityManager.GetEntityByID(
            request.GetEntityID());

    Player *player =
        dynamic_cast<Player *>(entity);

    if (player != nullptr &&
        !player->IsAlive())
    {
        return;
    }

    CancelActionsForEntity(
        request.GetEntityID(),
        ActionCancelReason::PLAYER_MOVED);

    pendingMeleeInteractions.erase(
        request.GetEntityID());

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
            continue;
        }
        ActiveMovementPath activePath{
            PathStep{
                request.GetDestinationX(),
                request.GetDestinationY()},
            {}};

        for (const PathStep &step : path)
        {
            activePath.remainingSteps.push(step);
        }

        activeMovementPaths.insert_or_assign(
            request.GetEntityID(),
            std::move(activePath));
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

        ActiveMovementPath &activePath =
            pathIterator->second;

        std::queue<PathStep> &path =
            activePath.remainingSteps;

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

        const bool moved = Movement::Move(
            *entity,
            map,
            changeX,
            changeY);

        if (moved)
        {
            path.pop();

            if (path.empty())
            {
                pathIterator =
                    activeMovementPaths.erase(
                        pathIterator);
            }
            else
            {
                ++pathIterator;
            }

            continue;
        }

        std::vector<PathStep> recalculatedPath =
            pathfinder.FindPath(
                map,
                entity->GetPosition().GetX(),
                entity->GetPosition().GetY(),
                activePath.destination.x,
                activePath.destination.y);

        if (recalculatedPath.empty())
        {
            pathIterator =
                activeMovementPaths.erase(
                    pathIterator);

            continue;
        }

        const PathStep &firstRecalculatedStep =
            recalculatedPath.front();

        const int recalculatedChangeX =
            firstRecalculatedStep.x -
            entity->GetPosition().GetX();

        const int recalculatedChangeY =
            firstRecalculatedStep.y -
            entity->GetPosition().GetY();

        const bool firstStepIsOrthogonal =
            ((recalculatedChangeX == -1 ||
              recalculatedChangeX == 1) &&
             recalculatedChangeY == 0) ||
            (recalculatedChangeX == 0 &&
             (recalculatedChangeY == -1 ||
              recalculatedChangeY == 1));

        if (!firstStepIsOrthogonal ||
            !map.IsValidPosition(
                firstRecalculatedStep.x,
                firstRecalculatedStep.y))
        {
            pathIterator =
                activeMovementPaths.erase(
                    pathIterator);

            continue;
        }

        std::queue<PathStep> replacementSteps;

        for (const PathStep &step : recalculatedPath)
        {
            replacementSteps.push(step);
        }

        activePath.remainingSteps =
            std::move(replacementSteps);

        ++pathIterator;
    }
}
void World::ProcessStationInteractions()
{
    auto interactionIterator =
        pendingStationInteractions.begin();

    while (interactionIterator !=
           pendingStationInteractions.end())
    {
        int entityID = interactionIterator->first;
        int stationID = interactionIterator->second;

        Entity *entity =
            entityManager.GetEntityByID(entityID);

        CraftingStation *station =
            objectManager.GetStationByID(stationID);

        if (entity == nullptr || station == nullptr)
        {
            interactionIterator =
                pendingStationInteractions.erase(
                    interactionIterator);
            continue;
        }

        int distanceX = std::abs(
            entity->GetPosition().GetX() -
            station->GetX());

        int distanceY = std::abs(
            entity->GetPosition().GetY() -
            station->GetY());

        bool isAdjacent =
            distanceX <= 1 && distanceY <= 1;

        if (!isAdjacent)
        {
            ++interactionIterator;
            continue;
        }

        activeStations[entityID] =
            stationID;

        openedStations[entityID] =
            station->GetStationType();

        interactionIterator =
            pendingStationInteractions.erase(
                interactionIterator);
    }
}

ActionValidationResult World::ValidateGatheringAction(
    int entityID,
    int resourceID,
    bool checkInventorySpace)
{
    Entity *entity =
        entityManager.GetEntityByID(entityID);

    if (entity == nullptr)
    {
        return {
            false,
            ActionCancelReason::REQUIREMENTS_FAILED,
            "Player could not be found"};
    }

    Player *player =
        dynamic_cast<Player *>(entity);

    if (player == nullptr)
    {
        return {
            false,
            ActionCancelReason::REQUIREMENTS_FAILED,
            "Only players can gather resources"};
    }

    if (!player->IsAlive())
    {
        return {
            false,
            ActionCancelReason::ENTITY_DIED,
            "Player is not alive"};
    }

    ResourceNode *resource =
        objectManager.GetResourceByID(resourceID);

    if (resource == nullptr)
    {
        return {
            false,
            ActionCancelReason::TARGET_MISSING,
            "The resource no longer exists"};
    }

    if (!resource->IsActive())
    {
        return {
            false,
            ActionCancelReason::TARGET_DEPLETED,
            "The resource has been depleted"};
    }

    int distanceX =
        std::abs(
            player->GetPosition().GetX() -
            resource->GetX());

    int distanceY =
        std::abs(
            player->GetPosition().GetY() -
            resource->GetY());

    bool isAdjacent =
        distanceX <= 1 &&
        distanceY <= 1;

    if (!isAdjacent)
    {
        return {
            false,
            ActionCancelReason::OUT_OF_RANGE,
            "You are too far away from the resource"};
    }

    ItemType equippedWeapon =
        player->GetEquipment().GetEquippedItem(
            EquipmentSlotType::WEAPON);

    if (equippedWeapon == ItemType::NONE)
    {
        return {
            false,
            ActionCancelReason::INVALID_TOOL,
            "You need to equip the correct tool"};
    }

    const ResourceDefinition &resourceDefinition =
        ResourceDatabase::Get(
            resource->GetResourceType());

    const ItemDefinition &weaponDefinition =
        ItemDatabase::Get(equippedWeapon);

    if (weaponDefinition.GetToolType() !=
        resourceDefinition.GetRequiredToolType())
    {
        return {
            false,
            ActionCancelReason::INVALID_TOOL,
            "You need to equip the correct tool"};
    }

    RequirementSystem::RequirementResult weaponRequirements =
        RequirementSystem::RequirementEvaluator::EvaluateAll(
            *player,
            weaponDefinition.GetRequirements());

    if (!weaponRequirements.satisfied)
    {
        return {
            false,
            ActionCancelReason::REQUIREMENTS_FAILED,
            weaponRequirements.message};
    }

    RequirementSystem::RequirementResult resourceRequirements =
        RequirementSystem::RequirementEvaluator::EvaluateAll(
            *player,
            resourceDefinition.GetRequirements());

    if (!resourceRequirements.satisfied)
    {
        return {
            false,
            ActionCancelReason::REQUIREMENTS_FAILED,
            resourceRequirements.message};
    }

    if (checkInventorySpace)
    {
        bool canAddReward =
            player->GetInventory().CanAddItem(
                resourceDefinition.GetItemReward(),
                resourceDefinition.GetItemAmount());

        if (!canAddReward)
        {
            return {
                false,
                ActionCancelReason::INVENTORY_FULL,
                "Your inventory is full"};
        }
    }

    return {
        true,
        ActionCancelReason::NONE,
        ""};
}

ActionValidationResult World::ValidateRecipeAction(
    int entityID,
    RecipeType recipeType)
{
    Entity *entity =
        entityManager.GetEntityByID(
            entityID);

    Player *player =
        dynamic_cast<Player *>(entity);

    if (player == nullptr)
    {
        return {
            false,
            ActionCancelReason::REQUIREMENTS_FAILED,
            "Player could not be found"};
    }

    if (!player->IsAlive())
    {
        return {
            false,
            ActionCancelReason::REQUIREMENTS_FAILED,
            "Attacker is not alive"};
    }

    switch (recipeType)
    {
    case RecipeType::BRONZE_BAR:
    case RecipeType::IRON_BAR:
    case RecipeType::STEEL_BAR:
        break;

    case RecipeType::NONE:
    default:
        return {
            false,
            ActionCancelReason::REQUIREMENTS_FAILED,
            "The selected recipe is invalid"};
    }

    const RecipeDefinition &recipe =
        RecipeDatabase::Get(
            recipeType);

    if (!CanUseStation(
            entityID,
            recipe.GetRequiredStationType()))
    {
        return {
            false,
            ActionCancelReason::OUT_OF_RANGE,
            "You must be beside the correct crafting station"};
    }

    RequirementSystem::RequirementResult requirements =
        RequirementSystem::RequirementEvaluator::EvaluateAll(
            *player,
            recipe.GetRequirements());

    if (!requirements.satisfied)
    {
        return {
            false,
            ActionCancelReason::REQUIREMENTS_FAILED,
            requirements.message};
    }

    Inventory simulatedInventory =
        player->GetInventory();

    for (const RecipeIngredient &ingredient :
         recipe.GetIngredients())
    {
        bool removed =
            simulatedInventory.RemoveItem(
                ingredient.itemType,
                ingredient.amount);

        if (!removed)
        {
            return {
                false,
                ActionCancelReason::REQUIREMENTS_FAILED,
                "The required materials could not be removed"};
        }
    }

    if (!simulatedInventory.CanAddItem(
            recipe.GetOutputItem(),
            recipe.GetOutputAmount()))
    {
        return {
            false,
            ActionCancelReason::INVENTORY_FULL,
            "Your inventory does not have enough space"};
    }

    return {
        true,
        ActionCancelReason::NONE,
        ""};
}

ActionValidationResult World::ValidateMeleeAttackAction(
    int attackerEntityID,
    int defenderEntityID)
{
    if (attackerEntityID == defenderEntityID)
    {
        return {
            false,
            ActionCancelReason::REQUIREMENTS_FAILED,
            "You cannot target yourself"};
    }

    Entity *attackerEntity =
        entityManager.GetEntityByID(
            attackerEntityID);

    Entity *defenderEntity =
        entityManager.GetEntityByID(
            defenderEntityID);

    Combatant *attacker =
        TryGetCombatant(
            attackerEntity);

    Combatant *defender =
        TryGetCombatant(
            defenderEntity);

    if (attacker == nullptr)
    {
        return {
            false,
            ActionCancelReason::REQUIREMENTS_FAILED,
            "Attacker could not be found"};
    }

    if (defender == nullptr)
    {
        return {
            false,
            ActionCancelReason::TARGET_MISSING,
            "Defender could not be found"};
    }

    if (!attacker->IsAlive())
    {
        return {
            false,
            ActionCancelReason::REQUIREMENTS_FAILED,
            "Attacker is not alive"};
    }

    if (!defender->IsAlive())
    {
        return {
            false,
            ActionCancelReason::ENTITY_DIED,
            "Defender is not alive"};
    }

    int distanceX = std::abs(
        attackerEntity->GetPosition().GetX() -
        defenderEntity->GetPosition().GetX());

    int distanceY = std::abs(
        attackerEntity->GetPosition().GetY() -
        defenderEntity->GetPosition().GetY());

    bool isAdjacent =
        distanceX <= 1 &&
        distanceY <= 1;

    if (!isAdjacent)
    {
        return {
            false,
            ActionCancelReason::OUT_OF_RANGE,
            "You are too far away from the defender"};
    }

    return {
        true,
        ActionCancelReason::NONE,
        ""};
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
            entityManager.GetEntityByID(
                entityID);

        ResourceNode *resource =
            objectManager.GetResourceByID(
                resourceID);

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
            interactionIterator =
                pendingResourceInteractions.erase(
                    interactionIterator);

            continue;
        }

        int distanceX =
            std::abs(
                entity->GetPosition().GetX() -
                resource->GetX());

        int distanceY =
            std::abs(
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

        ActionValidationResult validation =
            ValidateGatheringAction(
                entityID,
                resourceID,
                false);

        if (!validation.valid)
        {
            if (!validation.message.empty())
            {
                Logger::Game(validation.message);
            }

            CancelActionsForEntity(
                entityID,
                validation.reason);

            interactionIterator =
                pendingResourceInteractions.erase(
                    interactionIterator);

            continue;
        }

        Player *player =
            dynamic_cast<Player *>(entity);

        const ResourceDefinition &resourceDefinition =
            ResourceDatabase::Get(
                resource->GetResourceType());

        ItemType equippedWeapon =
            player->GetEquipment()
                .GetEquippedItem(
                    EquipmentSlotType::WEAPON);

        const ItemDefinition &weaponDefinition =
            ItemDatabase::Get(
                equippedWeapon);

        if (!actionManager.HasActionForEntity(
                entityID))
        {
            StartAction(
                Action(
                    ActionType::GATHERING,
                    "Gathering " +
                        resourceDefinition.GetName(),
                    weaponDefinition
                        .GetActionDurationTicks(),
                    entityID,
                    resourceID,
                    true));
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
        if (action.GetType() ==
            ActionType::MELEE_ATTACK)
        {
            ActionValidationResult validation =
                ValidateMeleeAttackAction(
                    action.GetOwnerID(),
                    action.GetTargetID());

            if (!validation.valid)
            {
                if (!validation.message.empty())
                {
                    Logger::Game(
                        validation.message);
                }

                continue;
            }

            Entity *attackerEntity =
                entityManager.GetEntityByID(
                    action.GetOwnerID());

            Entity *defenderEntity =
                entityManager.GetEntityByID(
                    action.GetTargetID());

            Combatant *attacker =
                TryGetCombatant(
                    attackerEntity);

            Combatant *defender =
                TryGetCombatant(
                    defenderEntity);

            if (attacker == nullptr ||
                defender == nullptr)
            {
                continue;
            }

            RecordActionCompletedEvent(action);

            const bool defenderWasAliveBeforeAttack =
                defender->IsAlive();

            lastMeleeAttackResult =
                combatService.ResolveMeleeAttack(
                    attacker->GetCombatRatings(),
                    defender->GetCombatRatings(),
                    *defender);

            RecordMeleeCombatFeedback(
                action.GetOwnerID(),
                action.GetTargetID(),
                *lastMeleeAttackResult);

            if (!defender->IsAlive())
            {
                int killerEntityID =
                    EntityDiedEvent::InvalidKillerEntityID;

                if (defenderWasAliveBeforeAttack &&
                    lastMeleeAttackResult->didHit &&
                    lastMeleeAttackResult->actualDamageApplied > 0)
                {
                    killerEntityID =
                        action.GetOwnerID();
                }

                HandleCombatantDeath(
                    action.GetTargetID(),
                    killerEntityID);

                continue;
            }

            if (!action.IsRepeating())
            {
                continue;
            }

            ActionValidationResult repeatValidation =
                ValidateMeleeAttackAction(
                    action.GetOwnerID(),
                    action.GetTargetID());

            if (!repeatValidation.valid)
            {
                if (!repeatValidation.message.empty())
                {
                    Logger::Game(
                        repeatValidation.message);
                }

                continue;
            }

            RestartAction(action);

            continue;
        }

        if (action.GetType() ==
            ActionType::RECIPE)
        {
            const int entityID =
                action.GetOwnerID();

            Entity *entity =
                entityManager.GetEntityByID(
                    entityID);

            Player *player =
                dynamic_cast<Player *>(entity);

            if (player == nullptr)
            {
                continue;
            }

            RecipeType recipeType =
                static_cast<RecipeType>(
                    action.GetTargetID());

            ActionValidationResult validation =
                ValidateRecipeAction(
                    entityID,
                    recipeType);

            if (!validation.valid)
            {
                if (!validation.message.empty())
                {
                    Logger::Game(
                        validation.message);
                }

                continue;
            }

            const RecipeDefinition &recipe =
                RecipeDatabase::Get(
                    recipeType);

            bool created =
                RecipeSystem::TryCreateRecipe(
                    *player,
                    recipeType);

            if (!created)
            {
                Logger::Game(
                    "The recipe could not be completed");

                continue;
            }

            RecordActionCompletedEvent(action);

            Logger::Game(
                "Created: " +
                recipe.GetName());

            ActionValidationResult repeatValidation =
                ValidateRecipeAction(
                    entityID,
                    recipeType);

            if (!repeatValidation.valid)
            {
                if (!repeatValidation.message.empty())
                {
                    Logger::Game(
                        repeatValidation.message);
                }

                continue;
            }

            RestartAction(action);

            continue;
        }

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
                action.GetOwnerID());

        Player *player =
            dynamic_cast<Player *>(entity);

        if (player == nullptr)
        {
            continue;
        }

        ActionValidationResult validation =
            ValidateGatheringAction(
                action.GetOwnerID(),
                action.GetTargetID(),
                true);

        if (!validation.valid)
        {
            if (!validation.message.empty())
            {
                Logger::Game(validation.message);
            }

            pendingResourceInteractions.erase(
                action.GetOwnerID());

            CancelActionsForEntity(
                action.GetOwnerID(),
                validation.reason);

            continue;
        }

        const ResourceDefinition &resourceDefinition =
            ResourceDatabase::Get(
                resource->GetResourceType());

        SkillType requiredSkill =
            resourceDefinition.GetRequiredSkill();

        int playerSkillLevel =
            player->GetSkills()
                .GetSkill(requiredSkill)
                .GetLevel();

        int levelsAboveRequirement =
            playerSkillLevel -
            resourceDefinition
                .GetRequiredSkillLevel();

        int successChance =
            resourceDefinition
                .GetBaseSuccessChance() +
            levelsAboveRequirement * 2;

        if (successChance > 95)
        {
            successChance = 95;
        }

        bool successfulGather =
            gatheringRandomSource->RollPercentage(
                successChance);

        if (successfulGather)
        {
            ItemType rewardItem =
                resourceDefinition.GetItemReward();

            int rewardAmount =
                resourceDefinition.GetItemAmount();

            bool itemAdded =
                player->GetInventory().AddItem(
                    rewardItem,
                    rewardAmount);

            if (!itemAdded)
            {
                Logger::Game(
                    "Player inventory is full");

                pendingResourceInteractions.erase(
                    action.GetOwnerID());

                continue;
            }

            int previousLevel =
                player->GetSkills()
                    .GetSkill(requiredSkill)
                    .GetLevel();

            player->GetSkills().AddXP(
                requiredSkill,
                resourceDefinition.GetXPReward());

            const Skill &gatheringSkill =
                player->GetSkills()
                    .GetSkill(requiredSkill);

            const ItemDefinition &rewardDefinition =
                ItemDatabase::Get(
                    rewardItem);

            Logger::Game(
                "Player " +
                std::to_string(
                    player->GetID()) +
                " gathered from " +
                resourceDefinition.GetName() +
                " | Received: " +
                std::to_string(rewardAmount) +
                " " +
                rewardDefinition.GetName() +
                " | XP: " +
                std::to_string(
                    gatheringSkill.GetXP()) +
                " (Level " +
                std::to_string(
                    gatheringSkill.GetLevel()) +
                ")");

            if (gatheringSkill.GetLevel() >
                previousLevel)
            {
                Logger::Game(
                    "Gathering skill level increased to " +
                    std::to_string(
                        gatheringSkill.GetLevel()));
            }

            resource->ConsumeUse();

            Logger::Debug(
                resourceDefinition.GetName() +
                " uses remaining: " +
                std::to_string(
                    resource->GetRemainingUses()));
        }
        else
        {
            Logger::Game(
                "You attempt to gather from the " +
                resourceDefinition.GetName() +
                " but receive nothing");
        }

        RecordActionCompletedEvent(action);

        // Only stop after a successful gather depleted the resource.
        if (!resource->IsActive())
        {
            pendingResourceInteractions.erase(
                action.GetOwnerID());

            Logger::Game(
                resourceDefinition.GetName() +
                " has been depleted");

            continue;
        }

        ActionValidationResult repeatValidation =
            ValidateGatheringAction(
                action.GetOwnerID(),
                action.GetTargetID(),
                true);

        if (!repeatValidation.valid)
        {
            if (!repeatValidation.message.empty())
            {
                Logger::Game(
                    repeatValidation.message);
            }

            continue;
        }

        RestartAction(action);
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

bool World::TryConsumeFood(
    int playerEntityID,
    int inventorySlotIndex)
{
    Entity *entity =
        entityManager.GetEntityByID(
            playerEntityID);

    if (entity == nullptr)
    {
        return false;
    }

    Player *player =
        dynamic_cast<Player *>(entity);

    if (player == nullptr)
    {
        return false;
    }

    if (!player->IsAlive())
    {
        return false;
    }

    if (inventorySlotIndex < 0 ||
        inventorySlotIndex >= Inventory::SlotCount)
    {
        return false;
    }

    Inventory &inventory =
        player->GetInventory();

    const InventorySlot &slot =
        inventory.GetSlots()[inventorySlotIndex];

    if (slot.IsEmpty())
    {
        return false;
    }

    const ItemType itemType =
        slot.GetItemType();

    const ItemDefinition &itemDefinition =
        ItemDatabase::Get(itemType);

    if (itemDefinition.GetItemType() != itemType)
    {
        return false;
    }

    const FoodDefinition *foodDefinition =
        itemDefinition.GetFoodDefinition();

    if (foodDefinition == nullptr)
    {
        return false;
    }

    const int healAmount =
        foodDefinition->healAmount;

    if (healAmount <= 0)
    {
        return false;
    }

    if (player->GetCurrentHealth() >=
        player->GetMaximumHealth())
    {
        return false;
    }

    Inventory simulatedInventory =
        inventory;

    if (!simulatedInventory.RemoveItemFromSlot(
            inventorySlotIndex,
            1))
    {
        return false;
    }

    player->GetInventory() =
        simulatedInventory;

    return player->TryHeal(
        healAmount);
}

bool World::TryEquipInventoryItem(
    int entityID,
    int slotIndex)
{
    Entity *entity =
        entityManager.GetEntityByID(
            entityID);

    Player *player =
        dynamic_cast<Player *>(entity);

    if (player == nullptr)
    {
        return false;
    }

    if (slotIndex < 0 ||
        slotIndex >= Inventory::SlotCount)
    {
        return false;
    }

    Inventory &inventory =
        player->GetInventory();

    const InventorySlot &inventorySlot =
        inventory.GetSlots()[slotIndex];

    if (inventorySlot.IsEmpty())
    {
        return false;
    }

    ItemType newItem =
        inventorySlot.GetItemType();

    const ItemDefinition &newDefinition =
        ItemDatabase::Get(newItem);

    if (!newDefinition.IsEquippable())
    {
        return false;
    }
    RequirementSystem::RequirementResult equipmentRequirements =
        RequirementSystem::RequirementEvaluator::EvaluateAll(
            *player,
            newDefinition.GetRequirements());

    if (!equipmentRequirements.satisfied)
    {
        Logger::Game(
            equipmentRequirements.message);

        return false;
    }

    EquipmentSlotType equipmentSlot =
        newDefinition.GetEquipmentSlot();

    Equipment &equipment =
        player->GetEquipment();

    ItemType previousItem =
        equipment.GetEquippedItem(
            equipmentSlot);

    if (previousItem == newItem)
    {
        Logger::Game(
            newDefinition.GetName() +
            " is already equipped");

        return false;
    }

    // Removing the new item first guarantees that
    // there is space for the old equipped item.
    bool removedNewItem =
        inventory.RemoveItem(
            newItem,
            1);

    if (!removedNewItem)
    {
        return false;
    }

    if (previousItem != ItemType::NONE)
    {
        equipment.Unequip(
            equipmentSlot);
    }

    bool equippedNewItem =
        equipment.Equip(
            equipmentSlot,
            newItem);

    if (!equippedNewItem)
    {
        if (previousItem != ItemType::NONE)
        {
            equipment.Equip(
                equipmentSlot,
                previousItem);
        }

        inventory.AddItem(
            newItem,
            1);

        return false;
    }

    if (previousItem != ItemType::NONE)
    {
        bool returnedPreviousItem =
            inventory.AddItem(
                previousItem,
                1);

        if (!returnedPreviousItem)
        {
            // Roll everything back so no item is lost.
            equipment.Unequip(
                equipmentSlot);

            equipment.Equip(
                equipmentSlot,
                previousItem);

            inventory.AddItem(
                newItem,
                1);

            Logger::Error(
                "Equipment swap failed");

            return false;
        }

        const ItemDefinition &previousDefinition =
            ItemDatabase::Get(
                previousItem);

        Logger::Game(
            newDefinition.GetName() +
            " equipped | " +
            previousDefinition.GetName() +
            " returned to inventory");

        if (equipmentSlot ==
            EquipmentSlotType::WEAPON)
        {
            CancelGatheringForToolChange(
                entityID);
        }

        player->RefreshDerivedState();

        return true;
    }

    Logger::Game(
        newDefinition.GetName() +
        " equipped");

    if (equipmentSlot ==
        EquipmentSlotType::WEAPON)
    {
        CancelGatheringForToolChange(
            entityID);
    }

    player->RefreshDerivedState();

    return true;
}

bool World::TryUnequipItem(
    int entityID,
    EquipmentSlotType equipmentSlot)
{
    Entity *entity =
        entityManager.GetEntityByID(
            entityID);

    Player *player =
        dynamic_cast<Player *>(entity);

    if (player == nullptr)
    {
        return false;
    }

    if (!IsValidEquipmentSlot(equipmentSlot))
    {
        return false;
    }

    Inventory simulatedInventory =
        player->GetInventory();

    Equipment simulatedEquipment =
        player->GetEquipment();

    ItemType removedItem =
        simulatedEquipment.Unequip(
            equipmentSlot);

    if (removedItem == ItemType::NONE)
    {
        return false;
    }

    const ItemDefinition &removedDefinition =
        ItemDatabase::Get(removedItem);

    if (removedDefinition.GetItemType() != removedItem)
    {
        return false;
    }

    if (!simulatedInventory.CanAddItem(
            removedItem,
            1))
    {
        return false;
    }

    if (!simulatedInventory.AddItem(
            removedItem,
            1))
    {
        return false;
    }

    player->GetInventory() = simulatedInventory;
    player->GetEquipment() = simulatedEquipment;
    player->RefreshDerivedState();

    if (removedDefinition.GetToolType() != ToolType::NONE)
    {
        CancelGatheringForToolChange(
            entityID);
    }

    Logger::Game(
        removedDefinition.GetName() +
        " unequipped");

    return true;
}

bool World::TryApplyStatusEffect(
    int playerEntityID,
    const StatusEffectDefinition &definition)
{
    Entity *entity =
        entityManager.GetEntityByID(
            playerEntityID);

    Player *player =
        dynamic_cast<Player *>(entity);

    if (player == nullptr)
    {
        return false;
    }

    if (!IsValidStatusEffectDefinition(
            definition))
    {
        return false;
    }

    if (!player->GetStatusEffectManager().Apply(
            definition,
            currentTick))
    {
        return false;
    }

    player->RefreshDerivedState();

    return true;
}

bool World::TryRemoveStatusEffect(
    int playerEntityID,
    StatusEffectType type)
{
    Entity *entity =
        entityManager.GetEntityByID(
            playerEntityID);

    Player *player =
        dynamic_cast<Player *>(entity);

    if (player == nullptr)
    {
        return false;
    }

    if (!IsKnownStatusEffectType(type))
    {
        return false;
    }

    if (!player->GetStatusEffectManager().Remove(
            type))
    {
        return false;
    }

    player->RefreshDerivedState();

    return true;
}

void World::TryStartMonsterRetaliation(
    int monsterEntityID,
    int playerEntityID)
{
    Entity *monsterEntity =
        entityManager.GetEntityByID(
            monsterEntityID);

    Entity *playerEntity =
        entityManager.GetEntityByID(
            playerEntityID);

    Monster *monster =
        dynamic_cast<Monster *>(monsterEntity);

    Player *player =
        dynamic_cast<Player *>(playerEntity);

    if (monster == nullptr ||
        player == nullptr)
    {
        return;
    }

    if (!monster->IsAlive() ||
        !player->IsAlive())
    {
        return;
    }

    if (monster->HasAggressionDefinition())
    {
        const int currentTargetID =
            monster->GetAggressionTargetEntityID();

        if (currentTargetID !=
                Monster::InvalidAggressionTargetEntityID &&
            IsValidMonsterAggressionTarget(
                *monster,
                currentTargetID))
        {
            return;
        }

        monster->SetAggressionTargetEntityID(
            playerEntityID);
    }

    int distanceX = std::abs(
        monster->GetPosition().GetX() -
        player->GetPosition().GetX());

    int distanceY = std::abs(
        monster->GetPosition().GetY() -
        player->GetPosition().GetY());

    bool isAdjacent =
        distanceX <= 1 &&
        distanceY <= 1;

    if (!isAdjacent)
    {
        return;
    }

    if (actionManager.HasActionForEntity(
            monsterEntityID))
    {
        return;
    }

    TryStartMeleeEngagement(
        monsterEntityID,
        playerEntityID,
        DefaultMonsterAttackDurationTicks);
}

void World::CancelMeleeActionsTargetingEntity(
    int targetEntityID,
    ActionCancelReason reason)
{
    std::vector<CancelledActionSnapshot> cancelledActions =
        actionManager.CancelMeleeActionsTargetingEntity(
            targetEntityID,
            reason);

    for (const CancelledActionSnapshot &cancelledAction :
         cancelledActions)
    {
        RecordActionCancelledEvent(
            cancelledAction);
    }
}

void World::PublishActionLifecycleEvents()
{
    publishedActionLifecycleEvents =
        std::move(pendingActionLifecycleEvents);
    pendingActionLifecycleEvents.clear();
}

void World::RecordActionStartedEvent(
    const ActionStartedSnapshot &snapshot)
{
    pendingActionLifecycleEvents.push_back(
        ActionLifecycleEvent{
            ActionStartedEvent{
                snapshot.ownerEntityID,
                snapshot.targetID,
                snapshot.actionType,
                snapshot.startTick,
                snapshot.completionTick}});
}

void World::RecordActionCompletedEvent(
    const Action &action)
{
    pendingActionLifecycleEvents.push_back(
        ActionLifecycleEvent{
            ActionCompletedEvent{
                action.GetOwnerID(),
                action.GetTargetID(),
                action.GetType(),
                action.GetCompletionTick()}});
}

void World::RecordActionCancelledEvent(
    const CancelledActionSnapshot &snapshot)
{
    pendingActionLifecycleEvents.push_back(
        ActionLifecycleEvent{
            ActionCancelledEvent{
                snapshot.ownerEntityID,
                snapshot.targetID,
                snapshot.actionType,
                snapshot.reason,
                currentTick}});
}

void World::StartAction(
    const Action &action)
{
    ActionStartTransition transition =
        actionManager.StartAction(
            action,
            currentTick);

    if (transition.cancelledAction.has_value())
    {
        RecordActionCancelledEvent(
            transition.cancelledAction.value());
    }

    if (transition.startedAction.has_value())
    {
        RecordActionStartedEvent(
            transition.startedAction.value());
    }
}

void World::RestartAction(
    const Action &action)
{
    ActionStartTransition transition =
        actionManager.RestartAction(
            action,
            currentTick);

    if (transition.cancelledAction.has_value())
    {
        RecordActionCancelledEvent(
            transition.cancelledAction.value());
    }

    if (transition.startedAction.has_value())
    {
        RecordActionStartedEvent(
            transition.startedAction.value());
    }
}

void World::ClearPendingMeleeInteractionsInvolvingEntity(
    int entityID)
{
    auto iterator =
        pendingMeleeInteractions.begin();

    while (iterator != pendingMeleeInteractions.end())
    {
        if (iterator->second.attackerEntityID == entityID ||
            iterator->second.defenderEntityID == entityID)
        {
            iterator = pendingMeleeInteractions.erase(
                iterator);
            continue;
        }

        ++iterator;
    }
}

void World::ClearPendingMovementForEntity(
    int entityID)
{
    std::queue<MovementRequest> remainingMovementRequests;

    while (!movementRequests.empty())
    {
        MovementRequest request = movementRequests.front();
        movementRequests.pop();

        if (request.GetEntityID() != entityID)
        {
            remainingMovementRequests.push(request);
        }
    }

    movementRequests = std::move(
        remainingMovementRequests);

    std::queue<MovementDestinationRequest>
        remainingDestinationRequests;

    while (!movementDestinationRequests.empty())
    {
        MovementDestinationRequest request =
            movementDestinationRequests.front();
        movementDestinationRequests.pop();

        if (request.GetEntityID() != entityID)
        {
            remainingDestinationRequests.push(
                request);
        }
    }

    movementDestinationRequests = std::move(
        remainingDestinationRequests);

    activeMovementPaths.erase(entityID);
}

void World::HandleCombatantDeath(
    int deadEntityID,
    int killerEntityID)
{
    Entity *entity =
        entityManager.GetEntityByID(
            deadEntityID);

    Combatant *combatant =
        TryGetCombatant(entity);

    if (combatant == nullptr ||
        combatant->IsAlive())
    {
        return;
    }

    const bool firstProcessedDeath =
        processedDeathEntityIDs.insert(
                                   deadEntityID)
            .second;

    if (firstProcessedDeath)
    {
        RecordEntityDiedEvent(
            deadEntityID,
            killerEntityID);
    }

    CancelActionsForEntity(
        deadEntityID,
        ActionCancelReason::ENTITY_DIED);

    CancelMeleeActionsTargetingEntity(
        deadEntityID,
        ActionCancelReason::ENTITY_DIED);

    ClearPendingMeleeInteractionsInvolvingEntity(
        deadEntityID);

    Monster *monster =
        dynamic_cast<Monster *>(entity);

    if (monster != nullptr)
    {
        monster->ClearAggressionTargetEntityID();
    }

    ClearPendingMovementForEntity(
        deadEntityID);

    Player *player =
        dynamic_cast<Player *>(entity);

    if (player == nullptr)
    {
        return;
    }

    pendingResourceInteractions.erase(
        deadEntityID);

    pendingStationInteractions.erase(
        deadEntityID);

    openedStations.erase(
        deadEntityID);

    activeStations.erase(
        deadEntityID);
}

void World::RecordEntityDiedEvent(
    int deadEntityID,
    int killerEntityID)
{
    entityDiedEvents.emplace_back(
        deadEntityID,
        killerEntityID,
        currentTick);

    if (killerEntityID ==
        EntityDiedEvent::InvalidKillerEntityID)
    {
        Logger::Game(
            "[COMBAT] Entity " +
            std::to_string(deadEntityID) +
            " died");

        return;
    }

    Logger::Game(
        "[COMBAT] Entity " +
        std::to_string(killerEntityID) +
        " killed entity " +
        std::to_string(deadEntityID));
}

void World::ProcessDeadCombatantCleanup()
{
    for (const auto &entity : entityManager.GetEntities())
    {
        Combatant *combatant =
            TryGetCombatant(entity.get());

        if (combatant == nullptr)
        {
            continue;
        }

        if (combatant->IsAlive())
        {
            processedDeathEntityIDs.erase(
                entity->GetID());

            continue;
        }

        HandleCombatantDeath(
            entity->GetID());
    }
}

bool World::TryBuildLootReceiptMessage(
    const std::vector<ItemReward> &rewards,
    std::string &message)
{
    message.clear();

    if (rewards.empty())
    {
        return true;
    }

    for (int index = 0; index < static_cast<int>(rewards.size()); ++index)
    {
        const ItemReward &reward = rewards[index];

        const ItemDefinition &definition =
            ItemDatabase::Get(reward.itemType);

        if (definition.GetItemType() != reward.itemType)
        {
            message.clear();
            return false;
        }

        if (!message.empty())
        {
            message += " and ";
        }

        message += std::to_string(reward.quantity) +
                   " " +
                   definition.GetName();
    }

    return true;
}

void World::ProcessEntityDeathRewards()
{
    for (const EntityDiedEvent &event : entityDiedEvents)
    {
        Entity *deadEntity =
            entityManager.GetEntityByID(
                event.deadEntityID);

        Monster *monster =
            dynamic_cast<Monster *>(deadEntity);

        if (monster == nullptr)
        {
            continue;
        }

        RewardTableType rewardTableType =
            monster->GetRewardTableType();

        if (rewardTableType == RewardTableType::NONE)
        {
            continue;
        }

        Entity *killerEntity =
            entityManager.GetEntityByID(
                event.killerEntityID);

        Player *killer =
            dynamic_cast<Player *>(killerEntity);

        if (killer == nullptr)
        {
            continue;
        }

        const RewardTable *rewardTable =
            RewardTableRegistry::TryGetRewardTable(
                rewardTableType);

        if (rewardTable == nullptr ||
            rewardTableRoller == nullptr)
        {
            continue;
        }

        std::vector<ItemReward> rolledRewards =
            rewardTableRoller->Roll(
                *rewardTable);

        if (rolledRewards.empty())
        {
            Logger::Game(
                "[LOOT] Player " +
                std::to_string(killer->GetID()) +
                " could not roll Monster " +
                std::to_string(monster->GetID()) +
                " rewards");
            continue;
        }

        std::vector<ItemAmount> itemAmounts;
        itemAmounts.reserve(rolledRewards.size());

        for (const ItemReward &reward : rolledRewards)
        {
            itemAmounts.push_back(
                ItemAmount{
                    reward.itemType,
                    reward.quantity});
        }

        bool granted =
            killer->GetInventory()
                .TryAddItemsAtomically(
                    itemAmounts);

        if (!granted)
        {
            Logger::Game(
                "[LOOT] Player " +
                std::to_string(killer->GetID()) +
                " could not receive Monster " +
                std::to_string(monster->GetID()) +
                " rewards");
            continue;
        }

        std::string rewardReceipt;

        if (!TryBuildLootReceiptMessage(
                rolledRewards,
                rewardReceipt))
        {
            Logger::Game(
                "[LOOT] Player " +
                std::to_string(killer->GetID()) +
                " received rewards from Monster " +
                std::to_string(monster->GetID()));
            continue;
        }

        Logger::Game(
            "[LOOT] Player " +
            std::to_string(killer->GetID()) +
            " received " +
            rewardReceipt);
    }
}

void World::TickPlayerStatusEffects()
{
    for (const std::unique_ptr<Entity> &entity :
         entityManager.GetEntities())
    {
        Player *player =
            dynamic_cast<Player *>(
                entity.get());

        if (player == nullptr)
        {
            continue;
        }

        bool expiredEffects =
            player->GetStatusEffectManager().Tick(
                currentTick);

        if (!expiredEffects)
        {
            continue;
        }

        player->RefreshDerivedState();
    }
}

void World::ScheduleMonsterRespawnsFromDeathEvents()
{
    for (const EntityDiedEvent &event : entityDiedEvents)
    {
        Entity *deadEntity =
            entityManager.GetEntityByID(
                event.deadEntityID);

        Monster *monster =
            dynamic_cast<Monster *>(deadEntity);

        if (monster == nullptr)
        {
            continue;
        }

        if (!monster->HasRespawnDefinition())
        {
            continue;
        }

        if (!monster->IsAlive())
        {
            if (HasScheduledMonsterRespawn(
                    monster->GetID()))
            {
                continue;
            }

            std::optional<MonsterRespawnDefinition>
                respawnDefinition =
                    monster->GetRespawnDefinition();

            if (!respawnDefinition.has_value() ||
                respawnDefinition->delayTicks <= 0)
            {
                continue;
            }

            int respawnTick = 0;

            if (!TryCalculateRespawnTick(
                    respawnDefinition->delayTicks,
                    respawnTick))
            {
                continue;
            }

            ScheduleMonsterRespawn(
                monster->GetID(),
                respawnTick);
        }
    }
}

void World::ProcessDueMonsterRespawns()
{
    while (!scheduledMonsterRespawnEntityIDsByTick.empty())
    {
        auto bucketIterator =
            scheduledMonsterRespawnEntityIDsByTick.begin();

        const int dueTick =
            bucketIterator->first;

        if (dueTick > currentTick)
        {
            break;
        }

        std::set<int> dueMonsterEntityIDs =
            bucketIterator->second;

        scheduledMonsterRespawnEntityIDsByTick.erase(
            bucketIterator);

        for (int monsterEntityID : dueMonsterEntityIDs)
        {
            auto scheduledIterator =
                scheduledMonsterRespawnTicksByEntityID.find(
                    monsterEntityID);

            if (scheduledIterator ==
                    scheduledMonsterRespawnTicksByEntityID.end() ||
                scheduledIterator->second != dueTick)
            {
                continue;
            }

            scheduledMonsterRespawnTicksByEntityID.erase(
                scheduledIterator);

            Entity *entity =
                entityManager.GetEntityByID(
                    monsterEntityID);

            Monster *monster =
                dynamic_cast<Monster *>(entity);

            if (monster == nullptr)
            {
                continue;
            }

            if (monster->IsAlive())
            {
                processedDeathEntityIDs.erase(
                    monsterEntityID);
                continue;
            }

            ExecuteMonsterRespawn(*monster);
        }
    }
}

bool World::TryCalculateRespawnTick(
    int delayTicks,
    int &respawnTick) const
{
    if (delayTicks <= 0)
    {
        return false;
    }

    constexpr int MaxInt =
        std::numeric_limits<int>::max();

    if (currentTick > MaxInt - delayTicks)
    {
        return false;
    }

    respawnTick =
        currentTick + delayTicks;

    return true;
}

bool World::ScheduleMonsterRespawn(
    int monsterEntityID,
    int respawnTick)
{
    if (scheduledMonsterRespawnTicksByEntityID.find(
            monsterEntityID) !=
        scheduledMonsterRespawnTicksByEntityID.end())
    {
        return false;
    }

    scheduledMonsterRespawnTicksByEntityID[monsterEntityID] =
        respawnTick;

    scheduledMonsterRespawnEntityIDsByTick[respawnTick].insert(
        monsterEntityID);

    return true;
}

void World::ExecuteMonsterRespawn(
    Monster &monster)
{
    const int monsterEntityID =
        monster.GetID();

    monster.RestoreHealthToFull();
    monster.ClearAggressionTargetEntityID();

    monster.GetPosition().SetPosition(
        monster.GetOriginalSpawnX(),
        monster.GetOriginalSpawnY());

    CancelActionsForEntity(
        monsterEntityID,
        ActionCancelReason::ENTITY_DIED);

    CancelMeleeActionsTargetingEntity(
        monsterEntityID,
        ActionCancelReason::ENTITY_DIED);

    ClearPendingMeleeInteractionsInvolvingEntity(
        monsterEntityID);

    ClearPendingMovementForEntity(
        monsterEntityID);

    ClearCombatFeedbackInvolvingEntity(
        monsterEntityID);

    pendingResourceInteractions.erase(
        monsterEntityID);

    pendingStationInteractions.erase(
        monsterEntityID);

    openedStations.erase(
        monsterEntityID);

    activeStations.erase(
        monsterEntityID);

    processedDeathEntityIDs.erase(
        monsterEntityID);

    respawnedMonsterEntityIDsThisTick.insert(
        monsterEntityID);

    Logger::Game(
        "[SPAWN] Monster " +
        std::to_string(monsterEntityID) +
        " respawned at (" +
        std::to_string(monster.GetOriginalSpawnX()) +
        ", " +
        std::to_string(monster.GetOriginalSpawnY()) +
        ")");
}

void World::ClearCombatFeedbackInvolvingEntity(
    int entityID)
{
    auto iterator =
        meleeCombatFeedbacks.begin();

    while (iterator != meleeCombatFeedbacks.end())
    {
        const MeleeCombatFeedback &feedback =
            iterator->second;

        if (iterator->first == entityID ||
            feedback.attackerEntityID == entityID ||
            feedback.defenderEntityID == entityID)
        {
            iterator = meleeCombatFeedbacks.erase(
                iterator);
            continue;
        }

        ++iterator;
    }
}

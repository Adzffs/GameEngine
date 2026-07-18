#include "World.h"
#include <iostream>
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
#include "../Core/Random.h"
#include "../Core/SeededRandom.h"
#include "../Recipe/RecipeSystem.h"
#include "../Recipe/RecipeDatabase.h"
#include "../Reward/RewardTableRegistry.h"
#include "../Combat/Combatant.h"
#include "../Combat/MeleeCombatFeedback.h"
#include "../Requirement/RequirementEvaluator.h"
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>
#include <set>

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
}

World::World(
    unsigned int combatSeed,
    unsigned int rewardSeed)
    : World(
          std::make_unique<SeededRandom>(combatSeed),
          std::make_unique<SeededRandom>(rewardSeed))
{
}

World::World(std::unique_ptr<RandomSource> combatRandomSource)
    : World(
          std::move(combatRandomSource),
          std::make_unique<SeededRandom>(7331U))
{
}

World::World(
    std::unique_ptr<RandomSource> combatRandomSource,
    std::unique_ptr<RandomSource> rewardRandomSource)
    : combatService(std::move(combatRandomSource)),
      rewardRandomSource(std::move(rewardRandomSource)),
      map(100, 100)
{
    if (this->rewardRandomSource == nullptr)
    {
        throw std::invalid_argument("World requires a non-null reward RandomSource");
    }

    rewardTableRoller = std::make_unique<RewardTableRoller>(
        *this->rewardRandomSource);

    entityManager.CreateNPC(3, 3);

    CreateMonster(
        6,
        1,
        CombatRatings{
            5,
            4,
            3,
            30},
        RewardTableType::DEVELOPMENT_MONSTER,
        MonsterRespawnDefinition{8});

    CreateResource(
        ResourceType::NORMAL_TREE,
        5,
        5);

    CreateResource(
        ResourceType::OAK_TREE,
        8,
        5);

    CreateResource(
        ResourceType::WILLOW_TREE,
        11,
        5);

    CreateResource(
        ResourceType::COPPER_ROCK,
        5,
        9);

    CreateResource(
        ResourceType::TIN_ROCK,
        8,
        9);

    CreateResource(
        ResourceType::IRON_ROCK,
        11,
        9);

    CreateResource(
        ResourceType::COAL_ROCK,
        14,
        12);

    CreateStation(
        StationType::FURNACE,
        14,
        9);
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
    std::optional<MonsterRespawnDefinition> respawnDefinition)
{
    return entityManager.CreateMonster(
        x,
        y,
        ratings,
        rewardTableType,
        respawnDefinition);
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
    actionManager.CancelActionsForEntity(
        entityID,
        reason);
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
        actionManager.CancelActionsForEntity(
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

    actionManager.StartAction(
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

    actionManager.StartAction(
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
        actionManager.Update();

    ProcessCompletedActions(completedActions);

    entityManager.Update(*this);
    ProcessMovementRequests();
    ProcessPendingMeleeInteractions();
    ProcessEntityDeathRewards();
    ScheduleMonsterRespawnsFromDeathEvents();
}

Map &World::GetMap()
{
    return map;
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
        const PathStep &finalStep = path.back();

        if (finalStep.x != request.GetDestinationX() ||
            finalStep.y != request.GetDestinationY())
        {
        }

        std::queue<PathStep> storedPath;

        for (const PathStep &step : path)
        {
            storedPath.push(step);
        }

        activeMovementPaths[request.GetEntityID()] =
            storedPath;
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

            actionManager.CancelActionsForEntity(
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
            actionManager.StartAction(
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

            actionManager.RestartAction(action);

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

            actionManager.RestartAction(action);

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

            actionManager.CancelActionsForEntity(
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
            Random::RollPercentage(
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

        actionManager.RestartAction(action);
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

bool World::TryUnequipWeapon(
    int entityID)
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

    Equipment &equipment =
        player->GetEquipment();

    ItemType equippedItem =
        equipment.GetEquippedItem(
            EquipmentSlotType::WEAPON);

    if (equippedItem ==
        ItemType::NONE)
    {
        return false;
    }

    bool added =
        player->GetInventory().AddItem(
            equippedItem,
            1);

    if (!added)
    {
        Logger::Game(
            "Your inventory is full");

        return false;
    }

    equipment.Unequip(
        EquipmentSlotType::WEAPON);

    player->RefreshDerivedState();

    CancelGatheringForToolChange(
        entityID);

    Logger::Game(
        "Weapon unequipped");

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
    actionManager.CancelMeleeActionsTargetingEntity(
        targetEntityID,
        reason);
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

    ClearPendingMovementForEntity(
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

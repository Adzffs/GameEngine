#include "World.h"
#include "../AI/MonsterAISystem.h"
#include <iostream>
#include "Distance.h"
#include "Object/Resource/ResourceNode.h"
#include "../Core/Logger.h"
#include "../Action/Action.h"
#include "../Movement/Movement.h"
#include "../Pathfinding/Pathfinder.h"
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
#include "../NPC/NpcDefinition.h"
#include "../NPC/NpcDefinitionDatabase.h"
#include "../NPC/NPC.h"
#include "../NPC/NpcSpawnDefinition.h"
#include "../NPC/NpcSpawnDatabase.h"
#include "../Dialogue/DialogueDefinitionDatabase.h"
#include "../Shop/ShopDefinitionDatabase.h"
#include "../Reward/RewardTableRegistry.h"
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
#include "../Persistence/PlayerSaveFileStore.h"
#include "../Persistence/PlayerSaveState.h"
#include "../Quest/QuestSystem.h"

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

    for (const NpcSpawnDefinition &spawn : NpcSpawnDatabase::GetStarterSpawns())
    {
        npcSpawnManager.AddSpawn(spawn);
    }

    CreateNpc(NpcType::DEVELOPMENT_GUIDE,
              *NpcSpawnDatabase::TryGet(NpcSpawnId::DEVELOPMENT_GUIDE_SPAWN));

    for (const NpcSpawnDefinition &spawn :
         NpcSpawnDatabase::GetStarterMonsterSpawns())
    {
        CreateMonster(spawn.npcType, spawn);
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

WorldPlayerLoadResult World::LoadOrCreatePlayerFromFile(
    const std::filesystem::path &savePath)
{
    WorldPlayerLoadResult result;
    PlayerSaveFileLoadResult fileResult = PlayerSaveFileStore::Load(savePath);
    const PlayerSaveFileLoadStatus fileStatus = fileResult.GetStatus();
    const std::optional<PlayerSaveData> saveData = fileResult.GetSaveData();

    if (fileStatus == PlayerSaveFileLoadStatus::FAILURE)
    {
        result.SetFileLoadResult(std::move(fileResult));
        result.AddIssue(WorldPlayerPersistenceIssueSeverity::ERROR,
                        WorldPlayerPersistenceIssueCode::FILE_LOAD_FAILED, 0,
                        "Player save file could not be loaded");
        return result;
    }

    std::unique_ptr<Player> player;
    int candidateID = 0;

    if (fileStatus == PlayerSaveFileLoadStatus::NOT_FOUND)
    {
        result.SetFileLoadResult(std::move(fileResult));
        if (!map.IsValidPosition(DevelopmentWorldContent::PlayerSpawnX,
                                 DevelopmentWorldContent::PlayerSpawnY))
        {
            result.AddIssue(WorldPlayerPersistenceIssueSeverity::ERROR,
                            WorldPlayerPersistenceIssueCode::DEVELOPMENT_SPAWN_INVALID,
                            candidateID, "Development Player spawn is invalid");
            return result;
        }
        candidateID = entityManager.GetNextEntityIDCandidate();
        player = std::make_unique<Player>(
            candidateID, PlayerInitializationMode::DEVELOPMENT_DEFAULTS);
        player->GetPosition().SetPosition(DevelopmentWorldContent::PlayerSpawnX,
                                          DevelopmentWorldContent::PlayerSpawnY);
    }
    else
    {
        if (!saveData.has_value())
        {
            result.SetFileLoadResult(std::move(fileResult));
            result.AddIssue(WorldPlayerPersistenceIssueSeverity::ERROR,
                            WorldPlayerPersistenceIssueCode::PLAYER_RECONSTRUCTION_FAILED,
                            candidateID, "Loaded save did not contain Player data");
            return result;
        }
        return TryRegisterLoadedPlayer(
            std::move(fileResult), *saveData, &PlayerSaveState::TryCreatePlayer);
    }

    const int finalX = player->GetPosition().GetX();
    const int finalY = player->GetPosition().GetY();
    if (!map.IsValidPosition(finalX, finalY))
    {
        result.AddIssue(WorldPlayerPersistenceIssueSeverity::ERROR,
                        WorldPlayerPersistenceIssueCode::DEVELOPMENT_SPAWN_INVALID,
                        candidateID, "Final Player position is invalid");
        return result;
    }

    const int registeredID = entityManager.RegisterPreparedPlayer(std::move(player));
    if (registeredID == 0)
    {
        result.AddIssue(WorldPlayerPersistenceIssueSeverity::ERROR,
                        WorldPlayerPersistenceIssueCode::ENTITY_REGISTRATION_FAILED,
                        candidateID, "Prepared Player registration failed");
        return result;
    }

    result.SetPlayerEntityID(registeredID);
    result.SetStatus(fileStatus == PlayerSaveFileLoadStatus::NOT_FOUND
                         ? WorldPlayerLoadStatus::CREATED_NEW
                         : WorldPlayerLoadStatus::LOADED_EXISTING);
    return result;
}

WorldPlayerLoadResult World::TryRegisterLoadedPlayer(
    PlayerSaveFileLoadResult fileLoadResult,
    const PlayerSaveData &saveData,
    PlayerReconstructionFunction reconstructPlayer)
{
    WorldPlayerLoadResult result;
    result.SetFileLoadResult(std::move(fileLoadResult));
    const int candidateID = entityManager.GetNextEntityIDCandidate();
    PlayerSaveValidationReport validationReport;
    std::unique_ptr<Player> player = reconstructPlayer == nullptr
        ? nullptr
        : reconstructPlayer(candidateID, saveData, validationReport);
    result.SetReconstructionValidationReport(validationReport);
    if (player == nullptr)
    {
        result.AddIssue(WorldPlayerPersistenceIssueSeverity::ERROR,
                        WorldPlayerPersistenceIssueCode::PLAYER_RECONSTRUCTION_FAILED,
                        candidateID, "Player reconstruction failed");
        return result;
    }

    const int savedX = player->GetPosition().GetX();
    const int savedY = player->GetPosition().GetY();
    if (!map.IsValidPosition(savedX, savedY))
    {
        if (!map.IsValidPosition(DevelopmentWorldContent::PlayerSpawnX,
                                 DevelopmentWorldContent::PlayerSpawnY))
        {
            result.AddIssue(WorldPlayerPersistenceIssueSeverity::ERROR,
                            WorldPlayerPersistenceIssueCode::DEVELOPMENT_SPAWN_INVALID,
                            candidateID, "Development Player spawn is invalid");
            return result;
        }

        const bool outOfBounds = !map.IsInBounds(savedX, savedY);
        result.AddIssue(WorldPlayerPersistenceIssueSeverity::WARNING,
                        outOfBounds
                            ? WorldPlayerPersistenceIssueCode::SAVED_POSITION_OUT_OF_BOUNDS
                            : WorldPlayerPersistenceIssueCode::SAVED_POSITION_BLOCKED,
                        candidateID,
                        outOfBounds ? "Saved position is out of bounds; development spawn used"
                                    : "Saved position is blocked; development spawn used");
        result.SetUsedSpawnFallback();
        player->GetPosition().SetPosition(DevelopmentWorldContent::PlayerSpawnX,
                                          DevelopmentWorldContent::PlayerSpawnY);
    }

    const int finalX = player->GetPosition().GetX();
    const int finalY = player->GetPosition().GetY();
    if (!map.IsValidPosition(finalX, finalY))
    {
        result.AddIssue(WorldPlayerPersistenceIssueSeverity::ERROR,
                        WorldPlayerPersistenceIssueCode::DEVELOPMENT_SPAWN_INVALID,
                        candidateID, "Final Player position is invalid");
        return result;
    }

    const int registeredID = entityManager.RegisterPreparedPlayer(std::move(player));
    if (registeredID == 0)
    {
        result.AddIssue(WorldPlayerPersistenceIssueSeverity::ERROR,
                        WorldPlayerPersistenceIssueCode::ENTITY_REGISTRATION_FAILED,
                        candidateID, "Prepared Player registration failed");
        return result;
    }

    result.SetPlayerEntityID(registeredID);
    result.SetStatus(WorldPlayerLoadStatus::LOADED_EXISTING);
    return result;
}

WorldPlayerSaveResult World::SavePlayerToFile(
    int playerEntityID,
    const std::filesystem::path &savePath) const
{
    WorldPlayerSaveResult result;
    if (playerEntityID <= 0)
    {
        result.AddIssue(WorldPlayerPersistenceIssueSeverity::ERROR,
                        WorldPlayerPersistenceIssueCode::INVALID_RUNTIME_ENTITY_ID,
                        playerEntityID, "Runtime entity ID must be positive");
        return result;
    }

    const Entity *entity = entityManager.GetEntityByID(playerEntityID);
    if (entity == nullptr)
    {
        result.AddIssue(WorldPlayerPersistenceIssueSeverity::ERROR,
                        WorldPlayerPersistenceIssueCode::ENTITY_NOT_FOUND,
                        playerEntityID, "Entity was not found");
        return result;
    }
    const Player *player = dynamic_cast<const Player *>(entity);
    if (player == nullptr)
    {
        result.AddIssue(WorldPlayerPersistenceIssueSeverity::ERROR,
                        WorldPlayerPersistenceIssueCode::ENTITY_NOT_PLAYER,
                        playerEntityID, "Entity is not a Player");
        return result;
    }
    if (!player->IsAlive())
    {
        result.AddIssue(WorldPlayerPersistenceIssueSeverity::ERROR,
                        WorldPlayerPersistenceIssueCode::PLAYER_NOT_ALIVE,
                        playerEntityID, "Dead Players cannot be saved");
        return result;
    }

    PlayerSaveFileSaveResult fileResult =
        PlayerSaveFileStore::Save(savePath, PlayerSaveState::Capture(*player));
    const bool saved = fileResult.IsSuccess();
    result.SetFileSaveResult(std::move(fileResult));
    if (!saved)
    {
        result.AddIssue(WorldPlayerPersistenceIssueSeverity::ERROR,
                        WorldPlayerPersistenceIssueCode::FILE_SAVE_FAILED,
                        playerEntityID, "Player save file could not be committed");
        return result;
    }
    result.SetPlayerEntityID(playerEntityID);
    return result;
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

int World::CreateMonster(
    NpcType npcType,
    const NpcSpawnDefinition &spawn)
{
    const NpcSpawnDefinition *authoredSpawn =
        NpcSpawnDatabase::TryGet(spawn.spawnId);
    const NpcDefinition *definition =
        NpcDefinitionDatabase::TryGet(npcType);
    if (definition == nullptr || authoredSpawn == nullptr ||
        spawn.spawnId == NpcSpawnId::NONE ||
        !npcSpawnManager.HasSpawn(spawn.spawnId) ||
        !npcSpawnManager.HasCapacity(spawn.spawnId) ||
        npcType != spawn.npcType ||
        authoredSpawn->npcType != spawn.npcType ||
        authoredSpawn->spawnPosition.GetX() != spawn.spawnPosition.GetX() ||
        authoredSpawn->spawnPosition.GetY() != spawn.spawnPosition.GetY() ||
        authoredSpawn->wanderRadius != spawn.wanderRadius ||
        authoredSpawn->wanderIntervalTicks != spawn.wanderIntervalTicks ||
        authoredSpawn->maximumActiveCount != spawn.maximumActiveCount ||
        authoredSpawn->respawns != spawn.respawns ||
        definition->kind != NpcKind::MONSTER || !definition->combat.has_value() ||
        definition->name.empty() ||
        definition->combat->ratings.attackAccuracy < 0 ||
        definition->combat->ratings.meleeStrength < 0 ||
        definition->combat->ratings.defence < 0 ||
        definition->combat->ratings.maximumHealth <= 0 ||
        definition->combat->attackDurationTicks <= 0 ||
        definition->combat->respawnDelayTicks < 0 ||
        RewardTableRegistry::TryGetRewardTable(
            definition->combat->rewardTableType) == nullptr ||
        !map.IsValidPosition(
            spawn.spawnPosition.GetX(),
            spawn.spawnPosition.GetY()) ||
        spawn.wanderRadius < 0 ||
        (spawn.wanderRadius > 0 && spawn.wanderIntervalTicks <= 0) ||
        (spawn.wanderRadius == 0 && spawn.wanderIntervalTicks != 0) ||
        spawn.maximumActiveCount <= 0 ||
        (spawn.respawns && definition->combat->respawnDelayTicks <= 0))
    {
        return 0;
    }

    if (definition->combat->aggression.has_value())
    {
        const MonsterAggressionDefinition &aggression =
            definition->combat->aggression.value();
        if (aggression.detectionRadius <= 0 ||
            aggression.leashRadius < aggression.detectionRadius)
        {
            return 0;
        }
    }

    const int monsterId = entityManager.CreateMonster(
        spawn.spawnPosition.GetX(),
        spawn.spawnPosition.GetY(),
        definition->combat->ratings,
        definition->combat->rewardTableType,
        spawn.respawns
            ? std::optional<MonsterRespawnDefinition>{
                  MonsterRespawnDefinition{
                      definition->combat->respawnDelayTicks}}
            : std::nullopt,
        definition->combat->aggression,
        definition->type,
        definition->combat->attackDurationTicks,
        spawn.spawnId,
        spawn.wanderRadius,
        spawn.wanderIntervalTicks);
    if (monsterId == 0)
        return 0;
    bool registered = false;
    try
    {
        registered = npcSpawnManager.RegisterEntity(
            spawn.spawnId, monsterId, currentTick);
    }
    catch (...)
    {
        entityManager.RollbackLastCreatedEntity(monsterId);
        throw;
    }
    if (!registered)
    {
        entityManager.RollbackLastCreatedEntity(monsterId);
        return 0;
    }
    return monsterId;
}

int World::CreateNpc(NpcType npcType, const NpcSpawnDefinition &spawn)
{
    const NpcDefinition *definition = NpcDefinitionDatabase::TryGet(npcType);
    const NpcSpawnDefinition *authored = NpcSpawnDatabase::TryGet(spawn.spawnId);
    if (definition == nullptr || authored == nullptr || definition->kind != NpcKind::FRIENDLY ||
        definition->combat.has_value() || npcType != spawn.npcType ||
        authored->npcType != npcType || authored->spawnId != spawn.spawnId ||
        authored->spawnPosition.GetX() != spawn.spawnPosition.GetX() ||
        authored->spawnPosition.GetY() != spawn.spawnPosition.GetY() ||
        authored->wanderRadius != spawn.wanderRadius ||
        authored->wanderIntervalTicks != spawn.wanderIntervalTicks ||
        authored->maximumActiveCount != spawn.maximumActiveCount ||
        authored->respawns != spawn.respawns || spawn.respawns ||
        !npcSpawnManager.HasCapacity(spawn.spawnId) ||
        !map.IsValidPosition(spawn.spawnPosition.GetX(), spawn.spawnPosition.GetY()))
        return 0;

    const int id = entityManager.CreateNPC(spawn.spawnPosition.GetX(),
        spawn.spawnPosition.GetY(), npcType, spawn.spawnId);
    if (id == 0) return 0;
    if (!npcSpawnManager.RegisterEntity(spawn.spawnId, id, currentTick))
    {
        entityManager.RollbackLastCreatedEntity(id);
        return 0;
    }
    return id;
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
    meleeEngagementSystem.ClearEngagement(entityID);
    dialogueSystem.CancelActor(entityID);
    shopSystem.CancelActor(entityID);
    interactionSystem.RequestInteraction(
        entityID,
        resourceID,
        InteractionTargetType::RESOURCE,
        entityManager,
        objectManager);
}

void World::QueueStationInteraction(
    int entityID,
    int stationID)
{
    meleeEngagementSystem.ClearEngagement(entityID);
    dialogueSystem.CancelActor(entityID);
    shopSystem.CancelActor(entityID);
    interactionSystem.RequestInteraction(
        entityID,
        stationID,
        InteractionTargetType::STATION,
        entityManager,
        objectManager);
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

    interactionSystem.ClearInteraction(attackerEntityID);

    if (!meleeEngagementSystem.RequestEngagement(
            attackerEntityID,
            defenderEntityID,
            durationTicks,
            entityManager))
    {
        return false;
    }

    dialogueSystem.CancelActor(attackerEntityID);
    shopSystem.CancelActor(attackerEntityID);

    movementSystem.CancelMovement(attackerEntityID);

    if (isAdjacent)
    {
        bool started = TryStartMeleeEngagement(
            attackerEntityID,
            defenderEntityID,
            durationTicks);

        meleeEngagementSystem.ClearEngagement(
            attackerEntityID);

        return started;
    }

    movementSystem.QueueDestination(
        attackerEntityID,
        Position(destination->first, destination->second),
        entityManager,
        map);

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

    const auto interaction = interactionSystem.GetInteraction(entityID);
    if (interaction.has_value() && interaction->targetType ==
                                       InteractionTargetType::RESOURCE)
    {
        interactionSystem.ClearInteraction(entityID);
    }
}

void World::ClearPendingResourceInteraction(
    int entityID)
{
    const auto interaction = interactionSystem.GetInteraction(entityID);
    if (interaction.has_value() && interaction->targetType ==
                                       InteractionTargetType::RESOURCE)
    {
        interactionSystem.ClearInteraction(entityID);
    }
}

void World::ClearPendingStationInteraction(
    int entityID)
{
    const auto interaction = interactionSystem.GetInteraction(entityID);
    if (interaction.has_value() && interaction->targetType ==
                                       InteractionTargetType::STATION)
    {
        interactionSystem.ClearInteraction(entityID);
    }
}

bool World::HasPendingMeleeEngagement(
    int entityID) const
{
    return meleeEngagementSystem.HasEngagement(entityID);
}

void World::ProcessMeleeEngagementSystem()
{
    const std::vector<MeleeEngagementIntent> intents =
        meleeEngagementSystem.Evaluate(entityManager);

    for (const MeleeEngagementIntent &intent : intents)
    {
        if (intent.type ==
            MeleeEngagementIntentType::CLEAR_ENGAGEMENT)
        {
            continue;
        }

        if (actionManager.HasActionForEntity(intent.attackerEntityID))
        {
            meleeEngagementSystem.ClearEngagement(
                intent.attackerEntityID);
            continue;
        }

        if (intent.type == MeleeEngagementIntentType::ATTACK_TARGET)
        {
            const bool started = TryStartMeleeEngagement(
                intent.attackerEntityID,
                intent.targetEntityID,
                intent.durationTicks);
            meleeEngagementSystem.ClearEngagement(
                intent.attackerEntityID);
            (void)started;
            continue;
        }

        if (movementSystem.HasMovement(intent.attackerEntityID))
        {
            continue;
        }

        Entity *attackerEntity = entityManager.GetEntityByID(
            intent.attackerEntityID);
        Entity *targetEntity = entityManager.GetEntityByID(
            intent.targetEntityID);
        if (attackerEntity == nullptr || targetEntity == nullptr)
        {
            meleeEngagementSystem.ClearEngagement(intent.attackerEntityID);
            continue;
        }

        const std::optional<std::pair<int, int>> destination =
            FindMeleeApproachTile(
                attackerEntity->GetPosition().GetX(),
                attackerEntity->GetPosition().GetY(),
                targetEntity->GetPosition().GetX(),
                targetEntity->GetPosition().GetY());

        if (!destination.has_value() ||
            !movementSystem.QueueDestination(
                intent.attackerEntityID,
                Position(destination->first, destination->second),
                entityManager,
                map))
        {
            meleeEngagementSystem.ClearEngagement(intent.attackerEntityID);
        }
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

bool World::HasActiveStation(
    int entityID) const
{
    return activeStations.contains(entityID);
}

bool World::IsStationUsableForRecipeValidation(
    int entityID,
    StationType requiredStationType) const
{
    return IsStationUsable(
        entityID,
        requiredStationType);
}

bool World::IsStationUsable(
    int entityID,
    StationType requiredStationType) const
{
    auto activeStationIterator =
        activeStations.find(entityID);

    if (activeStationIterator ==
        activeStations.end())
    {
        return false;
    }

    const Entity *entity =
        entityManager.GetEntityByID(
            entityID);

    const CraftingStation *station =
        objectManager.GetStationByID(
            activeStationIterator->second);

    if (entity == nullptr ||
        station == nullptr)
    {
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
        return false;
    }

    return true;
}

void World::CleanupInvalidActiveStationEntry(
    int entityID,
    StationType requiredStationType)
{
    auto activeStationIterator =
        activeStations.find(entityID);

    if (activeStationIterator ==
        activeStations.end())
    {
        return;
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
        return;
    }

    if (station->GetStationType() !=
        requiredStationType)
    {
        return;
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
    }
}

bool World::CanUseStation(
    int entityID,
    StationType requiredStationType)
{
    bool usable =
        IsStationUsable(
            entityID,
            requiredStationType);

    if (!usable)
    {
        CleanupInvalidActiveStationEntry(
            entityID,
            requiredStationType);
    }

    return usable;
}

void World::CloseStationInteraction(
    int entityID)
{
    ClearPendingStationInteraction(entityID);

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
    if (!meleeEngagementSystem.RequestEngagement(
            attackerEntityID,
            defenderEntityID,
            durationTicks,
            entityManager))
    {
        return false;
    }

    bool started = TryStartMeleeAction(
        attackerEntityID,
        defenderEntityID,
        durationTicks,
        true);

    if (!started)
    {
        meleeEngagementSystem.ClearEngagement(attackerEntityID);
        return false;
    }

    meleeEngagementSystem.ClearEngagement(attackerEntityID);

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
    Pathfinder pathfinder;

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
        monsterRespawnEventIDs.size());
}

bool World::HasScheduledMonsterRespawn(
    int monsterEntityID) const
{
    return monsterRespawnEventIDs.find(
               monsterEntityID) !=
           monsterRespawnEventIDs.end();
}

std::optional<int> World::GetScheduledMonsterRespawnTick(
    int monsterEntityID) const
{
    auto iterator =
        monsterRespawnEventIDs.find(
            monsterEntityID);

    if (iterator ==
        monsterRespawnEventIDs.end())
    {
        return std::nullopt;
    }

    std::optional<ScheduledEvent> event =
        tickScheduler.GetScheduledEvent(iterator->second);
    return event.has_value()
               ? std::optional<int>(event->dueTick)
               : std::nullopt;
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
    publishedNpcTalkEvents.clear();
    publishedShopOpenedEvents.clear();
    publishedShopTransactionEvents.clear();
    respawnedMonsterEntityIDsThisTick.clear();

    publishedCommandProcessingResults.clear();
    pendingCommandProcessingResults.clear();
    ProcessQueuedCommands();

    currentTick++;

    ProcessScheduledEvents();

    UpdateMeleeCombatFeedback();
    ProcessDeadCombatantCleanup();

    ProcessMovementSystem();
    ProcessInteractionSystem();

    objectManager.Update();

    std::vector<Action> completedActions =
        actionManager.Update(currentTick);

    ProcessCompletedActions(completedActions);

    entityManager.Update(*this);
    ProcessMovementRequests();
    ProcessAggressiveMonsters();
    ProcessMeleeEngagementSystem();
    ProcessEntityDeathRewards();
    ScheduleMonsterRespawnsFromDeathEvents();
    PublishActionLifecycleEvents();
    TickPlayerStatusEffects();

    publishedCommandProcessingResults =
        std::move(pendingCommandProcessingResults);
    publishedNpcTalkEvents = std::move(pendingNpcTalkEvents);
    pendingNpcTalkEvents.clear();
    publishedShopOpenedEvents = std::move(pendingShopOpenedEvents);
    pendingShopOpenedEvents.clear();
    publishedShopTransactionEvents = std::move(pendingShopTransactionEvents);
    pendingShopTransactionEvents.clear();
}

const std::vector<NpcTalkEvent> &World::GetNpcTalkEvents() const
{
    return publishedNpcTalkEvents;
}

const ActiveDialogueSession *World::GetActiveDialogueSession(int actorEntityID) const
{
    return dialogueSystem.GetSession(actorEntityID);
}

const ActiveShopSession *World::GetActiveShopSession(int actorEntityID) const
{
    return shopSystem.GetSession(actorEntityID);
}

const std::vector<ShopOpenedEvent> &World::GetShopOpenedEvents() const
{
    return publishedShopOpenedEvents;
}

const std::vector<ShopTransactionEvent> &World::GetShopTransactionEvents() const
{
    return publishedShopTransactionEvents;
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
                        interactionSystem.ClearInteraction(data.actorEntityID);
                        dialogueSystem.CancelActor(data.actorEntityID);
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
                        dialogueSystem.CancelActor(data.actorEntityID);
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
                    std::is_same_v<CommandType, NpcInteractionCommand>)
                {
                    NPC *npc = dynamic_cast<NPC *>(entityManager.GetEntityByID(data.targetNpcEntityID));
                    if (npc == nullptr || !IsValidNpcInteractionType(data.interactionType))
                        resultCode = CommandResultCode::INVALID_COMMAND_DATA;
                    else if (actor->IsAlive() && interactionSystem.RequestNpcInteraction(
                                 data.actorEntityID, data.targetNpcEntityID,
                                 data.interactionType, entityManager))
                    {
                        const Position &actorPosition = actor->GetPosition();
                        const Position &targetPosition = npc->GetPosition();
                        if (std::abs(actorPosition.GetX() - targetPosition.GetX()) > 1 ||
                            std::abs(actorPosition.GetY() - targetPosition.GetY()) > 1)
                        {
                            auto approach = FindMeleeApproachTile(actorPosition.GetX(), actorPosition.GetY(),
                                targetPosition.GetX(), targetPosition.GetY());
                            if (!approach.has_value() || !movementSystem.QueueDestination(data.actorEntityID,
                                    Position{approach->first, approach->second}, entityManager, map))
                            {
                                interactionSystem.ClearInteraction(data.actorEntityID);
                                resultCode = CommandResultCode::GAMEPLAY_REJECTED;
                                return;
                            }
                        }
                        dialogueSystem.CancelActor(data.actorEntityID);
                        shopSystem.CancelActor(data.actorEntityID);
                        CloseStationInteraction(data.actorEntityID);
                        CancelActionsForEntity(data.actorEntityID, ActionCancelReason::PLAYER_MOVED);
                        if (std::abs(actorPosition.GetX() - targetPosition.GetX()) <= 1 &&
                            std::abs(actorPosition.GetY() - targetPosition.GetY()) <= 1)
                            movementSystem.CancelMovement(data.actorEntityID);
                        resultCode = CommandResultCode::ACCEPTED;
                    }
                }
                else if constexpr (
                    std::is_same_v<CommandType, DialogueContinueCommand>)
                {
                    const ActiveDialogueSession *active =
                        dialogueSystem.GetSession(data.actorEntityID);
                    if (!actor->IsAlive())
                    {
                        dialogueSystem.CancelActor(data.actorEntityID);
                    }
                    else if (active != nullptr && data.sessionId != InvalidDialogueSessionId &&
                             active->sessionId == data.sessionId)
                    {
                        const ActiveDialogueSession session = *active;
                        NPC *npc = dynamic_cast<NPC *>(entityManager.GetEntityByID(session.npcEntityID));
                        const NpcDefinition *npcDefinition = npc == nullptr ? nullptr :
                            NpcDefinitionDatabase::TryGet(npc->GetNpcType());
                        const DialogueDefinition *dialogue =
                            DialogueDefinitionDatabase::TryGet(session.dialogueId);
                        const DialogueNodeDefinition *currentNode =
                            DialogueDefinitionDatabase::TryGetNode(session.dialogueId, session.currentNodeId);
                        const bool adjacent = npc != nullptr &&
                            std::abs(actor->GetPosition().GetX() - npc->GetPosition().GetX()) <= 1 &&
                            std::abs(actor->GetPosition().GetY() - npc->GetPosition().GetY()) <= 1;
                        if (npc == nullptr || npc->GetNpcType() != session.npcType ||
                            npcDefinition == nullptr || npcDefinition->kind != NpcKind::FRIENDLY ||
                            std::find(npcDefinition->interactions.begin(),
                                      npcDefinition->interactions.end(),
                                      NpcInteractionType::TALK) == npcDefinition->interactions.end() ||
                            npcDefinition->dialogueId != session.dialogueId || dialogue == nullptr ||
                            currentNode == nullptr || !IsValidDialogueNodeKind(currentNode->kind) || !adjacent)
                        {
                            dialogueSystem.CancelActor(data.actorEntityID);
                        }
                        else if (currentNode->kind == DialogueNodeKind::CONTINUE &&
                                 currentNode->nextNodeId.has_value())
                        {
                            const DialogueNodeDefinition *nextNode =
                                DialogueDefinitionDatabase::TryGetNode(
                                    session.dialogueId, *currentNode->nextNodeId);
                            if (nextNode == nullptr)
                                dialogueSystem.CancelActor(data.actorEntityID);
                            else if (session.lastAdvancedTick != currentTick + 1)
                            {
                                ActiveDialogueSession advanced = session;
                                advanced.currentNodeId = nextNode->id;
                                advanced.lastAdvancedTick = currentTick + 1;
                                if (PublishDialogueNode(advanced, *nextNode) &&
                                    dialogueSystem.CommitContinuation(
                                        data.actorEntityID, data.sessionId,
                                        nextNode->id, currentTick + 1))
                                {
                                    if (nextNode->kind == DialogueNodeKind::TERMINAL)
                                        dialogueSystem.Close(data.actorEntityID, data.sessionId);
                                    resultCode = CommandResultCode::ACCEPTED;
                                }
                            }
                        }
                    }
                }
                else if constexpr (
                    std::is_same_v<CommandType, DialogueChooseCommand>)
                {
                    const ActiveDialogueSession *active =
                        dialogueSystem.GetSession(data.actorEntityID);
                    if (!actor->IsAlive())
                    {
                        dialogueSystem.CancelActor(data.actorEntityID);
                    }
                    else if (active != nullptr && data.sessionId != InvalidDialogueSessionId &&
                             active->sessionId == data.sessionId)
                    {
                        const ActiveDialogueSession session = *active;
                        NPC *npc = dynamic_cast<NPC *>(entityManager.GetEntityByID(session.npcEntityID));
                        const NpcDefinition *npcDefinition = npc == nullptr ? nullptr :
                            NpcDefinitionDatabase::TryGet(npc->GetNpcType());
                        const DialogueDefinition *dialogue =
                            DialogueDefinitionDatabase::TryGet(session.dialogueId);
                        const DialogueNodeDefinition *currentNode =
                            DialogueDefinitionDatabase::TryGetNode(session.dialogueId, session.currentNodeId);
                        const bool adjacent = npc != nullptr &&
                            std::abs(actor->GetPosition().GetX() - npc->GetPosition().GetX()) <= 1 &&
                            std::abs(actor->GetPosition().GetY() - npc->GetPosition().GetY()) <= 1;
                        if (npc == nullptr || npc->GetNpcType() != session.npcType ||
                            npcDefinition == nullptr || npcDefinition->kind != NpcKind::FRIENDLY ||
                            std::find(npcDefinition->interactions.begin(),
                                      npcDefinition->interactions.end(),
                                      NpcInteractionType::TALK) == npcDefinition->interactions.end() ||
                            npcDefinition->dialogueId != session.dialogueId || dialogue == nullptr ||
                            currentNode == nullptr || !IsValidDialogueNodeKind(currentNode->kind) || !adjacent)
                        {
                            dialogueSystem.CancelActor(data.actorEntityID);
                        }
                        else if (currentNode->kind == DialogueNodeKind::CHOICE &&
                                 session.lastAdvancedTick != currentTick + 1)
                        {
                            const DialogueChoiceDefinition *choice =
                                DialogueDefinitionDatabase::TryGetChoice(
                                    session.dialogueId, session.currentNodeId, data.choiceId);
                            const DialogueNodeDefinition *destination = choice == nullptr ? nullptr :
                                DialogueDefinitionDatabase::TryGetNode(
                                    session.dialogueId, choice->destinationNodeId);
                            if (choice != nullptr && destination != nullptr)
                            {
                                ActiveDialogueSession advanced = session;
                                advanced.currentNodeId = destination->id;
                                advanced.lastAdvancedTick = currentTick + 1;
                                if (PublishDialogueNode(advanced, *destination) &&
                                    dialogueSystem.CommitChoice(
                                        data.actorEntityID, data.sessionId, data.choiceId,
                                        destination->id, currentTick + 1))
                                {
                                    if (destination->kind == DialogueNodeKind::TERMINAL)
                                        dialogueSystem.Close(data.actorEntityID, data.sessionId);
                                    resultCode = CommandResultCode::ACCEPTED;
                                }
                            }
                        }
                    }
                }
                else if constexpr (
                    std::is_same_v<CommandType, DialogueCloseCommand>)
                {
                    if (actor->IsAlive() && dialogueSystem.Close(
                            data.actorEntityID, data.sessionId))
                        resultCode = CommandResultCode::ACCEPTED;
                }
                else if constexpr (
                    std::is_same_v<CommandType, ShopBuyCommand>)
                {
                    if (TryProcessShopTransaction(data.actorEntityID,
                            data.shopSessionId, data.itemType, data.quantity,
                            ShopTransactionType::BUY))
                        resultCode = CommandResultCode::ACCEPTED;
                }
                else if constexpr (
                    std::is_same_v<CommandType, ShopSellCommand>)
                {
                    if (TryProcessShopTransaction(data.actorEntityID,
                            data.shopSessionId, data.itemType, data.quantity,
                            ShopTransactionType::SELL))
                        resultCode = CommandResultCode::ACCEPTED;
                }
                else if constexpr (
                    std::is_same_v<CommandType, ShopCloseCommand>)
                {
                    if (actor->IsAlive() && shopSystem.Close(
                            data.actorEntityID, data.shopSessionId))
                        resultCode = CommandResultCode::ACCEPTED;
                }
                else if constexpr (std::is_same_v<CommandType,QuestAcceptCommand> || std::is_same_v<CommandType,QuestCompleteCommand>)
                {
                    Player* player=dynamic_cast<Player*>(actor);
                    const ActiveDialogueSession* session=dialogueSystem.GetSession(data.actorEntityID);
                    NPC* npc=dynamic_cast<NPC*>(entityManager.GetEntityByID(data.npcEntityID));
                    const NpcDefinition* npcDefinition=npc==nullptr?nullptr:NpcDefinitionDatabase::TryGet(npc->GetNpcType());
                    const bool validGuide=npc!=nullptr&&npc->GetNpcType()==NpcType::DEVELOPMENT_GUIDE&&npcDefinition!=nullptr&&npcDefinition->kind==NpcKind::FRIENDLY;
                    const bool adjacent=validGuide&&std::abs(actor->GetPosition().GetX()-npc->GetPosition().GetX())<=1&&std::abs(actor->GetPosition().GetY()-npc->GetPosition().GetY())<=1;
                    if(player&&player->IsAlive()&&session&&validGuide&&session->sessionId==data.dialogueSessionId&&session->npcEntityID==data.npcEntityID&&session->npcType==npc->GetNpcType()&&session->dialogueId==DialogueId::DEVELOPMENT_GUIDE_INTRO&&session->currentNodeId==DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME&&data.questId==QuestId::GATHERING_BASICS&&adjacent){bool ok=false;if constexpr(std::is_same_v<CommandType,QuestAcceptCommand>)ok=QuestSystem::Accept(player->GetQuestJournal());else ok=QuestSystem::Complete(player->GetQuestJournal(),player->GetInventory());if(ok)resultCode=CommandResultCode::ACCEPTED;}
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

bool World::IsValidMonsterAggressionTarget(
    const Monster &monster,
    int targetEntityID)
{
    return MonsterAISystem().IsValidTarget(
        monster,
        entityManager,
        targetEntityID);
}

void World::ProcessAggressiveMonsters()
{
    const MonsterAISystem monsterAISystem;

    for (const std::unique_ptr<Entity> &entity :
         entityManager.GetEntities())
    {
        Monster *monster =
            dynamic_cast<Monster *>(entity.get());

        if (monster == nullptr)
        {
            continue;
        }

        const bool respawnSuppressed =
            HasScheduledMonsterRespawn(monster->GetID()) ||
            respawnedMonsterEntityIDsThisTick.contains(monster->GetID());

        const MonsterAIIntent combatIntent = monsterAISystem.Evaluate(
            *monster, entityManager, respawnSuppressed);
        ExecuteMonsterAIIntent(combatIntent);
        if (combatIntent.type == MonsterAIIntentType::NONE)
            TryProcessIdleWander(*monster);
    }
}

void World::TryProcessIdleWander(Monster &monster)
{
    if (!monster.IsAlive() || monster.GetWanderRadius() <= 0 ||
        monster.GetAggressionTargetEntityID() != Monster::InvalidAggressionTargetEntityID ||
        actionManager.HasActionForEntity(monster.GetID()) ||
        HasPendingMeleeEngagement(monster.GetID()) ||
        movementSystem.HasMovement(monster.GetID()) ||
        HasScheduledMonsterRespawn(monster.GetID()) ||
        respawnedMonsterEntityIDsThisTick.contains(monster.GetID()) ||
        Distance::Calculate(monster.GetPosition().GetX(), monster.GetPosition().GetY(),
                            monster.GetOriginalSpawnX(), monster.GetOriginalSpawnY()) >
            monster.GetWanderRadius())
        return;

    const std::optional<int> start =
        npcSpawnManager.BeginWanderAttempt(monster.GetID(), currentTick);
    if (!start.has_value())
        return;

    static constexpr int DirectionCount = 4;
    static constexpr int ChangeX[DirectionCount] = {1, 0, -1, 0};
    static constexpr int ChangeY[DirectionCount] = {0, 1, 0, -1};
    for (int offset = 0; offset < DirectionCount; ++offset)
    {
        const int candidate = (start.value() + offset) % DirectionCount;
        const Position destination(
            monster.GetPosition().GetX() + ChangeX[candidate],
            monster.GetPosition().GetY() + ChangeY[candidate]);
        if (!map.IsValidPosition(destination.GetX(), destination.GetY()) ||
            Distance::Calculate(destination.GetX(), destination.GetY(),
                                monster.GetOriginalSpawnX(), monster.GetOriginalSpawnY()) >
                monster.GetWanderRadius())
            continue;

        ExecuteMonsterAIIntent(MonsterAIIntent{
            MonsterAIIntentType::WANDER, monster.GetID(),
            Monster::InvalidAggressionTargetEntityID, destination});
        return;
    }
}

void World::ExecuteMonsterAIIntent(
    const MonsterAIIntent &intent)
{
    Monster *monster = dynamic_cast<Monster *>(
        entityManager.GetEntityByID(intent.monsterEntityID));
    if (monster == nullptr)
    {
        return;
    }

    switch (intent.type)
    {
    case MonsterAIIntentType::NONE:
        return;
    case MonsterAIIntentType::CLEAR_TARGET:
        monster->ClearAggressionTargetEntityID();
        CancelActionsForEntity(
            intent.monsterEntityID,
            intent.clearReason == MonsterAIClearReason::LEASH_VIOLATED
                ? ActionCancelReason::OUT_OF_RANGE
                : ActionCancelReason::TARGET_MISSING);
        meleeEngagementSystem.ClearEngagement(intent.monsterEntityID);
        ClearPendingMovementForEntity(intent.monsterEntityID);
        return;
    case MonsterAIIntentType::ATTACK_TARGET:
    {
        const int previousTargetID =
            monster->GetAggressionTargetEntityID();
        if (previousTargetID != Monster::InvalidAggressionTargetEntityID &&
            previousTargetID != intent.targetEntityID)
        {
            CancelActionsForEntity(
                intent.monsterEntityID,
                ActionCancelReason::TARGET_MISSING);
            meleeEngagementSystem.ClearEngagement(intent.monsterEntityID);
        }
        monster->SetAggressionTargetEntityID(intent.targetEntityID);
        ClearPendingMovementForEntity(intent.monsterEntityID);
        if (!actionManager.HasActionForEntity(intent.monsterEntityID) &&
            !HasPendingMeleeEngagement(intent.monsterEntityID))
        {
            TryStartMeleeEngagement(
                intent.monsterEntityID,
                intent.targetEntityID,
                monster->GetAttackDurationTicks());
        }
        return;
    }
    case MonsterAIIntentType::CHASE_TARGET:
    {
        const int previousTargetID =
            monster->GetAggressionTargetEntityID();
        const bool acquiredTarget = previousTargetID != intent.targetEntityID;
        if (previousTargetID != Monster::InvalidAggressionTargetEntityID &&
            acquiredTarget)
        {
            CancelActionsForEntity(
                intent.monsterEntityID,
                ActionCancelReason::TARGET_MISSING);
            meleeEngagementSystem.ClearEngagement(intent.monsterEntityID);
        }
        monster->SetAggressionTargetEntityID(intent.targetEntityID);
        if (acquiredTarget)
        {
            ClearPendingMovementForEntity(intent.monsterEntityID);
        }

        if (actionManager.HasActionForEntity(intent.monsterEntityID) ||
            HasPendingMeleeEngagement(intent.monsterEntityID) ||
            movementSystem.HasMovement(intent.monsterEntityID))
        {
            return;
        }

        const std::optional<std::pair<int, int>> destination =
            FindMeleeApproachTile(
                monster->GetPosition().GetX(),
                monster->GetPosition().GetY(),
                intent.destination.GetX(),
                intent.destination.GetY());
        if (destination.has_value())
        {
            QueueMovementDestination(MovementDestinationRequest(
                intent.monsterEntityID,
                destination->first,
                destination->second));
        }
        return;
    }
    case MonsterAIIntentType::RETURN_HOME:
        if (!actionManager.HasActionForEntity(intent.monsterEntityID) &&
            !HasPendingMeleeEngagement(intent.monsterEntityID) &&
            !movementSystem.HasMovement(intent.monsterEntityID))
        {
            QueueMovementDestination(MovementDestinationRequest(
                intent.monsterEntityID,
                intent.destination.GetX(),
                intent.destination.GetY()));
        }
        return;
    case MonsterAIIntentType::WANDER:
        if (monster->IsAlive() && monster->GetWanderRadius() > 0 &&
            monster->GetAggressionTargetEntityID() == Monster::InvalidAggressionTargetEntityID &&
            !actionManager.HasActionForEntity(intent.monsterEntityID) &&
            !HasPendingMeleeEngagement(intent.monsterEntityID) &&
            !movementSystem.HasMovement(intent.monsterEntityID))
        {
            movementSystem.QueueDestination(
                intent.monsterEntityID, intent.destination, entityManager, map);
        }
        return;
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

    meleeEngagementSystem.ClearEngagement(
        request.GetEntityID());
    interactionSystem.ClearInteraction(request.GetEntityID());
    dialogueSystem.CancelActor(request.GetEntityID());
    shopSystem.CancelActor(request.GetEntityID());

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

    meleeEngagementSystem.ClearEngagement(
        request.GetEntityID());
    dialogueSystem.CancelActor(request.GetEntityID());
    shopSystem.CancelActor(request.GetEntityID());

    movementSystem.QueueDestination(
        request.GetEntityID(),
        Position(
            request.GetDestinationX(),
            request.GetDestinationY()),
        entityManager,
        map);
}

bool World::ClearMovementPath(int entityID)
{
    return movementSystem.CancelMovement(entityID);
}

bool World::HasActiveMovementPath(int entityID) const
{
    return movementSystem.HasMovement(entityID);
}

std::optional<Position> World::GetMovementDestination(int entityID) const
{
    return movementSystem.GetDestination(entityID);
}

void World::ProcessMovementSystem()
{
    const std::vector<MovementOutcome> outcomes =
        movementSystem.Process(entityManager, map);

    // Movement currently has no gameplay event of its own. Resource, station,
    // and melee phases below authoritatively observe the updated positions.
    (void)outcomes;
}
void World::ProcessInteractionSystem()
{
    std::unordered_set<int> actorsWithActiveMovement;
    for (const auto &entity : entityManager.GetEntities())
    {
        if (movementSystem.HasMovement(entity->GetID()))
        {
            actorsWithActiveMovement.insert(entity->GetID());
        }
    }

    const auto intents = interactionSystem.Evaluate(
        entityManager, objectManager, actorsWithActiveMovement);
    for (const InteractionIntent &intent : intents)
    {
        if (intent.type == InteractionIntentType::CLEAR_INTERACTION)
        {
            if (intent.targetType == InteractionTargetType::NPC)
                movementSystem.CancelMovement(intent.actorEntityID);
            continue;
        }

        if (intent.targetType == InteractionTargetType::STATION)
        {
            CraftingStation *station =
                objectManager.GetStationByID(intent.targetObjectID);
            if (station != nullptr)
            {
                activeStations[intent.actorEntityID] = intent.targetObjectID;
                openedStations[intent.actorEntityID] =
                    station->GetStationType();
            }
            continue;
        }

        if (intent.targetType == InteractionTargetType::NPC)
        {
            movementSystem.CancelMovement(intent.actorEntityID);
            if (intent.npcInteractionType == NpcInteractionType::TRADE)
            {
                TryOpenShop(intent);
                continue;
            }
            NPC *npc = dynamic_cast<NPC *>(entityManager.GetEntityByID(intent.targetObjectID));
            const NpcDefinition *definition = npc == nullptr ? nullptr : NpcDefinitionDatabase::TryGet(npc->GetNpcType());
            Player *actor = dynamic_cast<Player *>(entityManager.GetEntityByID(intent.actorEntityID));
            const DialogueDefinition *dialogue = definition == nullptr ? nullptr :
                DialogueDefinitionDatabase::TryGet(definition->dialogueId);
            const DialogueNodeDefinition *startNode = dialogue == nullptr ? nullptr :
                DialogueDefinitionDatabase::TryGetNode(dialogue->id, dialogue->startNodeId);
            if (actor != nullptr && actor->IsAlive() && npc != nullptr && definition != nullptr &&
                definition->kind == NpcKind::FRIENDLY &&
                intent.npcInteractionType == NpcInteractionType::TALK &&
                std::find(definition->interactions.begin(), definition->interactions.end(),
                          NpcInteractionType::TALK) != definition->interactions.end() &&
                dialogue != nullptr && startNode != nullptr)
            {
                shopSystem.CancelActor(intent.actorEntityID);
                dialogueSystem.CancelActor(intent.actorEntityID);
                const DialogueSessionId sessionId = dialogueSystem.Start(
                    intent.actorEntityID, npc->GetID(), npc->GetNpcType(),
                    dialogue->id, startNode->id, currentTick);
                const ActiveDialogueSession *session =
                    dialogueSystem.GetSession(intent.actorEntityID);
                if (sessionId != InvalidDialogueSessionId && session != nullptr)
                {
                    try
                    {
                        if (!PublishDialogueNode(*session, *startNode))
                            dialogueSystem.Close(intent.actorEntityID, sessionId);
                        else if (startNode->kind == DialogueNodeKind::TERMINAL)
                            dialogueSystem.Close(intent.actorEntityID, sessionId);
                    }
                    catch (...)
                    {
                        dialogueSystem.Close(intent.actorEntityID, sessionId);
                        throw;
                    }
                }
            }
            continue;
        }

        ResourceNode *resource =
            objectManager.GetResourceByID(intent.targetObjectID);
        if (resource == nullptr || !resource->IsActive())
        {
            continue;
        }

        ActionValidationResult validation = ValidateGatheringAction(
            intent.actorEntityID, intent.targetObjectID, false);
        if (!validation.valid)
        {
            if (!validation.message.empty())
            {
                Logger::Game(validation.message);
            }
            CancelActionsForEntity(intent.actorEntityID, validation.reason);
            continue;
        }

        Player *player = dynamic_cast<Player *>(
            entityManager.GetEntityByID(intent.actorEntityID));
        const ResourceDefinition &resourceDefinition =
            ResourceDatabase::Get(resource->GetResourceType());
        const ItemDefinition &weaponDefinition = ItemDatabase::Get(
            player->GetEquipment().GetEquippedItem(
                EquipmentSlotType::WEAPON));
        if (!actionManager.HasActionForEntity(intent.actorEntityID))
        {
            StartAction(Action(
                ActionType::GATHERING,
                "Gathering " + resourceDefinition.GetName(),
                weaponDefinition.GetActionDurationTicks(),
                intent.actorEntityID,
                intent.targetObjectID,
                true));
        }
    }
}

bool World::TryOpenShop(const InteractionIntent &intent)
{
    Player *actor = dynamic_cast<Player *>(
        entityManager.GetEntityByID(intent.actorEntityID));
    NPC *npc = dynamic_cast<NPC *>(
        entityManager.GetEntityByID(intent.targetObjectID));
    const NpcDefinition *npcDefinition = npc == nullptr ? nullptr :
        NpcDefinitionDatabase::TryGet(npc->GetNpcType());
    const ShopDefinition *shop = npcDefinition == nullptr ? nullptr :
        ShopDefinitionDatabase::TryGet(npcDefinition->shopId);
    const bool adjacent = actor != nullptr && npc != nullptr &&
        std::abs(actor->GetPosition().GetX() - npc->GetPosition().GetX()) <= 1 &&
        std::abs(actor->GetPosition().GetY() - npc->GetPosition().GetY()) <= 1;
    if (actor == nullptr || !actor->IsAlive() || npc == nullptr ||
        npcDefinition == nullptr || npcDefinition->kind != NpcKind::FRIENDLY ||
        intent.npcInteractionType != NpcInteractionType::TRADE ||
        std::find(npcDefinition->interactions.begin(), npcDefinition->interactions.end(),
                  NpcInteractionType::TRADE) == npcDefinition->interactions.end() ||
        npcDefinition->shopId == ShopId::NONE || shop == nullptr || !adjacent)
        return false;

    ShopOpenedEvent event{intent.actorEntityID, npc->GetID(), npc->GetNpcType(),
        InvalidShopSessionId, shop->id, shop->name, shop->currencyItemType, {}};
    event.entries.reserve(shop->entries.size());
    for (const ShopEntryDefinition &entry : shop->entries)
        event.entries.push_back({entry.itemType, entry.buyPrice, entry.sellPrice});

    // Reserve before replacing interface state so event-buffer allocation
    // failure cannot leave an invisible newly opened session.
    if (pendingShopOpenedEvents.size() == pendingShopOpenedEvents.max_size())
        return false;
    pendingShopOpenedEvents.reserve(pendingShopOpenedEvents.size() + 1);

    shopSystem.CancelActor(intent.actorEntityID);
    dialogueSystem.CancelActor(intent.actorEntityID);
    CloseStationInteraction(intent.actorEntityID);
    const ShopSessionId sessionId = shopSystem.Start(intent.actorEntityID,
        npc->GetID(), npc->GetNpcType(), shop->id, currentTick);
    if (sessionId == InvalidShopSessionId)
        return false;
    event.sessionId = sessionId;
    pendingShopOpenedEvents.push_back(std::move(event));
    return true;
}

const ActiveShopSession *World::ValidateActiveShopSession(
    int actorEntityID, ShopSessionId sessionId, bool closeLifecycleFailure)
{
    if (sessionId == InvalidShopSessionId)
        return nullptr;
    const ActiveShopSession *session = shopSystem.GetSession(actorEntityID);
    if (session == nullptr || session->sessionId != sessionId)
        return nullptr;
    Player *actor = dynamic_cast<Player *>(entityManager.GetEntityByID(actorEntityID));
    NPC *npc = dynamic_cast<NPC *>(entityManager.GetEntityByID(session->npcEntityID));
    const NpcDefinition *definition = npc == nullptr ? nullptr :
        NpcDefinitionDatabase::TryGet(npc->GetNpcType());
    const bool adjacent = actor != nullptr && npc != nullptr &&
        std::abs(actor->GetPosition().GetX() - npc->GetPosition().GetX()) <= 1 &&
        std::abs(actor->GetPosition().GetY() - npc->GetPosition().GetY()) <= 1;
    const bool valid = actor != nullptr && actor->IsAlive() && npc != nullptr &&
        npc->GetID() == session->npcEntityID && npc->GetNpcType() == session->npcType &&
        definition != nullptr && definition->kind == NpcKind::FRIENDLY &&
        std::find(definition->interactions.begin(), definition->interactions.end(),
                  NpcInteractionType::TRADE) != definition->interactions.end() &&
        definition->shopId == session->shopId &&
        ShopDefinitionDatabase::TryGet(session->shopId) != nullptr && adjacent;
    if (!valid)
    {
        if (closeLifecycleFailure)
            shopSystem.CancelActor(actorEntityID);
        return nullptr;
    }
    return session;
}

bool World::TryProcessShopTransaction(int actorEntityID, ShopSessionId sessionId,
    ItemType itemType, int quantity, ShopTransactionType transactionType)
{
    const ActiveShopSession *active = ValidateActiveShopSession(
        actorEntityID, sessionId, true);
    if (active == nullptr || quantity <= 0 || itemType == ItemType::NONE)
        return false;
    const ActiveShopSession session = *active;
    const ShopDefinition *shop = ShopDefinitionDatabase::TryGet(session.shopId);
    const ShopEntryDefinition *entry =
        ShopDefinitionDatabase::TryGetEntry(session.shopId, itemType);
    if (shop == nullptr || entry == nullptr || itemType == shop->currencyItemType)
        return false;
    const std::optional<int> price = transactionType == ShopTransactionType::BUY
        ? entry->buyPrice : entry->sellPrice;
    if (!price.has_value() || *price <= 0)
        return false;
    const std::int64_t wideTotal = static_cast<std::int64_t>(*price) * quantity;
    if (wideTotal <= 0 || wideTotal > std::numeric_limits<int>::max())
        return false;
    const int totalPrice = static_cast<int>(wideTotal);
    Player *actor = dynamic_cast<Player *>(entityManager.GetEntityByID(actorEntityID));
    if (actor == nullptr)
        return false;
    const std::vector<ItemAmount> removals = transactionType == ShopTransactionType::BUY
        ? std::vector<ItemAmount>{{shop->currencyItemType, totalPrice}}
        : std::vector<ItemAmount>{{itemType, quantity}};
    const std::vector<ItemAmount> additions = transactionType == ShopTransactionType::BUY
        ? std::vector<ItemAmount>{{itemType, quantity}}
        : std::vector<ItemAmount>{{shop->currencyItemType, totalPrice}};
    ShopTransactionEvent event{actorEntityID, session.npcEntityID,
        session.sessionId, session.shopId, transactionType, itemType, quantity,
        shop->currencyItemType, *price, totalPrice};
    // Reserve before inventory commit. ShopTransactionEvent is value-only and
    // nothrow-movable, so insertion cannot allocate after the mutation.
    if (pendingShopTransactionEvents.size() ==
        pendingShopTransactionEvents.max_size())
        return false;
    pendingShopTransactionEvents.reserve(
        pendingShopTransactionEvents.size() + 1);
    if (!actor->GetInventory().TryApplyTransactionAtomically(removals, additions))
        return false;
    pendingShopTransactionEvents.push_back(std::move(event));
    return true;
}

bool World::PublishDialogueNode(const ActiveDialogueSession &session,
                                const DialogueNodeDefinition &node)
{
    if (node.text.empty() || !IsValidDialogueNodeKind(node.kind))
        return false;
    std::vector<DialogueEventChoice> eventChoices;
    if (node.kind == DialogueNodeKind::CHOICE)
    {
        eventChoices.reserve(node.choices.size());
        for (const DialogueChoiceDefinition &choice : node.choices)
        {
            if (!IsValidDialogueChoiceId(choice.id) || choice.text.empty())
                return false;
            eventChoices.push_back({choice.id, choice.text});
        }
    }
    QuestDialogueAction questAction=QuestDialogueAction::NONE; Player* questPlayer=dynamic_cast<Player*>(entityManager.GetEntityByID(session.actorEntityID)); if(node.id==DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME&&questPlayer){const auto state=questPlayer->GetQuestJournal().Get().state;if(state==QuestState::AVAILABLE)questAction=QuestDialogueAction::ACCEPT_GATHERING_BASICS;else if(state==QuestState::READY_TO_COMPLETE)questAction=QuestDialogueAction::COMPLETE_GATHERING_BASICS;}
    pendingNpcTalkEvents.push_back({session.actorEntityID, session.npcEntityID,
        session.npcType, session.sessionId, session.dialogueId, node.id,
        node.text, node.kind, node.kind == DialogueNodeKind::TERMINAL,
        std::move(eventChoices), node.offersTrade,questAction});
    return true;
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
    ActionValidationResult validation =
        ValidateRecipeActionReadOnly(
            entityID,
            recipeType);

    if (!validation.valid &&
        validation.reason ==
            ActionCancelReason::OUT_OF_RANGE)
    {
        switch (recipeType)
        {
        case RecipeType::BRONZE_BAR:
        case RecipeType::IRON_BAR:
        case RecipeType::STEEL_BAR:
        {
            const RecipeDefinition &recipe =
                RecipeDatabase::Get(
                    recipeType);

            CleanupInvalidActiveStationEntry(
                entityID,
                recipe.GetRequiredStationType());
            break;
        }

        case RecipeType::NONE:
        default:
            break;
        }
    }

    return validation;
}

ActionValidationResult World::ValidateRecipeActionReadOnly(
    int entityID,
    RecipeType recipeType) const
{
    const Entity *entity =
        entityManager.GetEntityByID(
            entityID);

    const Player *player =
        dynamic_cast<const Player *>(entity);

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

    if (!IsStationUsable(
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

MeleeCompletionContext World::BuildMeleeCompletionContext(
    int attackerEntityID,
    int defenderEntityID) const
{
    MeleeCompletionContext context;
    context.attackerEntityID = attackerEntityID;
    context.defenderEntityID = defenderEntityID;

    const Entity *attackerEntity =
        entityManager.GetEntityByID(attackerEntityID);
    if (attackerEntity == nullptr)
    {
        context.status =
            MeleeCompletionContextStatus::MISSING_ATTACKER;
        return context;
    }

    const Entity *defenderEntity =
        entityManager.GetEntityByID(defenderEntityID);
    if (defenderEntity == nullptr)
    {
        context.status =
            MeleeCompletionContextStatus::MISSING_DEFENDER;
        return context;
    }

    const Combatant *attacker =
        dynamic_cast<const Combatant *>(attackerEntity);
    if (attacker == nullptr)
    {
        context.status =
            MeleeCompletionContextStatus::INVALID_ATTACKER;
        return context;
    }

    const Combatant *defender =
        dynamic_cast<const Combatant *>(defenderEntity);
    if (defender == nullptr)
    {
        context.status =
            MeleeCompletionContextStatus::INVALID_DEFENDER;
        return context;
    }

    context.status =
        MeleeCompletionContextStatus::VALID;
    context.attackerRatings =
        attacker->GetCombatRatings();
    context.defenderRatings =
        defender->GetCombatRatings();
    return context;
}

void World::ProcessCompletedActions(
    const std::vector<Action> &completedActions)
{
    for (const Action &action : completedActions)
    {
        if (action.GetType() ==
            ActionType::MELEE_ATTACK)
        {
            const ActionValidationResult validation =
                ValidateMeleeAttackAction(
                    action.GetOwnerID(),
                    action.GetTargetID());

            MeleeCompletionContext context;
            if (validation.valid)
            {
                context = BuildMeleeCompletionContext(
                    action.GetOwnerID(),
                    action.GetTargetID());
            }

            MeleeCompletionOutcome meleeOutcome =
                combatService.EvaluateCompletedMeleeAction(
                    action,
                    validation,
                    context);

            if (meleeOutcome.type ==
                MeleeCompletionOutcomeType::IGNORE)
            {
                continue;
            }

            if (meleeOutcome.type ==
                MeleeCompletionOutcomeType::CLEAR_STALE)
            {
                continue;
            }

            if (meleeOutcome.type ==
                MeleeCompletionOutcomeType::CANCEL)
            {
                if (!meleeOutcome.message.empty())
                {
                    Logger::Game(
                        meleeOutcome.message);
                }

                continue;
            }

            Entity *defenderEntity =
                entityManager.GetEntityByID(
                    meleeOutcome.defenderEntityID);

            Combatant *defender =
                TryGetCombatant(
                    defenderEntity);

            if (defender == nullptr)
            {
                continue;
            }

            RecordActionCompletedEvent(action);

            const bool defenderWasAliveBeforeAttack =
                defender->IsAlive();

            MeleeAttackResult resolvedAttack =
                combatService.EvaluateMeleeAttack(
                    meleeOutcome.attackerRatings,
                    meleeOutcome.defenderRatings);

            resolvedAttack.actualDamageApplied =
                combatService.ApplyMeleeDamage(
                    resolvedAttack.rolledDamage,
                    *defender);

            lastMeleeAttackResult =
                resolvedAttack;

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
                    meleeOutcome.defenderEntityID,
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
            RecipeActionCompletionOutcome recipeOutcome =
                recipeActionSystem.EvaluateCompletedAction(
                    action,
                    entityManager,
                    [this](
                        int actorEntityID,
                        RecipeType recipeType)
                    {
                        return ValidateRecipeActionReadOnly(
                            actorEntityID,
                            recipeType);
                    });

            if (recipeOutcome.type ==
                RecipeActionCompletionOutcomeType::IGNORE)
            {
                continue;
            }

            if (recipeOutcome.type ==
                RecipeActionCompletionOutcomeType::CLEAR_STALE)
            {
                continue;
            }

            if (recipeOutcome.type ==
                RecipeActionCompletionOutcomeType::CANCEL)
            {
                if (!recipeOutcome.message.empty())
                {
                    Logger::Game(
                        recipeOutcome.message);
                }

                if (recipeOutcome.cancelReason ==
                    ActionCancelReason::OUT_OF_RANGE)
                {
                    const RecipeDefinition &recipe =
                        RecipeDatabase::Get(
                            recipeOutcome.recipeType);

                    CleanupInvalidActiveStationEntry(
                        recipeOutcome.actorEntityID,
                        recipe.GetRequiredStationType());
                }

                continue;
            }

            Entity *entity =
                entityManager.GetEntityByID(
                    recipeOutcome.actorEntityID);

            Player *player =
                dynamic_cast<Player *>(entity);

            if (player == nullptr)
            {
                continue;
            }

            bool created =
                RecipeSystem::TryCreateRecipe(
                    *player,
                    recipeOutcome.recipeType);

            if (!created)
            {
                Logger::Game(
                    "The recipe could not be completed");

                continue;
            }

            RecordActionCompletedEvent(action);

            Logger::Game(
                "Created: " +
                recipeOutcome.recipeName);

            ActionValidationResult repeatValidation =
                ValidateRecipeAction(
                    recipeOutcome.actorEntityID,
                    recipeOutcome.recipeType);

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

        GatheringCompletionOutcome gatheringOutcome =
            gatheringSystem.EvaluateCompletedAction(
                action,
                entityManager,
                objectManager,
                [this](
                    int actorEntityID,
                    int resourceID,
                    bool checkInventorySpace)
                {
                    return ValidateGatheringAction(
                        actorEntityID,
                        resourceID,
                        checkInventorySpace);
                },
                *gatheringRandomSource);

        if (gatheringOutcome.type ==
            GatheringCompletionOutcomeType::IGNORE)
        {
            continue;
        }

        if (gatheringOutcome.type ==
            GatheringCompletionOutcomeType::CLEAR_STALE)
        {
            continue;
        }

        if (gatheringOutcome.type ==
            GatheringCompletionOutcomeType::CANCEL)
        {
            if (!gatheringOutcome.message.empty())
            {
                Logger::Game(
                    gatheringOutcome.message);
            }

            ClearPendingResourceInteraction(
                action.GetOwnerID());

            CancelActionsForEntity(
                action.GetOwnerID(),
                gatheringOutcome.cancelReason);

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

        const ResourceDefinition &resourceDefinition =
            ResourceDatabase::Get(
                resource->GetResourceType());

        if (gatheringOutcome.type ==
            GatheringCompletionOutcomeType::SUCCESSFUL_ROLL)
        {
            bool itemAdded =
                player->GetInventory().AddItem(
                    gatheringOutcome.rewardItem,
                    gatheringOutcome.rewardAmount);

            if (!itemAdded)
            {
                Logger::Game(
                    "Player inventory is full");

                ClearPendingResourceInteraction(action.GetOwnerID());

                continue;
            }
            for(int questUnit=0;questUnit<gatheringOutcome.rewardAmount;++questUnit) QuestSystem::RecordGathered(player->GetQuestJournal(),gatheringOutcome.rewardItem);

            int previousLevel =
                player->GetSkills()
                    .GetSkill(gatheringOutcome.requiredSkill)
                    .GetLevel();

            player->GetSkills().AddXP(
                gatheringOutcome.requiredSkill,
                gatheringOutcome.xpReward);

            const Skill &gatheringSkill =
                player->GetSkills()
                    .GetSkill(gatheringOutcome.requiredSkill);

            const ItemDefinition &rewardDefinition =
                ItemDatabase::Get(
                    gatheringOutcome.rewardItem);

            Logger::Game(
                "Player " +
                std::to_string(
                    player->GetID()) +
                " gathered from " +
                resourceDefinition.GetName() +
                " | Received: " +
                std::to_string(
                    gatheringOutcome.rewardAmount) +
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
            ClearPendingResourceInteraction(action.GetOwnerID());

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

    ClearPendingMovementForEntity(monsterEntityID);

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
        monster->GetAttackDurationTicks());
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
    meleeEngagementSystem.ClearEngagementsInvolving(entityID);
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

    movementSystem.CancelMovement(entityID);
}

void World::HandleCombatantDeath(
    int deadEntityID,
    int killerEntityID)
{
    dialogueSystem.CancelActor(deadEntityID);
    shopSystem.CancelActor(deadEntityID);
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

    interactionSystem.ClearInteraction(deadEntityID);

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

void World::ProcessScheduledEvents()
{
    std::vector<ScheduledEvent> dueEvents =
        tickScheduler.PopDueEvents(currentTick);

    for (const ScheduledEvent &event : dueEvents)
    {
        std::visit(
            [this, eventID = event.eventID](const auto &data)
            {
                HandleScheduledEvent(data, eventID);
            },
            event.data);
    }
}

void World::HandleScheduledEvent(
    const MonsterRespawnScheduledEvent &event,
    std::uint64_t eventID)
{
    auto association = monsterRespawnEventIDs.find(
        event.monsterEntityID);
    if (association == monsterRespawnEventIDs.end() ||
        association->second != eventID)
    {
        return;
    }
    monsterRespawnEventIDs.erase(association);

    Entity *entity = entityManager.GetEntityByID(
        event.monsterEntityID);
    Monster *monster = dynamic_cast<Monster *>(entity);
    if (monster == nullptr)
    {
        return;
    }

    std::optional<MonsterRespawnDefinition> definition =
        monster->GetRespawnDefinition();
    if (monster->IsAlive() ||
        !definition.has_value() ||
        definition->delayTicks <= 0)
    {
        if (monster->IsAlive())
        {
            processedDeathEntityIDs.erase(event.monsterEntityID);
        }
        return;
    }

    ExecuteMonsterRespawn(*monster);
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
    if (monsterRespawnEventIDs.find(
            monsterEntityID) !=
        monsterRespawnEventIDs.end())
    {
        return false;
    }

    std::uint64_t eventID = tickScheduler.Schedule(
        respawnTick,
        monsterEntityID,
        MonsterRespawnScheduledEvent{monsterEntityID});
    if (eventID == 0)
    {
        return false;
    }

    monsterRespawnEventIDs.emplace(monsterEntityID, eventID);
    return true;
}

bool World::RemoveEntity(int entityID)
{
    for (const auto &entity : entityManager.GetEntities())
    {
        const auto pending = interactionSystem.GetInteraction(entity->GetID());
        if (pending.has_value() && pending->targetType == InteractionTargetType::NPC &&
            pending->targetObjectID == entityID)
            movementSystem.CancelMovement(entity->GetID());
    }
    ClearPendingMovementForEntity(entityID);
    ClearPendingMeleeInteractionsInvolvingEntity(entityID);
    interactionSystem.ClearInteraction(entityID);
    interactionSystem.ClearInteractionsTargeting(entityID);
    dialogueSystem.CancelActor(entityID);
    dialogueSystem.CancelTarget(entityID);
    shopSystem.CancelActor(entityID);
    shopSystem.CancelTarget(entityID);

    auto association = monsterRespawnEventIDs.find(entityID);
    if (association != monsterRespawnEventIDs.end())
    {
        tickScheduler.Cancel(association->second);
        monsterRespawnEventIDs.erase(association);
    }
    processedDeathEntityIDs.erase(entityID);
    respawnedMonsterEntityIDsThisTick.erase(entityID);
    const bool removed = entityManager.RemoveEntity(entityID);
    if (removed)
        npcSpawnManager.UnregisterEntity(entityID);
    return removed;
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

    interactionSystem.ClearInteraction(monsterEntityID);

    openedStations.erase(
        monsterEntityID);

    activeStations.erase(
        monsterEntityID);

    processedDeathEntityIDs.erase(
        monsterEntityID);

    npcSpawnManager.ResetWanderState(monsterEntityID, currentTick);

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

const NpcSpawnManager &World::GetNpcSpawnManager() const
{
    return npcSpawnManager;
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

#include "World.h"
#include <iostream>
#include "Object/Resource/ResourceNode.h"
#include "../Core/Logger.h"
#include "../Action/Action.h"
#include "../Movement/Movement.h"
#include <cstdlib>
#include "../Player/Player.h"
#include "../Inventory/ItemType.h"
#include "../Skills/SkillType.h"
#include "../Equipment/EquipmentSlotType.h"
#include "../Item/ItemDatabase.h"
#include "../Item/ToolType.h"
#include <string>
#include "Object/Resource/ResourceDatabase.h"
#include "../Core/Random.h"
#include "../Recipe/RecipeSystem.h"
#include "../Recipe/RecipeDatabase.h"

World::World()
    : map(100, 100)
{
    entityManager.CreateNPC(3, 3);

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
    pendingResourceInteractions[entityID] =
        resourceID;
}

void World::QueueStationInteraction(
    int entityID,
    int stationID)
{
    pendingStationInteractions[entityID] =
        stationID;
}

void World::CancelActionsForEntity(
    int entityID,
    ActionCancelReason reason)
{
    actionManager.CancelActionsForEntity(
        entityID,
        reason);

    activeRecipeLoops.erase(
        entityID);
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
        return false;
    }

    if (actionManager.HasActionForEntity(
            entityID))
    {
        Logger::Game(
            "You are already performing an action");

        return false;
    }

    const RecipeDefinition &recipe =
        RecipeDatabase::Get(
            recipeType);

    if (!CanUseStation(
            entityID,
            recipe.GetRequiredStationType()))
    {
        Logger::Game(
            "You must be beside the correct crafting station");

        return false;
    }

    if (!RecipeSystem::CanCreateRecipe(
            *player,
            recipeType))
    {
        Logger::Game(
            "You do not meet the recipe requirements");

        return false;
    }

    activeRecipeLoops[entityID] =
        recipeType;

    actionManager.AddAction(
        Action(
            ActionType::RECIPE,
            recipe.GetName(),
            recipe.GetActionDurationTicks(),
            entityID,
            static_cast<int>(
                recipeType)));

    Logger::Game(
        "You begin " +
        recipe.GetName());

    return true;
}

void World::Update()
{
    Logger::Debug("Updating World");

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
}

Map &World::GetMap()
{
    return map;
}
void World::QueueMovementRequest(const MovementRequest &request)
{
    movementRequests.push(request);
}
void World::QueueMovementDestination(
    const MovementDestinationRequest &request)
{
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

    const ResourceDefinition &resourceDefinition =
        ResourceDatabase::Get(
            resource->GetResourceType());

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

    if (weaponDefinition.HasSkillRequirement())
    {
        SkillType toolSkill =
            weaponDefinition.GetRequiredSkill();

        int playerLevel =
            player->GetSkills()
                .GetSkill(toolSkill)
                .GetLevel();

        int requiredLevel =
            weaponDefinition.GetRequiredSkillLevel();

        if (playerLevel < requiredLevel)
        {
            return {
                false,
                ActionCancelReason::REQUIREMENTS_FAILED,
                "You need level " +
                    std::to_string(requiredLevel) +
                    " in the required skill to use " +
                    weaponDefinition.GetName()};
        }
    }

    SkillType requiredSkill =
        resourceDefinition.GetRequiredSkill();

    int playerSkillLevel =
        player->GetSkills()
            .GetSkill(requiredSkill)
            .GetLevel();

    int requiredSkillLevel =
        resourceDefinition.GetRequiredSkillLevel();

    if (playerSkillLevel < requiredSkillLevel)
    {
        std::string skillName =
            "the required skill";

        switch (requiredSkill)
        {
        case SkillType::WOODCUTTING:
            skillName = "Woodcutting";
            break;

        case SkillType::MINING:
            skillName = "Mining";
            break;

        case SkillType::SMITHING:
            skillName = "Smithing";
            break;

        default:
            break;
        }

        return {
            false,
            ActionCancelReason::REQUIREMENTS_FAILED,
            "You need " +
                skillName +
                " level " +
                std::to_string(requiredSkillLevel) +
                " to gather from " +
                resourceDefinition.GetName()};
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
            ActionType::RECIPE)
        {
            const int entityID =
                action.GetEntityID();

            Entity *entity =
                entityManager.GetEntityByID(
                    entityID);

            Player *player =
                dynamic_cast<Player *>(entity);

            if (player == nullptr)
            {
                activeRecipeLoops.erase(
                    entityID);

                continue;
            }

            RecipeType recipeType =
                static_cast<RecipeType>(
                    action.GetTargetID());

            const RecipeDefinition &recipe =
                RecipeDatabase::Get(
                    recipeType);

            if (!CanUseStation(
                    entityID,
                    recipe.GetRequiredStationType()))
            {
                activeRecipeLoops.erase(
                    entityID);

                Logger::Game(
                    "You are no longer beside the required station");

                continue;
            }

            bool created =
                RecipeSystem::TryCreateRecipe(
                    *player,
                    recipeType);

            if (!created)
            {
                activeRecipeLoops.erase(
                    entityID);

                Logger::Game(
                    "The recipe could not be completed");

                continue;
            }

            Logger::Game(
                "Created: " +
                recipe.GetName());

            auto loopIterator =
                activeRecipeLoops.find(
                    entityID);

            bool shouldRepeat =
                loopIterator !=
                    activeRecipeLoops.end() &&
                loopIterator->second ==
                    recipeType;

            if (!shouldRepeat)
            {
                continue;
            }

            if (!RecipeSystem::CanCreateRecipe(
                    *player,
                    recipeType))
            {
                activeRecipeLoops.erase(
                    loopIterator);

                Logger::Game(
                    "You do not have enough materials to continue");

                continue;
            }

            actionManager.AddAction(
                Action(
                    ActionType::RECIPE,
                    recipe.GetName(),
                    recipe.GetActionDurationTicks(),
                    entityID,
                    static_cast<int>(
                        recipeType)));

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
                action.GetEntityID());

        Player *player =
            dynamic_cast<Player *>(entity);

        if (player == nullptr)
        {
            continue;
        }

        ActionValidationResult validation =
            ValidateGatheringAction(
                action.GetEntityID(),
                action.GetTargetID(),
                true);

        if (!validation.valid)
        {
            if (!validation.message.empty())
            {
                Logger::Game(validation.message);
            }

            pendingResourceInteractions.erase(
                action.GetEntityID());

            actionManager.CancelActionsForEntity(
                action.GetEntityID(),
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
                    action.GetEntityID());

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
                action.GetEntityID());

            Logger::Game(
                resourceDefinition.GetName() +
                " has been depleted");

            continue;
        }

        ActionValidationResult repeatValidation =
            ValidateGatheringAction(
                action.GetEntityID(),
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
    if (newDefinition.HasSkillRequirement())
    {
        SkillType requiredSkill =
            newDefinition.GetRequiredSkill();

        int requiredLevel =
            newDefinition.GetRequiredSkillLevel();

        int playerLevel =
            player->GetSkills()
                .GetSkill(requiredSkill)
                .GetLevel();

        if (playerLevel < requiredLevel)
        {
            Logger::Game(
                "You need level " +
                std::to_string(requiredLevel) +
                " in the required skill to equip " +
                newDefinition.GetName());

            return false;
        }
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

    CancelGatheringForToolChange(
        entityID);

    Logger::Game(
        "Weapon unequipped");

    return true;
}

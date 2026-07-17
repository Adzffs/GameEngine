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

void World::CancelActionsForEntity(
    int entityID)
{
    actionManager.CancelActionsForEntity(
        entityID);
}

void World::ClearPendingResourceInteraction(
    int entityID)
{
    pendingResourceInteractions.erase(entityID);
}
void World::Update()
{
    Logger::Debug("Updating World");

    ProcessMovementDestinationRequests();
    ProcessActiveMovementPaths();
    ProcessResourceInteractions();

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

        Player *player =
            dynamic_cast<Player *>(entity);

        if (player == nullptr)
        {
            interactionIterator =
                pendingResourceInteractions.erase(
                    interactionIterator);

            continue;
        }

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

        if (weaponDefinition.GetToolType() !=
            resourceDefinition.GetRequiredToolType())
        {
            Logger::Game(
                "You need to equip the correct tool");

            interactionIterator =
                pendingResourceInteractions.erase(
                    interactionIterator);

            continue;
        }

        if (weaponDefinition.HasSkillRequirement())
        {
            SkillType toolSkill =
                weaponDefinition.GetRequiredSkill();

            int playerLevel =
                player->GetSkills()
                    .GetSkill(toolSkill)
                    .GetLevel();

            int toolLevelRequirement =
                weaponDefinition
                    .GetRequiredSkillLevel();

            if (playerLevel <
                toolLevelRequirement)
            {
                Logger::Game(
                    "You need level " +
                    std::to_string(
                        toolLevelRequirement) +
                    " in the required skill to use " +
                    weaponDefinition.GetName());

                interactionIterator =
                    pendingResourceInteractions.erase(
                        interactionIterator);

                continue;
            }
        }

        SkillType requiredSkill =
            resourceDefinition.GetRequiredSkill();

        int playerSkillLevel =
            player->GetSkills()
                .GetSkill(requiredSkill)
                .GetLevel();

        if (playerSkillLevel <
            resourceDefinition
                .GetRequiredSkillLevel())
        {
            Logger::Game(
                "You need Woodcutting level " +
                std::to_string(
                    resourceDefinition
                        .GetRequiredSkillLevel()) +
                " to chop " +
                resourceDefinition.GetName());

            interactionIterator =
                pendingResourceInteractions.erase(
                    interactionIterator);

            continue;
        }

        if (!actionManager.HasActionForEntity(
                entityID))
        {
            actionManager.AddAction(
                Action(
                    "Gathering " +
                        resourceDefinition.GetName(),
                    weaponDefinition
                        .GetActionDurationTicks(),
                    entityID,
                    resourceID));
        }

        ++interactionIterator;
    }
}
void World::ProcessCompletedActions(
    const std::vector<Action> &completedActions)
{
    for (const Action &action : completedActions)
    {
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

        auto interactionIterator =
            pendingResourceInteractions.find(
                action.GetEntityID());

        bool stillInteractingWithResource =
            interactionIterator !=
                pendingResourceInteractions.end() &&
            interactionIterator->second ==
                action.GetTargetID();

        if (!stillInteractingWithResource)
        {
            continue;
        }

        ItemType equippedWeapon =
            player->GetEquipment()
                .GetEquippedItem(
                    EquipmentSlotType::WEAPON);

        const ItemDefinition &weaponDefinition =
            ItemDatabase::Get(
                equippedWeapon);

        if (weaponDefinition.GetToolType() !=
            resourceDefinition.GetRequiredToolType())
        {
            pendingResourceInteractions.erase(
                action.GetEntityID());

            continue;
        }

        // Start another attempt after either success or failure.
        actionManager.AddAction(
            Action(
                "Gathering " +
                    resourceDefinition.GetName(),
                weaponDefinition
                    .GetActionDurationTicks(),
                action.GetEntityID(),
                action.GetTargetID()));
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

        return true;
    }

    Logger::Game(
        newDefinition.GetName() +
        " equipped");

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

    Logger::Game(
        "Weapon unequipped");

    return true;
}

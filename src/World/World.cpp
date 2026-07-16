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

World::World()
    : map(100, 100)
{
    entityManager.CreateNPC(3, 3);
    CreateResource(5, 5);
}
void World::CreateResource(int x, int y)
{
    objectManager.CreateResource(x, y);

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

        ItemType equippedWeapon =
            player->GetEquipment()
                .GetEquippedItem(
                    EquipmentSlotType::WEAPON);

        const ItemDefinition &weaponDefinition =
            ItemDatabase::Get(
                equippedWeapon);

        if (weaponDefinition.GetToolType() !=
            ToolType::AXE)
        {
            Logger::Game(
                "You need to equip an axe to chop this tree");

            interactionIterator =
                pendingResourceInteractions.erase(
                    interactionIterator);

            continue;
        }
        if (weaponDefinition.HasSkillRequirement())
        {
            int playerLevel =
                player->GetSkills()
                    .GetSkill(
                        SkillType::WOODCUTTING)
                    .GetLevel();

            int requiredLevel =
                weaponDefinition
                    .GetRequiredSkillLevel();

            if (playerLevel < requiredLevel)
            {
                Logger::Game(
                    "You need Woodcutting level " +
                    std::to_string(requiredLevel) +
                    " to use a " +
                    weaponDefinition.GetName());

                interactionIterator =
                    pendingResourceInteractions.erase(
                        interactionIterator);

                continue;
            }
        }

        if (!actionManager.HasActionForEntity(
                entityID))
        {
            actionManager.AddAction(
                Action(
                    "Chopping Tree",
                    weaponDefinition
                        .GetActionDurationTicks(),
                    entityID,
                    resourceID));
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

        bool itemAdded =
            player->GetInventory().AddItem(
                ItemType::LOG,
                1);

        if (!itemAdded)
        {
            Logger::Game(
                "Player inventory is full");

            continue;
        }

        int previousLevel =
            player->GetSkills()
                .GetSkill(SkillType::WOODCUTTING)
                .GetLevel();

        player->GetSkills().AddXP(
            SkillType::WOODCUTTING,
            25);

        const Skill &woodcutting =
            player->GetSkills()
                .GetSkill(SkillType::WOODCUTTING);

        Logger::Game(
            "Player " +
            std::to_string(player->GetID()) +
            " chopped 1 log | Logs: " +
            std::to_string(
                player->GetInventory().GetItemAmount(
                    ItemType::LOG)) +
            " | Woodcutting: " +
            std::to_string(woodcutting.GetXP()) +
            " XP (Level " +
            std::to_string(woodcutting.GetLevel()) +
            ")");

        if (woodcutting.GetLevel() > previousLevel)
        {
            Logger::Game(
                "Woodcutting level increased to " +
                std::to_string(
                    woodcutting.GetLevel()));
        }

        resource->Deplete();
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

        if (requiredSkill ==
            SkillType::WOODCUTTING)
        {
            int playerLevel =
                player->GetSkills()
                    .GetSkill(
                        SkillType::WOODCUTTING)
                    .GetLevel();

            if (playerLevel < requiredLevel)
            {
                Logger::Game(
                    "You need Woodcutting level " +
                    std::to_string(requiredLevel) +
                    " to equip a " +
                    newDefinition.GetName());

                return false;
            }
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
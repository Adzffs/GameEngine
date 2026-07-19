#include "EntityManager.h"
#include "../../Movement/Movement.h"
#include "../../Movement/MovementRequest.h"
#include "../../Player/Player.h"
#include "../../Entity/Monster/Monster.h"
#include "../../World/World.h"
#include <algorithm>

EntityManager::EntityManager()
{
}
int EntityManager::CreatePlayer()
{
    int playerID = nextID;

    if (RegisterEntity(
            std::make_unique<Player>(playerID)) == nullptr)
    {
        return 0;
    }

    nextID++;

    return playerID;
}
int EntityManager::CreateNPC(int x, int y)
{
    int npcID = nextID;

    std::unique_ptr<NPC> npc =
        std::make_unique<NPC>(npcID);
    npc->GetPosition().SetPosition(x, y);

    if (RegisterEntity(std::move(npc)) == nullptr)
    {
        return 0;
    }

    nextID++;

    return npcID;
}

int EntityManager::CreateMonster(
    int x,
    int y,
    const CombatRatings &ratings,
    RewardTableType rewardTableType,
    std::optional<MonsterRespawnDefinition> respawnDefinition,
    std::optional<MonsterAggressionDefinition> aggressionDefinition,
    NpcType npcType,
    int attackDurationTicks,
    NpcSpawnId npcSpawnId,
    int wanderRadius,
    int wanderIntervalTicks)
{
    int monsterID = nextID;

    if (RegisterEntity(
            std::make_unique<Monster>(
            monsterID,
            x,
            y,
            ratings,
            rewardTableType,
            aggressionDefinition,
            respawnDefinition,
            npcType,
            attackDurationTicks,
            npcSpawnId,
            wanderRadius,
            wanderIntervalTicks)) == nullptr)
    {
        return 0;
    }

    nextID++;

    return monsterID;
}

Entity *EntityManager::GetEntityByID(int id)
{
    if (id <= 0)
    {
        return nullptr;
    }

    auto iterator = entityLookup.find(id);
    return iterator == entityLookup.end()
               ? nullptr
               : iterator->second;
}

const Entity *EntityManager::GetEntityByID(int id) const
{
    if (id <= 0)
    {
        return nullptr;
    }

    auto iterator = entityLookup.find(id);
    return iterator == entityLookup.end()
               ? nullptr
               : iterator->second;
}

Entity *EntityManager::RegisterEntity(
    std::unique_ptr<Entity> entity)
{
    if (entity == nullptr)
    {
        return nullptr;
    }

    int id = entity->GetID();
    if (id <= 0 || entityLookup.find(id) != entityLookup.end())
    {
        return nullptr;
    }

    Entity *entityPointer = entity.get();
    entities.push_back(std::move(entity));

    try
    {
        auto [iterator, inserted] =
            entityLookup.emplace(id, entityPointer);
        (void)iterator;

        if (!inserted)
        {
            entities.pop_back();
            return nullptr;
        }
    }
    catch (...)
    {
        entities.pop_back();
        throw;
    }

    return entityPointer;
}

bool EntityManager::RemoveEntity(int id)
{
    auto lookupIterator = entityLookup.find(id);
    if (lookupIterator == entityLookup.end())
    {
        return false;
    }

    Entity *entityPointer = lookupIterator->second;
    auto entityIterator = std::find_if(
        entities.begin(),
        entities.end(),
        [entityPointer](const std::unique_ptr<Entity> &entity)
        {
            return entity.get() == entityPointer;
        });

    if (entityIterator == entities.end())
    {
        entityLookup.erase(lookupIterator);
        return false;
    }

    entityLookup.erase(lookupIterator);
    entities.erase(entityIterator);
    return true;
}

bool EntityManager::RollbackLastCreatedEntity(int id)
{
    if (id <= 0 || nextID != id + 1 || entities.empty() ||
        entities.back()->GetID() != id)
    {
        return false;
    }

    if (!RemoveEntity(id))
    {
        return false;
    }

    nextID = id;
    return true;
}
const std::vector<std::unique_ptr<Entity>> &
EntityManager::GetEntities() const
{
    return entities;
}
void EntityManager::Update(World &world)
{

    for (auto &entity : entities)
    {
        entity->Update(world);
    }
}

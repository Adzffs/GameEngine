#include "EntityManager.h"
#include "../../Movement/Movement.h"
#include "../../Movement/MovementRequest.h"
#include "../../Player/Player.h"
#include "../../Entity/Monster/Monster.h"
#include "../../World/World.h"

EntityManager::EntityManager()
{
}
int EntityManager::CreatePlayer()
{
    int playerID = nextID;

    entities.push_back(
        std::make_unique<Player>(playerID));

    nextID++;

    return playerID;
}
int EntityManager::CreateNPC(int x, int y)
{
    int npcID = nextID;

    entities.push_back(
        std::make_unique<NPC>(npcID));

    entities.back()
        ->GetPosition()
        .SetPosition(x, y);

    nextID++;

    return npcID;
}

int EntityManager::CreateMonster(
    int x,
    int y,
    const CombatRatings &ratings)
{
    int monsterID = nextID;

    entities.push_back(
        std::make_unique<Monster>(
            monsterID,
            x,
            y,
            ratings));

    nextID++;

    return monsterID;
}

Entity *EntityManager::GetEntityByID(int id)
{
    for (const auto &entity : entities)
    {
        if (entity->GetID() == id)
        {
            return entity.get();
        }
    }

    return nullptr;
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
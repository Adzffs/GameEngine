#pragma once
#include <memory>
#include "../Entity.h"
#include <unordered_map>
#include <vector>
#include "../../Player/Player.h"
#include "../../NPC/NPC.h"
#include "../../Entity/Monster/MonsterAggressionDefinition.h"
#include "../../Entity/Monster/MonsterRespawnDefinition.h"
#include "../../Reward/RewardTableType.h"
#include "../../Stats/CombatRatings.h"
#include <optional>
class World;
struct EntityManagerTestAccess;

class EntityManager
{
public:
    EntityManager();

    int CreatePlayer();

    int CreateNPC(int x, int y);

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

    void Update(World &world);

    Entity *GetEntityByID(int id);
    const Entity *GetEntityByID(int id) const;

    const std::vector<std::unique_ptr<Entity>> &GetEntities() const;

private:
    Entity *RegisterEntity(std::unique_ptr<Entity> entity);
    bool RemoveEntity(int id);

    // Vector order is authoritative for deterministic iteration. Moving a
    // unique_ptr during vector growth does not move its heap-allocated Entity.
    std::vector<std::unique_ptr<Entity>> entities;
    // Non-owning pointers remain stable until the matching owner is removed.
    std::unordered_map<int, Entity *> entityLookup;

    int nextID = 1;

    friend struct EntityManagerTestAccess;
    friend class World;
};

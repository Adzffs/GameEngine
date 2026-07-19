#pragma once
#include <memory>
#include "../Entity.h"
#include <vector>
#include "../../Input/InputManager.h"
#include "../../Player/Player.h"
#include "../../NPC/NPC.h"
#include "../../Entity/Monster/MonsterAggressionDefinition.h"
#include "../../Entity/Monster/MonsterRespawnDefinition.h"
#include "../../Reward/RewardTableType.h"
#include "../../Stats/CombatRatings.h"
#include <optional>
class World;

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

    const std::vector<std::unique_ptr<Entity>> &GetEntities() const;

private:
    std::vector<std::unique_ptr<Entity>> entities;

    int nextID = 1;

    InputManager input;
};
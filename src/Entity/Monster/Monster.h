#pragma once

#include "../../Combat/Combatant.h"
#include "../Entity.h"
#include "MonsterAggressionDefinition.h"
#include "MonsterRespawnDefinition.h"
#include "../../Reward/RewardTableType.h"
#include "../../Stats/HealthPool.h"
#include <optional>

class Monster : public Entity, public Combatant
{
public:
    Monster(
        int id,
        int x,
        int y,
        const CombatRatings &ratings,
        RewardTableType rewardTableType =
            RewardTableType::NONE,
        std::optional<MonsterAggressionDefinition> aggressionDefinition =
            std::nullopt,
        std::optional<MonsterRespawnDefinition> respawnDefinition =
            std::nullopt);

    void Update(World &world) override;

    CombatRatings GetCombatRatings() const override;

    int GetCurrentHealth() const override;
    int GetMaximumHealth() const override;
    bool IsAlive() const override;

    int ApplyDamage(int amount) override;
    void RestoreHealthToFull();

    RewardTableType GetRewardTableType() const;
    bool HasRespawnDefinition() const;
    std::optional<MonsterRespawnDefinition> GetRespawnDefinition() const;
    bool HasAggressionDefinition() const;
    std::optional<MonsterAggressionDefinition> GetAggressionDefinition() const;

    static constexpr int InvalidAggressionTargetEntityID = -1;
    int GetAggressionTargetEntityID() const;
    void SetAggressionTargetEntityID(int targetEntityID);
    void ClearAggressionTargetEntityID();

    int GetOriginalSpawnX() const;
    int GetOriginalSpawnY() const;

private:
    CombatRatings combatRatings;
    HealthPool healthPool;
    RewardTableType rewardTableType;
    std::optional<MonsterAggressionDefinition> aggressionDefinition;
    std::optional<MonsterRespawnDefinition> respawnDefinition;
    int aggressionTargetEntityID =
        InvalidAggressionTargetEntityID;
    const int originalSpawnX;
    const int originalSpawnY;
};

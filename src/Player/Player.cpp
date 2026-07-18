#include "Player.h"
#include "../World/World.h"
#include "../Core/DevelopmentConfig.h"
#include "../Skills/SkillType.h"
#include <iostream>

Player::Player(int id)
    : Entity(id, EntityType::PLAYER),
      healthPool(1)
{
    inventory.AddItem(
        ItemType::BRONZE_AXE,
        1);

    inventory.AddItem(
        ItemType::BRONZE_PICKAXE,
        1);

    inventory.AddItem(
        ItemType::IRON_PICKAXE,
        1);

    inventory.AddItem(
        ItemType::STEEL_PICKAXE,
        1);

    if (DevelopmentConfig::ENABLE_TEST_PLAYER)
    {
        skills.AddXP(
            SkillType::WOODCUTTING,
            DevelopmentConfig::TEST_WOODCUTTING_XP);

        skills.AddXP(
            SkillType::MINING,
            DevelopmentConfig::TEST_MINING_XP);

        skills.AddXP(
            SkillType::SMITHING,
            DevelopmentConfig::TEST_SMITHING_XP);
    }

    RefreshDerivedState();
    RestoreHealthToFull();
}

void Player::Update(World &world)
{
    (void)world;
}
Inventory &Player::GetInventory()
{
    return inventory;
}
const Inventory &Player::GetInventory() const
{
    return inventory;
}
SkillSet &Player::GetSkills()
{
    return skills;
}

const SkillSet &Player::GetSkills() const
{
    return skills;
}
Equipment &Player::GetEquipment()
{
    return equipment;
}

const Equipment &Player::GetEquipment() const
{
    return equipment;
}

int Player::GetBaseStat(StatType stat) const
{
    switch (stat)
    {
    case StatType::ATTACK_ACCURACY:
        return skills.GetSkill(
                         SkillType::ATTACK)
            .GetLevel();

    case StatType::MELEE_STRENGTH:
        return skills.GetSkill(
                         SkillType::ATTACK)
            .GetLevel();

    case StatType::DEFENCE:
        return skills.GetSkill(
                         SkillType::DEFENCE)
            .GetLevel();

    case StatType::MAX_HEALTH:
        return 100;

    case StatType::COUNT:
    default:
        return 0;
    }
}

int Player::GetEquipmentBonus(StatType stat) const
{
    return equipment.GetTotalStatBonuses().Get(stat);
}

int Player::GetTotalStat(StatType stat) const
{
    int total =
        GetBaseStat(stat) +
        GetEquipmentBonus(stat);

    if (stat == StatType::MAX_HEALTH &&
        total < 1)
    {
        return 1;
    }

    return total;
}

int Player::GetCurrentHealth() const
{
    return healthPool.GetCurrentHealth();
}

int Player::GetMaximumHealth() const
{
    return healthPool.GetMaximumHealth();
}

bool Player::IsAlive() const
{
    return healthPool.IsAlive();
}

void Player::ApplyDamage(int amount)
{
    healthPool.ApplyDamage(amount);
}

void Player::Heal(int amount)
{
    healthPool.Heal(amount);
}

void Player::RestoreHealthToFull()
{
    healthPool.RestoreToFull();
}

CombatRatings Player::GetCombatRatings() const
{
    return CombatRatings{
        GetTotalStat(StatType::ATTACK_ACCURACY),
        GetTotalStat(StatType::MELEE_STRENGTH),
        GetTotalStat(StatType::DEFENCE),
        GetTotalStat(StatType::MAX_HEALTH)};
}

void Player::RefreshDerivedState()
{
    healthPool.SetMaximumHealth(
        GetTotalStat(StatType::MAX_HEALTH));
}

#include "Player.h"
#include "../World/World.h"
#include "../Core/DevelopmentConfig.h"
#include "../Skills/SkillType.h"
#include <iostream>
#include <limits>

namespace
{
    int ClampToIntRange(long long value)
    {
        if (value > static_cast<long long>(
                        std::numeric_limits<int>::max()))
        {
            return std::numeric_limits<int>::max();
        }

        if (value < static_cast<long long>(
                        std::numeric_limits<int>::min()))
        {
            return std::numeric_limits<int>::min();
        }

        return static_cast<int>(value);
    }

    int GetStatusEffectModifierForStat(
        const StatusEffectModifiers &modifiers,
        StatType stat)
    {
        switch (stat)
        {
        case StatType::ATTACK_ACCURACY:
            return modifiers.meleeAccuracy;

        case StatType::MELEE_STRENGTH:
            return modifiers.meleeStrength;

        case StatType::DEFENCE:
            return modifiers.meleeDefence;

        case StatType::MAX_HEALTH:
            return modifiers.maximumHealth;

        case StatType::COUNT:
        default:
            return 0;
        }
    }
}

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

StatusEffectManager &Player::GetStatusEffectManager()
{
    return statusEffectManager;
}

const StatusEffectManager &Player::GetStatusEffectManager() const
{
    return statusEffectManager;
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
    StatusEffectModifiers statusModifiers =
        statusEffectManager.GetCombinedModifiers();

    long long combined =
        static_cast<long long>(
            GetBaseStat(stat)) +
        static_cast<long long>(
            GetEquipmentBonus(stat)) +
        static_cast<long long>(
            GetStatusEffectModifierForStat(
                statusModifiers,
                stat));

    int total =
        ClampToIntRange(combined);

    if ((stat == StatType::ATTACK_ACCURACY ||
         stat == StatType::MELEE_STRENGTH ||
         stat == StatType::DEFENCE) &&
        total < 0)
    {
        return 0;
    }

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

int Player::ApplyDamage(int amount)
{
    return healthPool.ApplyDamage(amount);
}

bool Player::TryHeal(int amount)
{
    if (amount <= 0)
    {
        return false;
    }

    if (!IsAlive())
    {
        return false;
    }

    const int currentHealth =
        healthPool.GetCurrentHealth();

    const int maximumHealth =
        healthPool.GetMaximumHealth();

    if (currentHealth >= maximumHealth)
    {
        return false;
    }

    int missingHealth =
        maximumHealth - currentHealth;

    int appliedHealing = amount;

    if (appliedHealing > missingHealth)
    {
        appliedHealing = missingHealth;
    }

    if (appliedHealing <= 0)
    {
        return false;
    }

    healthPool.Heal(appliedHealing);
    return true;
}

void Player::Heal(int amount)
{
    TryHeal(amount);
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

CombatFormulas::MeleeCombatProfile Player::GetMeleeCombatProfile() const
{
    return CombatFormulas::BuildMeleeCombatProfile(
        GetCombatRatings());
}

void Player::RefreshDerivedState()
{
    healthPool.SetMaximumHealth(
        GetTotalStat(StatType::MAX_HEALTH));
}

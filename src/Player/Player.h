#pragma once

#include "../Entity/Entity.h"
#include "../Combat/Combatant.h"
#include "../Combat/CombatFormulas.h"
#include "../Inventory/Inventory.h"
#include "../Skills/SkillSet.h"
#include "../Equipment/Equipment.h"
#include "../Stats/CombatRatings.h"
#include "../Stats/HealthPool.h"
#include "../Stats/StatType.h"
#include "../StatusEffect/StatusEffectManager.h"
#include "PlayerInitializationMode.h"

class World;

class Player : public Entity, public Combatant
{
public:
    Player(
        int id,
        PlayerInitializationMode initializationMode =
            PlayerInitializationMode::DEVELOPMENT_DEFAULTS);

    void Update(World &world) override;

    Inventory &GetInventory();
    const Inventory &GetInventory() const;

    SkillSet &GetSkills();
    const SkillSet &GetSkills() const;

    Equipment &GetEquipment();
    const Equipment &GetEquipment() const;

    StatusEffectManager &GetStatusEffectManager();
    const StatusEffectManager &GetStatusEffectManager() const;

    int GetBaseStat(StatType stat) const;
    int GetEquipmentBonus(StatType stat) const;
    int GetTotalStat(StatType stat) const;

    int GetCurrentHealth() const override;
    int GetMaximumHealth() const override;
    bool IsAlive() const override;

    int ApplyDamage(int amount) override;
    bool TryHeal(int amount);
    void Heal(int amount);
    void RestoreHealthToFull();

    CombatRatings GetCombatRatings() const override;
    CombatFormulas::MeleeCombatProfile GetMeleeCombatProfile() const;

    // Call after changes that can affect derived stats such as maximum health.
    void RefreshDerivedState();

private:
    Inventory inventory;

    SkillSet skills;

    Equipment equipment;

    StatusEffectManager statusEffectManager;

    HealthPool healthPool;
};

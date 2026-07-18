#pragma once

#include "../Entity/Entity.h"
#include "../Inventory/Inventory.h"
#include "../Skills/SkillSet.h"
#include "../Equipment/Equipment.h"
#include "../Stats/StatType.h"

class World;

class Player : public Entity
{
public:
    Player(int id);

    void Update(World &world) override;

    Inventory &GetInventory();
    const Inventory &GetInventory() const;

    SkillSet &GetSkills();
    const SkillSet &GetSkills() const;

    Equipment &GetEquipment();
    const Equipment &GetEquipment() const;

    int GetBaseStat(StatType stat) const;
    int GetEquipmentBonus(StatType stat) const;
    int GetTotalStat(StatType stat) const;

private:
    Inventory inventory;

    SkillSet skills;

    Equipment equipment;
};
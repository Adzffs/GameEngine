#pragma once

#include "../Entity/Entity.h"
#include "../Inventory/Inventory.h"
#include "../Skills/SkillSet.h"

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

private:
    Inventory inventory;

    SkillSet skills;
};
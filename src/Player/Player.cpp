#include "Player.h"
#include "../World/World.h"
#include "../Core/DevelopmentConfig.h"
#include "../Skills/SkillType.h"
#include <iostream>

Player::Player(int id)
    : Entity(id, EntityType::PLAYER)
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

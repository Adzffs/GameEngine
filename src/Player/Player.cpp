#include "Player.h"
#include "../World/World.h"

#include <iostream>

Player::Player(int id)
    : Entity(id, EntityType::PLAYER)
{
}

void Player::Update(World &world)
{
    (void)world;
}
Inventory &Player::GetInventory()
{
    return inventory;
}
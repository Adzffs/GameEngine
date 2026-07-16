#include "Player.h"
#include "../World/World.h"
#include "../Movement/Movement.h"

#include <iostream>

Player::Player(int id)
    : Entity(id, EntityType::PLAYER)
{
}

void Player::Update(World &world)
{
    MovementRequest request = input.GetMovementRequest();

    Movement::Move(
        *this,
        world.GetMap(),
        request.GetX(),
        request.GetY());
}
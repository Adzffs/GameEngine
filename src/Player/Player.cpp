#include "Player.h"
#include <iostream>

Player::Player(int id)
    : Entity(id, EntityType::PLAYER)
{
}

void Player::Update()
{
    std::cout
        << "Player ID: "
        << GetID()
        << " Position: ("
        << GetPosition().GetX()
        << ", "
        << GetPosition().GetY()
        << ")"
        << std::endl;
}
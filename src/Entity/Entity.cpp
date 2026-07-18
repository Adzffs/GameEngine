#include "Entity.h"
#include "../World/World.h"
#include <iostream>
#include <string>
#include "Core/Logger.h"

Entity::Entity(int id, EntityType type)
    : position(0, 0)
{
    this->id = id;
    this->type = type;
}
Position &Entity::GetPosition()
{
    return position;
}
const Position &Entity::GetPosition() const
{
    return position;
}
int Entity::GetID()
{
    return id;
}

int Entity::GetID() const
{
    return id;
}

EntityType Entity::GetType()
{
    return type;
}

EntityType Entity::GetType() const
{
    return type;
}

void Entity::Update(World &world)
{
    (void)world;

    std::string name;

    switch (type)
    {
    case EntityType::PLAYER:
        name = "PLAYER";
        break;

    case EntityType::NPC:
        name = "NPC";
        break;

    case EntityType::MONSTER:
        name = "MONSTER";
        break;

    case EntityType::RESOURCE:
        name = "RESOURCE";
        break;
    }

    std::cout
        << "Entity ID: "
        << id
        << " Type: "
        << name
        << " Position: ("
        << position.GetX()
        << ", "
        << position.GetY()
        << ")"
        << std::endl;
}
void Entity::StartAction()
{
    std::cout
        << "Entity "
        << id
        << " started an action"
        << std::endl;
}
#include "Entity.h"
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
int Entity::GetID()
{
    return id;
}

EntityType Entity::GetType()
{
    return type;
}

void Entity::Update()
{
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

    Logger::Debug(
        "Entity ID: " + std::to_string(id));
}
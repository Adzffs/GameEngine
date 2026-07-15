#include "Entity.h"
#include <iostream>
#include <string>

Entity::Entity(int id, EntityType type)
{
    this->id = id;
    this->type = type;
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

    std::cout
        << "Entity ID: "
        << id
        << " Type: "
        << name
        << std::endl;
}
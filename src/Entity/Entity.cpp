#include "Entity.h"
#include <iostream>

Entity::Entity(int id)
{
    this->id = id;
}

int Entity::GetID()
{
    return id;
}

void Entity::Update()
{
    std::cout
        << "Updating Entity ID: "
        << id
        << std::endl;
}
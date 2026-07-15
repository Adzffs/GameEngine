#pragma once

#include "EntityType.h"

class Entity
{
public:
    Entity(int id, EntityType type);

    int GetID();

    EntityType GetType();

    void Update();

private:
    int id;

    EntityType type;
};
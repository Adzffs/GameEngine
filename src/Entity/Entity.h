#pragma once

#include "EntityType.h"
#include "../World/Position.h"

class Entity
{
public:
    Entity(int id, EntityType type);

    int GetID();

    EntityType GetType();

    Position &GetPosition();

    void Update();

    void StartAction();

private:
    int id;

    EntityType type;

    Position position;
};
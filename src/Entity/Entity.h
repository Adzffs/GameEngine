#pragma once

#include "EntityType.h"
#include "../World/Position.h"

class World;

class Entity
{
public:
    Entity(int id, EntityType type);

    virtual ~Entity() = default;

    int GetID();

    EntityType GetType();

    Position &GetPosition();

    virtual void Update(World &world) = 0;

    void StartAction();

private:
    int id;

    EntityType type;

    Position position;
};
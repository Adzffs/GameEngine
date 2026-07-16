#pragma once

#include "EntityType.h"
#include "../World/Position.h"

class Entity
{
public:
    Entity(int id, EntityType type);

    virtual ~Entity() = default;

    int GetID();

    EntityType GetType();

    Position &GetPosition();

    virtual void Update();

    void StartAction();

private:
    int id;

    EntityType type;

    Position position;
};
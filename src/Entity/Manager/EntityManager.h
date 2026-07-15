#pragma once

#include "../Entity.h"
#include <vector>

class EntityManager
{
public:
    EntityManager();

    void Update();

    void CreateEntity(EntityType type);

private:
    std::vector<Entity> entities;

    int nextID = 1;
};
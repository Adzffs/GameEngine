#pragma once

#include "../Entity.h"
#include <vector>
#include "../../World/Map.h"

class EntityManager
{
public:
    EntityManager();

    void Update(Map &map);

    void CreateEntity(EntityType type);

private:
    std::vector<Entity> entities;

    int nextID = 1;
};
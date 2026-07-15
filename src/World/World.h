#pragma once

#include "../Entity/Manager/EntityManager.h"
#include "Map.h"

class World
{
public:
    World();

    void Update();

private:
    EntityManager entityManager;

    Map map;
};
#pragma once

#include "../Entity/Manager/EntityManager.h"
#include "Map.h"
#include "Object/Manager/ObjectManager.h"

class World
{
public:
    World();

    void Update();

private:
    EntityManager entityManager;

    ObjectManager objectManager;

    Map map;
};
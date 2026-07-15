#pragma once

#include "../Entity/Manager/EntityManager.h"

class World
{
public:
    World();

    void Update();

private:
    EntityManager entityManager;
};
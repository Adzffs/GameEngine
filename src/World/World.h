#pragma once

#include "../Entity/Entity.h"

class World
{
public:
    World();

    void Update();

private:
    Entity player;
};
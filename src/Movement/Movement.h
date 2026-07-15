#pragma once

#include "../Entity/Entity.h"
#include "../World/Map.h"

class Movement
{
public:
    static bool Move(Entity &entity, Map &map, int x, int y);
};
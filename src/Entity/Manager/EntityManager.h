#pragma once
#include <memory>
#include "../Entity.h"
#include <vector>
#include "../../World/Map.h"
#include "../../Input/InputManager.h"
#include "../../Player/Player.h"

class EntityManager
{
public:
    EntityManager();

    void Update(Map &map);

    void CreateEntity(EntityType type);

    void CreatePlayer();

private:
    std::vector<std::unique_ptr<Entity>> entities;

    int nextID = 1;

    InputManager input;
};
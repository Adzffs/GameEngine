#pragma once
#include <memory>
#include "../Entity.h"
#include <vector>
#include "../../Input/InputManager.h"
#include "../../Player/Player.h"
class World;

class EntityManager
{
public:
    EntityManager();

    void Update(World &world);

    void CreatePlayer();

private:
    std::vector<std::unique_ptr<Entity>> entities;

    int nextID = 1;

    InputManager input;
};
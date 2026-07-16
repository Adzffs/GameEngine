#pragma once
#include <memory>
#include "../Entity.h"
#include <vector>
#include "../../Input/InputManager.h"
#include "../../Player/Player.h"
#include "../../NPC/NPC.h"
class World;

class EntityManager
{
public:
    EntityManager();

    int CreatePlayer();

    int CreateNPC();

    void Update(World &world);

    Entity *GetEntityByID(int id);

private:
    std::vector<std::unique_ptr<Entity>> entities;

    int nextID = 1;

    InputManager input;
};
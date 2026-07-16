#pragma once

#include "../Entity/Entity.h"
#include "../Input/InputManager.h"
class World;

class Player : public Entity
{
public:
    Player(int id);

    void Update(World &world) override;

private:
    InputManager input;
};
#pragma once

#include "../Entity/Entity.h"
class World;

class Player : public Entity
{
public:
    Player(int id);

    void Update(World &world) override;

private:
};
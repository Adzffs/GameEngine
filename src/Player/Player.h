#pragma once

#include "../Entity/Entity.h"

class Player : public Entity
{
public:
    Player(int id);

    void Update() override;
};
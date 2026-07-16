#pragma once

#include "../Entity/Entity.h"

class NPC : public Entity
{
public:
    NPC(int id);

    void Update(World &world) override;
};
#include "NPC.h"
#include "../World/World.h"
#include <iostream>

NPC::NPC(int id)
    : Entity(id, EntityType::NPC)
{
}

void NPC::Update(World &world)
{
    (void)world;
}
#include "NPC.h"
#include "../World/World.h"
#include <iostream>

NPC::NPC(int id, int x, int y, NpcType npcType, NpcSpawnId spawnId)
    : Entity(id, EntityType::NPC), npcType(npcType), npcSpawnId(spawnId)
{
    GetPosition().SetPosition(x, y);
}

NpcType NPC::GetNpcType() const { return npcType; }
NpcSpawnId NPC::GetNpcSpawnId() const { return npcSpawnId; }

void NPC::Update(World &world)
{
    (void)world;
}

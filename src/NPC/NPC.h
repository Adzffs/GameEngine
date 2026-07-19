#pragma once

#include "../Entity/Entity.h"
#include "NpcType.h"
#include "NpcSpawnId.h"

class NPC : public Entity
{
public:
    NPC(int id, int x = 0, int y = 0, NpcType npcType = NpcType::NONE,
        NpcSpawnId spawnId = NpcSpawnId::NONE);

    NpcType GetNpcType() const;
    NpcSpawnId GetNpcSpawnId() const;

    void Update(World &world) override;
private:
    NpcType npcType;
    NpcSpawnId npcSpawnId;
};

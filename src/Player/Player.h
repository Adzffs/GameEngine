#pragma once

#include "../Entity/Entity.h"
#include "../Inventory/Inventory.h"

class World;

class Player : public Entity
{
public:
    Player(int id);

    void Update(World &world) override;

    Inventory &GetInventory();

    const Inventory &GetInventory() const;

private:
    Inventory inventory;
};
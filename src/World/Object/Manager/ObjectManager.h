#pragma once

#include "../Resource/ResourceNode.h"
#include "../Station/CraftingStation.h"
#include <vector>

class ObjectManager
{
public:
    ObjectManager();

    void Update();

    void CreateResource(
        ResourceType resourceType,
        int x,
        int y);

    const std::vector<ResourceNode> &GetResources() const;
    ResourceNode *GetResourceAt(int x, int y);
    ResourceNode *GetResourceByID(int id);

    void CreateStation(
        StationType stationType,
        int x,
        int y);

    const std::vector<CraftingStation> &
    GetStations() const;

    CraftingStation *GetStationAt(
        int x,
        int y);

private:
    std::vector<ResourceNode> resources;
    std::vector<CraftingStation> stations;

    int nextID = 1;
};
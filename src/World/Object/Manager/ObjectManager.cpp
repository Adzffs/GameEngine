#include "ObjectManager.h"

ObjectManager::ObjectManager()
{
}

void ObjectManager::CreateResource(
    ResourceType resourceType,
    int x,
    int y)
{
    ResourceNode resource(
        nextID,
        resourceType,
        x,
        y);

    resources.push_back(resource);

    nextID++;
}
const std::vector<ResourceNode> &
ObjectManager::GetResources() const
{
    return resources;
}
ResourceNode *ObjectManager::GetResourceAt(
    int x,
    int y)
{
    for (ResourceNode &resource : resources)
    {
        if (resource.GetX() == x &&
            resource.GetY() == y)
        {
            return &resource;
        }
    }

    return nullptr;
}
ResourceNode *ObjectManager::GetResourceByID(int id)
{
    for (ResourceNode &resource : resources)
    {
        if (resource.GetID() == id)
        {
            return &resource;
        }
    }

    return nullptr;
}

void ObjectManager::CreateStation(
    StationType stationType,
    int x,
    int y)
{
    stations.emplace_back(
        nextID,
        stationType,
        x,
        y);

    nextID++;
}

const std::vector<CraftingStation> &
ObjectManager::GetStations() const
{
    return stations;
}

CraftingStation *
ObjectManager::GetStationByID(int id)
{
    for (CraftingStation &station : stations)
    {
        if (station.GetID() == id)
        {
            return &station;
        }
    }

    return nullptr;
}

CraftingStation *
ObjectManager::GetStationAt(
    int x,
    int y)
{
    for (CraftingStation &station :
         stations)
    {
        if (station.GetX() == x &&
            station.GetY() == y)
        {
            return &station;
        }
    }

    return nullptr;
}

void ObjectManager::Update()
{

    for (auto &resource : resources)
    {
        resource.Update();
    }
}
#include "ObjectManager.h"
#include <iostream>

ObjectManager::ObjectManager()
{
}

void ObjectManager::CreateResource(int x, int y)
{
    ResourceNode resource(nextID, x, y);

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
void ObjectManager::Update()
{
    std::cout
        << "Updating "
        << resources.size()
        << " World Objects..."
        << std::endl;

    for (auto &resource : resources)
    {
        std::cout
            << "Resource ID: "
            << resource.GetID()
            << " Position: ("
            << resource.GetX()
            << ", "
            << resource.GetY()
            << ")"
            << std::endl;
    }
}
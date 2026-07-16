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